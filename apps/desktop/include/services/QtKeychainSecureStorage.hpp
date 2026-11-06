#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "services/ISecureStorage.hpp"

namespace devdialogue::services {

class QtKeychainSecureStorage : public ISecureStorage {
public:
    explicit QtKeychainSecureStorage(std::string serviceName = "DevDialogue");
    ~QtKeychainSecureStorage() override = default;

    bool writeCredential(const std::string& key, const std::string& secret) override;
    std::optional<std::string> readCredential(const std::string& key) override;
    bool deleteCredential(const std::string& key) override;
    void clearAll() override;

private:
    std::string m_serviceName;
    std::unordered_map<std::string, std::string> m_memoryCache;
    mutable std::mutex m_mutex;
};

} // namespace devdialogue::services
