#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <optional>
#include <string>
#include <functional>

#include "core/AppConfig.hpp"
#include "models/ApiContracts.hpp"
#include "services/ISecureStorage.hpp"
#include "network/ApiClient.hpp"

namespace devdialogue::services {

class AuthService : public QObject {
    Q_OBJECT

public:
    explicit AuthService(
        std::shared_ptr<network::ApiClient> apiClient,
        std::shared_ptr<ISecureStorage> storage,
        std::shared_ptr<core::AppConfig> config,
        QObject* parent = nullptr
    );
    ~AuthService() override = default;

    std::optional<models::UserProfile> currentUser() const { return m_currentUser; }
    bool isAuthenticated() const { return m_currentUser.has_value(); }
    bool isAdmin() const {
        return m_currentUser.has_value() && m_currentUser->role == models::UserRole::Admin;
    }

    // Core Authentication APIs
    void loginWithEmail(const QString& email, const QString& password, std::function<void(bool success, const QString& error)> callback = nullptr);
    void registerUser(const QString& email, const QString& password, const QString& fullName, std::function<void(bool success, bool confirmationRequired, const QString& error)> callback = nullptr);
    void refreshToken(std::function<void(bool success, const QString& error)> callback = nullptr);
    void restoreSession(std::function<void(bool success)> callback = nullptr);
    void mockLogin(models::UserRole role = models::UserRole::User);
    void logout();

    void fetchCurrentUserProfile(std::function<void(bool success)> callback = nullptr);

signals:
    void userChanged();
    void emailConfirmationRequired(const QString& email);
    void authErrorOccurred(const QString& errorMessage);
    void sessionExpired();

private slots:
    void handleUnauthorizedDetected();

private:
    std::shared_ptr<network::ApiClient> m_apiClient;
    std::shared_ptr<ISecureStorage> m_storage;
    std::shared_ptr<core::AppConfig> m_config;

    std::optional<models::UserProfile> m_currentUser;
    bool m_isRefreshing{false};
};

} // namespace devdialogue::services
