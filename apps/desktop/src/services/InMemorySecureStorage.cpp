#include "services/InMemorySecureStorage.hpp"

namespace devdialogue::services {

bool InMemorySecureStorage::writeCredential(std::string_view key, std::string_view secret) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage[std::string(key)] = std::string(secret);
    return true;
}

std::optional<std::string> InMemorySecureStorage::readCredential(std::string_view key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_storage.find(std::string(key));
    if (it != m_storage.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool InMemorySecureStorage::deleteCredential(std::string_view key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_storage.erase(std::string(key)) > 0;
}

void InMemorySecureStorage::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage.clear();
}

} // namespace devdialogue::services
