#include "services/AuthService.hpp"

#include <QString>

#include <nlohmann/json.hpp>
#include <utility>

#include "core/AppConfig.hpp"
#include "network/ApiClient.hpp"
#include "services/SecureStorage.hpp"

namespace devdialogue::services {

namespace {

nlohmann::json profileToJson(const models::UserProfile &profile,
                             bool isMockSession) {
  nlohmann::json json;
  json["id"] = profile.id;
  json["email"] = profile.email;
  json["fullName"] = profile.fullName ? nlohmann::json(*profile.fullName)
                                      : nlohmann::json(nullptr);
  json["avatarUrl"] = profile.avatarUrl ? nlohmann::json(*profile.avatarUrl)
                                        : nlohmann::json(nullptr);
  json["role"] = profile.role;
  json["dailyInterviewsCount"] = profile.dailyInterviewsCount;
  json["lastInterviewDate"] =
      profile.lastInterviewDate ? nlohmann::json(*profile.lastInterviewDate)
                                : nlohmann::json(nullptr);
  json["createdAt"] = profile.createdAt;
  json["isMockSession"] = isMockSession;
  return json;
}

models::UserProfile profileFromJson(const nlohmann::json &json) {
  models::UserProfile profile;
  profile.id = json.value("id", "");
  profile.email = json.value("email", "");
  if (json.contains("fullName") && !json["fullName"].is_null())
    profile.fullName = json["fullName"].get<std::string>();
  if (json.contains("avatarUrl") && !json["avatarUrl"].is_null())
    profile.avatarUrl = json["avatarUrl"].get<std::string>();
  if (json.contains("role") && !json["role"].is_null())
    profile.role = json["role"].get<models::UserRole>();
  profile.dailyInterviewsCount = json.value("dailyInterviewsCount", 0);
  if (json.contains("lastInterviewDate") &&
      !json["lastInterviewDate"].is_null())
    profile.lastInterviewDate = json["lastInterviewDate"].get<std::string>();
  profile.createdAt = json.value("createdAt", "");
  return profile;
}

} // namespace

AuthService::AuthService(std::shared_ptr<network::ApiClient> apiClient,
                         std::shared_ptr<SecureStorage> storage,
                         std::shared_ptr<core::AppConfig> config,
                         QObject *parent)
    : QObject(parent), m_apiClient(std::move(apiClient)),
      m_storage(std::move(storage)), m_config(std::move(config)) {}

std::optional<models::UserProfile> AuthService::currentUser() const {
  return m_user;
}

bool AuthService::isAuthenticated() const { return m_user.has_value(); }

bool AuthService::isMockSession() const { return m_isMockSession; }

void AuthService::restoreSession() {
  if (!m_storage)
    return;

  const std::optional<std::string> stored = m_storage->readCredential("user_profile");
  if (!stored)
    return;

  try {
    const nlohmann::json json = nlohmann::json::parse(*stored);
    m_user = profileFromJson(json);
    m_isMockSession = json.value("isMockSession", false);

    if (m_isMockSession) {
      if (m_config)
        m_config->setOfflineModeEnabled(true);
      emit userChanged();
      return;
    }
  } catch (const std::exception &) {
    clearPersistedProfile();
    return;
  }

  // Validate a real session against the backend. A 401 is handled by
  // ApiClient::unauthorizedDetected and clears the session there.
  m_apiClient->get<models::AuthMeResponse>(
      "/api/auth/me",
      [this](const std::variant<models::AuthMeResponse, network::ApiError> &result) {
        if (std::holds_alternative<models::AuthMeResponse>(result)) {
          applyAuthMe(std::get<models::AuthMeResponse>(result));
        }
      });
}

void AuthService::applyAuthMe(const models::AuthMeResponse &response) {
  models::UserProfile profile;
  if (m_user)
    profile = *m_user;

  profile.id = response.id;
  profile.email = response.email;
  profile.fullName = response.fullName;
  profile.avatarUrl = response.avatarUrl;
  profile.role = response.role;

  m_user = profile;
  m_isMockSession = false;
  if (m_config)
    m_config->setOfflineModeEnabled(false);

  persistProfile();
  emit userChanged();
}

void AuthService::setSession(const models::UserProfile &profile,
                             const std::string &token) {
  m_user = profile;
  m_isMockSession = false;
  if (m_config)
    m_config->setOfflineModeEnabled(false);

  if (m_apiClient && !token.empty())
    m_apiClient->setBearerToken(token);

  persistProfile();
  emit userChanged();
}

void AuthService::mockLogin(models::UserRole role) {
  const bool isAdmin = role == models::UserRole::Admin;

  models::UserProfile profile;
  profile.id = isAdmin ? "00000000-0000-0000-0000-000000000001"
                       : "00000000-0000-0000-0000-000000000002";
  profile.email = isAdmin ? "admin@devdialogue.io" : "student@devdialogue.io";
  profile.fullName = isAdmin ? "Yaroslav (Admin)" : "Candidate User";
  profile.role = role;
  profile.createdAt = "";

  m_user = profile;
  m_isMockSession = true;

  // Demo sessions never talk to the API: everything runs through MockAiEngine
  // and the in-memory question bank, exactly like the web demo mode.
  if (m_config)
    m_config->setOfflineModeEnabled(true);

  persistProfile();
  emit userChanged();
}

void AuthService::logout() {
  m_user.reset();
  m_isMockSession = false;

  if (m_apiClient)
    m_apiClient->clearBearerToken();
  if (m_config)
    m_config->setOfflineModeEnabled(false);

  clearPersistedProfile();
  emit userChanged();
}

void AuthService::persistProfile() const {
  if (!m_storage || !m_user)
    return;

  const nlohmann::json json = profileToJson(*m_user, m_isMockSession);
  m_storage->writeCredential("user_profile", json.dump());
}

void AuthService::clearPersistedProfile() {
  if (m_storage)
    m_storage->deleteCredential("user_profile");
}

} // namespace devdialogue::services
