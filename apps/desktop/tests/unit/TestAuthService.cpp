#include <gtest/gtest.h>
#include <QEventLoop>
#include <QTimer>

#include "services/AuthService.hpp"
#include "services/InMemorySecureStorage.hpp"
#include "network/ApiClient.hpp"
#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "tests/helpers/MockHttpServer.hpp"

using namespace devdialogue;

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(server.start());

        config = std::make_shared<core::AppConfig>();
        config->setApiBaseUrl(server.url().toStdString());
        config->setSupabaseUrl(server.url().toStdString());
        config->setSupabaseAnonKey("test-anon-key");

        storage = std::make_shared<services::InMemorySecureStorage>();
        client = std::make_shared<network::ApiClient>(config, storage);
        authService = std::make_shared<services::AuthService>(client, storage, config);
    }

    void TearDown() override {
        server.stop();
    }

    test::MockHttpServer server;
    std::shared_ptr<core::AppConfig> config;
    std::shared_ptr<services::InMemorySecureStorage> storage;
    std::shared_ptr<network::ApiClient> client;
    std::shared_ptr<services::AuthService> authService;
};

TEST_F(AuthServiceTest, SuccessfulLogin) {
    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path.startsWith("/auth/v1/token")) {
            return {
                200, "OK",
                "{\"access_token\": \"jwt-access-token\", \"refresh_token\": \"jwt-refresh-token\", \"user\": {\"id\": \"u-123\", \"email\": \"user@example.com\"}}",
                "application/json", {}
            };
        }
        if (req.path == "/api/auth/me") {
            QString auth = req.headers.value("authorization");
            EXPECT_EQ(auth, "Bearer jwt-access-token");
            return {
                200, "OK",
                "{\"id\": \"u-123\", \"email\": \"user@example.com\", \"fullName\": \"Test Dev\", \"avatarUrl\": null, \"role\": \"user\"}",
                "application/json", {}
            };
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    QEventLoop loop;
    bool loginSuccess = false;
    QString loginError;

    authService->loginWithEmail("user@example.com", "valid_password", [&](bool ok, const QString& err) {
        loginSuccess = ok;
        loginError = err;
        loop.quit();
    });

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(loginSuccess);
    EXPECT_TRUE(loginError.isEmpty());
    EXPECT_TRUE(authService->isAuthenticated());
    ASSERT_TRUE(authService->currentUser().has_value());
    EXPECT_EQ(authService->currentUser()->id, "u-123");
    EXPECT_EQ(authService->currentUser()->email, "user@example.com");
    EXPECT_EQ(authService->currentUser()->fullName.value_or(""), "Test Dev");
    EXPECT_EQ(authService->currentUser()->role, models::UserRole::User);
    EXPECT_EQ(client->accessToken(), "jwt-access-token");
    EXPECT_EQ(client->refreshToken(), "jwt-refresh-token");
}

TEST_F(AuthServiceTest, InvalidPasswordLogin) {
    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path.startsWith("/auth/v1/token")) {
            return {
                400, "Bad Request",
                "{\"error\": \"invalid_grant\", \"error_description\": \"Invalid login credentials\"}",
                "application/json", {}
            };
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    QEventLoop loop;
    bool loginSuccess = true;
    QString loginError;
    QString signalError;

    QObject::connect(authService.get(), &services::AuthService::authErrorOccurred, [&](const QString& err) {
        signalError = err;
    });

    authService->loginWithEmail("user@example.com", "wrong_password", [&](bool ok, const QString& err) {
        loginSuccess = ok;
        loginError = err;
        loop.quit();
    });

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_FALSE(loginSuccess);
    EXPECT_FALSE(authService->isAuthenticated());
    EXPECT_EQ(loginError, "Invalid login credentials");
    EXPECT_EQ(signalError, "Invalid login credentials");
    EXPECT_TRUE(client->accessToken().empty());
}

TEST_F(AuthServiceTest, SignupWithoutSessionRequiresEmailConfirmation) {
    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path.startsWith("/auth/v1/signup")) {
            // Supabase returns user without access_token when email confirmation is enabled
            return {
                200, "OK",
                "{\"id\": \"u-new\", \"email\": \"newbie@example.com\", \"identities\": []}",
                "application/json", {}
            };
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    QEventLoop loop;
    bool registerSuccess = false;
    bool confirmationReq = false;
    QString confirmationSignalEmail;

    QObject::connect(authService.get(), &services::AuthService::emailConfirmationRequired, [&](const QString& email) {
        confirmationSignalEmail = email;
    });

    authService->registerUser("newbie@example.com", "pass123", "Newbie Dev", [&](bool ok, bool reqConfirm, const QString&) {
        registerSuccess = ok;
        confirmationReq = reqConfirm;
        loop.quit();
    });

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(registerSuccess);
    EXPECT_TRUE(confirmationReq);
    EXPECT_EQ(confirmationSignalEmail, "newbie@example.com");
    EXPECT_FALSE(authService->isAuthenticated());
    EXPECT_TRUE(client->accessToken().empty());
}

TEST_F(AuthServiceTest, MockLoginAdminRestrictedInRelease) {
    authService->mockLogin(models::UserRole::Admin);

    ASSERT_TRUE(authService->currentUser().has_value());

#ifdef NDEBUG
    // In Release builds, mockLogin MUST NEVER grant Admin role
    EXPECT_EQ(authService->currentUser()->role, models::UserRole::User);
    EXPECT_FALSE(authService->isAdmin());
#else
    // In Debug builds, mockLogin(Admin) grants Admin for development
    EXPECT_EQ(authService->currentUser()->role, models::UserRole::Admin);
    EXPECT_TRUE(authService->isAdmin());
#endif
}

TEST_F(AuthServiceTest, RestoreSessionFromSavedToken) {
    // Pre-populate storage with valid tokens
    storage->writeCredential(core::constants::StorageKeyAccessToken, "saved-access-token");
    storage->writeCredential(core::constants::StorageKeyRefreshToken, "saved-refresh-token");

    // Recreate client to load stored tokens on boot
    auto clientWithStorage = std::make_shared<network::ApiClient>(config, storage);
    auto authWithStorage = std::make_shared<services::AuthService>(clientWithStorage, storage, config);

    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path == "/api/auth/me") {
            QString auth = req.headers.value("authorization");
            EXPECT_EQ(auth, "Bearer saved-access-token");
            return {
                200, "OK",
                "{\"id\": \"u-restored\", \"email\": \"restored@dev.local\", \"fullName\": \"Restored User\", \"role\": \"user\"}",
                "application/json", {}
            };
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    QEventLoop loop;
    bool restoreSuccess = false;

    authWithStorage->restoreSession([&](bool ok) {
        restoreSuccess = ok;
        loop.quit();
    });

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(restoreSuccess);
    EXPECT_TRUE(authWithStorage->isAuthenticated());
    ASSERT_TRUE(authWithStorage->currentUser().has_value());
    EXPECT_EQ(authWithStorage->currentUser()->id, "u-restored");
    EXPECT_EQ(authWithStorage->currentUser()->email, "restored@dev.local");
}
