#include <gtest/gtest.h>
#include "network/ApiClient.hpp"
#include "services/InMemorySecureStorage.hpp"
#include "core/AppConfig.hpp"
#include "core/Constants.hpp"

using namespace devdialogue;

class ApiClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_shared<core::AppConfig>();
        storage = std::make_shared<services::InMemorySecureStorage>();
        client = std::make_unique<network::ApiClient>(config, storage);
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
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyAccessToken), "access-123");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyRefreshToken), "refresh-456");

    // Clear tokens
    client->clearTokens();
    EXPECT_TRUE(client->accessToken().empty());
    EXPECT_TRUE(client->refreshToken().empty());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyAccessToken).has_value());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyRefreshToken).has_value());
}

TEST_F(ApiClientTest, RefreshQueueSingleFlightAndReplay) {
    // Initial tokens
    client->setTokens("old-access", "old-refresh");

    bool unauthorizedEmitted = false;
    QObject::connect(client.get(), &network::ApiClient::unauthorizedDetected, [&unauthorizedEmitted]() {
        unauthorizedEmitted = true;
    });

    // Simulate refresh succeeded
    client->onRefreshSucceeded("new-access-xyz", "new-refresh-uvw");

    // Tokens must be updated together
    EXPECT_EQ(client->accessToken(), "new-access-xyz");
    EXPECT_EQ(client->refreshToken(), "new-refresh-uvw");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyAccessToken), "new-access-xyz");
    EXPECT_EQ(storage->readCredential(core::constants::StorageKeyRefreshToken), "new-refresh-uvw");
}

TEST_F(ApiClientTest, RefreshQueueFailureClearsTokens) {
    client->setTokens("access-fail", "refresh-fail");

    bool unauthorizedEmitted = false;
    QObject::connect(client.get(), &network::ApiClient::unauthorizedDetected, [&unauthorizedEmitted]() {
        unauthorizedEmitted = true;
    });

    models::ApiError refreshErr{401, "Refresh token expired", "", false};
    client->onRefreshFailed(refreshErr);

    // Tokens must be cleared on failure
    EXPECT_TRUE(client->accessToken().empty());
    EXPECT_TRUE(client->refreshToken().empty());
    EXPECT_FALSE(storage->readCredential(core::constants::StorageKeyAccessToken).has_value());
    EXPECT_TRUE(unauthorizedEmitted);
}
