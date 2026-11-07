#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "services/ISecureStorage.hpp"

namespace devdialogue::services {

class QtKeychainSecureStorage : public ISecureStorage {
public:
    explicit QtKeychainSecureStorage(std::string serviceName = "DevDialogue");
    ~QtKeychainSecureStorage() override = default;

    bool writeCredential(std::string_view key, std::string_view secret) override;
    std::optional<std::string> readCredential(std::string_view key) override;
    bool deleteCredential(std::string_view key) override;
    void clearAll() override;

private:
    std::string m_serviceName;
    std::unordered_map<std::string, std::string> m_memoryCache;
    mutable std::mutex m_mutex;
};

} // namespace devdialogue::services
