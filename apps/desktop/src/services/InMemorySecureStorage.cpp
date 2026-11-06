#include "services/InMemorySecureStorage.hpp"

namespace devdialogue::services {

bool InMemorySecureStorage::writeCredential(const std::string& key, const std::string& secret) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage[key] = secret;
    return true;
}

std::optional<std::string> InMemorySecureStorage::readCredential(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_storage.find(key);
    if (it != m_storage.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool InMemorySecureStorage::deleteCredential(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_storage.erase(key) > 0;
}

void InMemorySecureStorage::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage.clear();
}

} // namespace devdialogue::services
