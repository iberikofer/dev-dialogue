#include <gtest/gtest.h>
#include <QEventLoop>
#include <QTimer>
#include <atomic>

#include "network/ApiClient.hpp"
#include "services/AuthService.hpp"
#include "services/InMemorySecureStorage.hpp"
#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "tests/helpers/MockHttpServer.hpp"

using namespace devdialogue;

class ApiClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_shared<core::AppConfig>();
        storage = std::make_shared<services::InMemorySecureStorage>();
        client = std::make_unique<network::ApiClient>(config, storage);
    }

    void triggerRefreshSucceeded(network::ApiClient* c, const std::string& a, const std::string& r) {
        c->onRefreshSucceeded(a, r);
    }

    void triggerRefreshFailed(network::ApiClient* c, const models::ApiError& err) {
        c->onRefreshFailed(err);
    }

    std::shared_ptr<core::AppConfig> config;
    std::shared_ptr<services::InMemorySecureStorage> storage;
    std::unique_ptr<network::ApiClient> client;
};

TEST_F(ApiClientTest, ConstantsValidation) {
    EXPECT_EQ(core::constants::ColdStartTimeoutMs, 90000);
    EXPECT_EQ(core::constants::StandardTimeoutMs, 30000);
    EXPECT_EQ(core::constants::MaxGetRetries, 2);
    EXPECT_EQ(core::constants::WakingUpThresholdMs, 3000);
}

TEST_F(ApiClientTest, TokenStorageIntegration) {
    EXPECT_TRUE(client->accessToken().empty());
    EXPECT_TRUE(client->refreshToken().empty());

    client->setTokens("access-123", "refresh-456");

    EXPECT_EQ(client->accessToken(), "access-123");
    EXPECT_EQ(client->refreshToken(), "refresh-456");

    // Verify written to storage
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyAccessToken).value_or(""), "access-123");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyRefreshToken).value_or(""), "refresh-456");

    // Clear tokens
    client->clearTokens();
    EXPECT_TRUE(client->accessToken().empty());
    EXPECT_TRUE(client->refreshToken().empty());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyAccessToken).has_value());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyRefreshToken).has_value());
}

TEST_F(ApiClientTest, DirectTokenUpdateOnRefreshSuccess) {
    // Initial tokens
    client->setTokens("old-access", "old-refresh");

    // Direct invocation via friend class helper
    triggerRefreshSucceeded(client.get(), "new-access-xyz", "new-refresh-uvw");

    // Tokens must be updated together
    EXPECT_EQ(client->accessToken(), "new-access-xyz");
    EXPECT_EQ(client->refreshToken(), "new-refresh-uvw");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyAccessToken).value_or(""), "new-access-xyz");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyRefreshToken).value_or(""), "new-refresh-uvw");
}

TEST_F(ApiClientTest, DirectTokenClearanceOnRefreshFailure) {
    client->setTokens("access-fail", "refresh-fail");

    bool unauthorizedEmitted = false;
    QObject::connect(client.get(), &network::ApiClient::unauthorizedDetected, [&unauthorizedEmitted]() {
        unauthorizedEmitted = true;
    });

    models::ApiError refreshErr{401, "Refresh token expired", "", false};
    // Direct invocation via friend class helper
    triggerRefreshFailed(client.get(), refreshErr);

    // Tokens must be cleared on failure
    EXPECT_TRUE(client->accessToken().empty());
    EXPECT_TRUE(client->refreshToken().empty());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyAccessToken).has_value());
    EXPECT_TRUE(unauthorizedEmitted);
}

// ==========================================
// Real QTcpServer Refresh Queue Tests
// ==========================================

TEST(ApiClientQueueTest, RefreshQueue_ThreeParallelGets_SingleRefresh_AllReplayedWith200) {
    test::MockHttpServer server;
    ASSERT_TRUE(server.start());

    std::atomic<int> getWithOldToken{0};
    std::atomic<int> getWithNewToken{0};
    std::atomic<int> refreshCalls{0};

    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path.startsWith("/api/data/")) {
            QString auth = req.headers.value("authorization");
            if (auth == "Bearer new-access-token") {
                getWithNewToken++;
                return {200, "OK", "{\"id\": \"success-data\"}", "application/json", {}};
            } else {
                getWithOldToken++;
                return {401, "Unauthorized", "{\"error\": \"Unauthorized\"}", "application/json", {}};
            }
        }
        if (req.path.startsWith("/auth/v1/token")) {
            refreshCalls++;
            return {200, "OK", "{\"access_token\": \"new-access-token\", \"refresh_token\": \"new-refresh-token\"}", "application/json", {}};
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    auto testConfig = std::make_shared<core::AppConfig>();
    testConfig->setApiBaseUrl(server.url().toStdString());
    testConfig->setSupabaseUrl(server.url().toStdString());
    testConfig->setSupabaseAnonKey("test-anon-key");

    auto testStorage = std::make_shared<services::InMemorySecureStorage>();
    auto testClient = std::make_shared<network::ApiClient>(testConfig, testStorage);
    auto testAuthService = std::make_shared<services::AuthService>(testClient, testStorage, testConfig);

    testClient->setTokens("old-access-token", "old-refresh-token");

    QEventLoop loop;
    int completedCount = 0;
    int successCount = 0;

    auto onComplete = [&](const std::variant<nlohmann::json, models::ApiError>& res) {
        if (std::holds_alternative<nlohmann::json>(res)) {
            successCount++;
        }
        completedCount++;
        if (completedCount == 3) {
            loop.quit();
        }
    };

    // Dispatch 3 parallel GET requests that will all hit 401
    testClient->get<nlohmann::json>("/api/data/1", onComplete);
    testClient->get<nlohmann::json>("/api/data/2", onComplete);
    testClient->get<nlohmann::json>("/api/data/3", onComplete);

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    // Verification
    EXPECT_EQ(completedCount, 3);
    EXPECT_EQ(successCount, 3);
    EXPECT_EQ(refreshCalls.load(), 1);       // Refresh called exactly ONCE (single-flight)
    EXPECT_EQ(getWithOldToken.load(), 3);    // 3 initial requests with old token
    EXPECT_EQ(getWithNewToken.load(), 3);    // All 3 replayed with new token
    EXPECT_EQ(testClient->accessToken(), "new-access-token");
    EXPECT_EQ(testClient->refreshToken(), "new-refresh-token");
    EXPECT_EQ(testStorage->readCredential(core::constants::StorageKeyAccessToken).value_or(""), "new-access-token");
    EXPECT_EQ(testStorage->readCredential(core::constants::StorageKeyRefreshToken).value_or(""), "new-refresh-token");
}

TEST(ApiClientQueueTest, RefreshQueue_RefreshFailsWith401_TokensCleared_NoReplays) {
    test::MockHttpServer server;
    ASSERT_TRUE(server.start());

    std::atomic<int> getCalls{0};
    std::atomic<int> refreshCalls{0};

    server.setHandler([&](const test::MockHttpServer::HttpRequest& req) -> test::MockHttpServer::HttpResponse {
        if (req.path.startsWith("/api/data/")) {
            getCalls++;
            return {401, "Unauthorized", "{\"error\": \"Unauthorized\"}", "application/json", {}};
        }
        if (req.path.startsWith("/auth/v1/token")) {
            refreshCalls++;
            return {401, "Unauthorized", "{\"error\": \"invalid_grant\", \"error_description\": \"Refresh token expired\"}", "application/json", {}};
        }
        return {404, "Not Found", "{}", "application/json", {}};
    });

    auto testConfig = std::make_shared<core::AppConfig>();
    testConfig->setApiBaseUrl(server.url().toStdString());
    testConfig->setSupabaseUrl(server.url().toStdString());
    testConfig->setSupabaseAnonKey("test-anon-key");

    auto testStorage = std::make_shared<services::InMemorySecureStorage>();
    auto testClient = std::make_shared<network::ApiClient>(testConfig, testStorage);
    auto testAuthService = std::make_shared<services::AuthService>(testClient, testStorage, testConfig);

    testClient->setTokens("invalid-access", "invalid-refresh");

    bool unauthorizedDetectedEmitted = false;
    QObject::connect(testClient.get(), &network::ApiClient::unauthorizedDetected, [&]() {
        unauthorizedDetectedEmitted = true;
    });

    QEventLoop loop;
    int completedCount = 0;
    int errorCount = 0;

    auto onComplete = [&](const std::variant<nlohmann::json, models::ApiError>& res) {
        if (std::holds_alternative<models::ApiError>(res)) {
            errorCount++;
        }
        completedCount++;
        if (completedCount == 3) {
            loop.quit();
        }
    };

    // Dispatch 3 parallel GET requests
    testClient->get<nlohmann::json>("/api/data/1", onComplete);
    testClient->get<nlohmann::json>("/api/data/2", onComplete);
    testClient->get<nlohmann::json>("/api/data/3", onComplete);

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    // Verification
    EXPECT_EQ(completedCount, 3);
    EXPECT_EQ(errorCount, 3);
    EXPECT_EQ(refreshCalls.load(), 1);
    EXPECT_EQ(getCalls.load(), 3);          // NO replays sent!
    EXPECT_TRUE(testClient->accessToken().empty());
    EXPECT_TRUE(testClient->refreshToken().empty());
    EXPECT_FALSE(testStorage->readCredential(core::constants::StorageKeyAccessToken).has_value());
    EXPECT_FALSE(testStorage->readCredential(core::constants::StorageKeyRefreshToken).has_value());
    EXPECT_TRUE(unauthorizedDetectedEmitted);
}
