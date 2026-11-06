#pragma once

#include <string>
#include <optional>

namespace devdialogue::services {

class ISecureStorage {
public:
    virtual ~ISecureStorage() = default;

    virtual bool writeCredential(const std::string& key, const std::string& secret) = 0;
    virtual std::optional<std::string> readCredential(const std::string& key) = 0;
    virtual bool deleteCredential(const std::string& key) = 0;
    virtual void clearAll() = 0;
};

} // namespace devdialogue::services
