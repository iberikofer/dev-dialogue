#pragma once

#include <string>

namespace devdialogue::core {

class AppConfig {
public:
    AppConfig();

    std::string apiBaseUrl() const;
    void setApiBaseUrl(const std::string& url);

    std::string supabaseUrl() const;
    void setSupabaseUrl(const std::string& url);

    std::string supabaseAnonKey() const;
    void setSupabaseAnonKey(const std::string& key);

    bool isOfflineModeEnabled() const;
    void setOfflineModeEnabled(bool enabled);

private:
    std::string m_apiBaseUrl;
    std::string m_supabaseUrl;
    std::string m_supabaseAnonKey;
    bool m_offlineMode{false};
};

} // namespace devdialogue::core
