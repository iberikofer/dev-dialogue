#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace devdialogue::services {

class ISecureStorage {
public:
    virtual ~ISecureStorage() = default;

    virtual bool writeCredential(std::string_view key, std::string_view secret) = 0;
    virtual std::optional<std::string> readCredential(std::string_view key) = 0;
    virtual bool deleteCredential(std::string_view key) = 0;
    virtual void clearAll() = 0;
};

} // namespace devdialogue::services
