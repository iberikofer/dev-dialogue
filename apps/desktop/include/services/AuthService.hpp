#pragma once

#include <QObject>
#include <memory>
#include <optional>
#include <string>

#include "models/ApiContracts.hpp"

namespace devdialogue::core {
class AppConfig;
}

namespace devdialogue::network {
class ApiClient;
}

namespace devdialogue::services {

class SecureStorage;

/// Owns the authenticated session: profile, bearer token lifetime, and the
/// demo/mock mode used when the API or an identity provider is unavailable.
class AuthService : public QObject {
  Q_OBJECT

public:
  AuthService(std::shared_ptr<network::ApiClient> apiClient,
              std::shared_ptr<SecureStorage> storage,
              std::shared_ptr<core::AppConfig> config,
              QObject *parent = nullptr);

  std::optional<models::UserProfile> currentUser() const;
  bool isAuthenticated() const;
  bool isMockSession() const;

  void restoreSession();
  void applyAuthMe(const models::AuthMeResponse &response);
  void setSession(const models::UserProfile &profile,
                  const std::string &token);
  void mockLogin(models::UserRole role);
  void logout();

signals:
  void userChanged();

private:
  void persistProfile() const;
  void clearPersistedProfile();

  std::shared_ptr<network::ApiClient> m_apiClient;
  std::shared_ptr<SecureStorage> m_storage;
  std::shared_ptr<core::AppConfig> m_config;
  std::optional<models::UserProfile> m_user;
  bool m_isMockSession{false};
};

} // namespace devdialogue::services
