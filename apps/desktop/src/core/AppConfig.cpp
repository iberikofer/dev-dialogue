#include "core/AppConfig.hpp"
#include <cstdlib>

namespace devdialogue::core {

AppConfig::AppConfig()
    : m_apiBaseUrl("http://localhost:5238"), m_offlineMode(false) {
  if (const char *envUrl = std::getenv("DEV_DIALOGUE_API_URL")) {
    m_apiBaseUrl = envUrl;
  }
}

std::string AppConfig::apiBaseUrl() const { return m_apiBaseUrl; }

void AppConfig::setApiBaseUrl(const std::string &url) { m_apiBaseUrl = url; }

bool AppConfig::isOfflineModeEnabled() const { return m_offlineMode; }

void AppConfig::setOfflineModeEnabled(bool enabled) { m_offlineMode = enabled; }

} // namespace devdialogue::core
