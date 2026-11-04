#pragma once

#include <string>

namespace devdialogue::core {

class AppConfig {
public:
  AppConfig();

  std::string apiBaseUrl() const;
  void setApiBaseUrl(const std::string &url);

  bool isOfflineModeEnabled() const;
  void setOfflineModeEnabled(bool enabled);

private:
  std::string m_apiBaseUrl;
  bool m_offlineMode;
};

} // namespace devdialogue::core
