#include <gtest/gtest.h>
#include "services/InMemorySecureStorage.hpp"
#include <thread>
#include <vector>

using namespace devdialogue::services;

TEST(SecureStorageTest, BasicCrudOperations) {
    InMemorySecureStorage storage;

    EXPECT_FALSE(storage.readCredential("jwt_token").has_value());

    EXPECT_TRUE(storage.writeCredential("jwt_token", "secret-token-123"));
    auto readBack = storage.readCredential("jwt_token");
    ASSERT_TRUE(readBack.has_value());
    EXPECT_EQ(*readBack, "secret-token-123");

    // Overwrite
    EXPECT_TRUE(storage.writeCredential("jwt_token", "updated-secret"));
    EXPECT_EQ(*storage.readCredential("jwt_token"), "updated-secret");

    // Delete
    EXPECT_TRUE(storage.deleteCredential("jwt_token"));
    EXPECT_FALSE(storage.readCredential("jwt_token").has_value());
    EXPECT_FALSE(storage.deleteCredential("jwt_token"));
}

TEST(SecureStorageTest, ClearAll) {
    InMemorySecureStorage storage;
    storage.writeCredential("key1", "val1");
    storage.writeCredential("key2", "val2");

    EXPECT_TRUE(storage.readCredential("key1").has_value());
    EXPECT_TRUE(storage.readCredential("key2").has_value());

    storage.clearAll();
    EXPECT_FALSE(storage.readCredential("key1").has_value());
    EXPECT_FALSE(storage.readCredential("key2").has_value());
}

TEST(SecureStorageTest, ConcurrentAccess) {
    InMemorySecureStorage storage;
    const int threadCount = 8;
    const int opsPerThread = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&storage, t]() {
            std::string key = "thread_key_" + std::to_string(t);
            for (int i = 0; i < opsPerThread; ++i) {
                storage.writeCredential(key, "val_" + std::to_string(i));
                auto val = storage.readCredential(key);
                (void)val;
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    for (int t = 0; t < threadCount; ++t) {
        std::string key = "thread_key_" + std::to_string(t);
        EXPECT_TRUE(storage.readCredential(key).has_value());
    }
}
