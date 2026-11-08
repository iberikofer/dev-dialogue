#include "core/AppConfig.hpp"
#include <cstdlib>

namespace devdialogue::core {

AppConfig::AppConfig()
    : m_apiBaseUrl("http://localhost:5238"),
      m_supabaseUrl("https://lzvjmonlkryljwkanexr.supabase.co"),
      m_supabaseAnonKey(""),
      m_offlineMode(false) {

    if (const char* envUrl = std::getenv("DEV_DIALOGUE_API_URL")) {
        m_apiBaseUrl = envUrl;
    }
    if (const char* envSbUrl = std::getenv("DEV_DIALOGUE_SUPABASE_URL")) {
        m_supabaseUrl = envSbUrl;
    }
    if (const char* envAnon = std::getenv("DEV_DIALOGUE_SUPABASE_ANON_KEY")) {
        m_supabaseAnonKey = envAnon;
    }
}

std::string AppConfig::apiBaseUrl() const { return m_apiBaseUrl; }
void AppConfig::setApiBaseUrl(const std::string& url) { m_apiBaseUrl = url; }

std::string AppConfig::supabaseUrl() const { return m_supabaseUrl; }
void AppConfig::setSupabaseUrl(const std::string& url) { m_supabaseUrl = url; }

std::string AppConfig::supabaseAnonKey() const { return m_supabaseAnonKey; }
void AppConfig::setSupabaseAnonKey(const std::string& key) { m_supabaseAnonKey = key; }

bool AppConfig::isOfflineModeEnabled() const { return m_offlineMode; }
void AppConfig::setOfflineModeEnabled(bool enabled) { m_offlineMode = enabled; }

} // namespace devdialogue::core
