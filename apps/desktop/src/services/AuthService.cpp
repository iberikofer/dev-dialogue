#include "services/AuthService.hpp"
#include "network/NetworkEndpoints.hpp"
#include "core/Constants.hpp"

#include <QUrl>
#include <QNetworkRequest>
#include <nlohmann/json.hpp>

namespace devdialogue::services {

AuthService::AuthService(
    std::shared_ptr<network::ApiClient> apiClient,
    std::shared_ptr<ISecureStorage> storage,
    std::shared_ptr<core::AppConfig> config,
    QObject* parent
) : QObject(parent),
    m_apiClient(std::move(apiClient)),
    m_storage(std::move(storage)),
    m_config(std::move(config)) {

    if (m_apiClient) {
        connect(m_apiClient.get(), &network::ApiClient::unauthorizedDetected,
                this, &AuthService::handleUnauthorizedDetected);
    }
}

void AuthService::loginWithEmail(
    const QString& email,
    const QString& password,
    std::function<void(bool success, const QString& error)> callback
) {
    QString supabaseUrl = QString::fromStdString(m_config->supabaseUrl());
    if (supabaseUrl.endsWith('/')) {
        supabaseUrl.chop(1);
    }
    QUrl url(supabaseUrl + endpoints::supabase::PasswordToken);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("apikey", QByteArray::fromStdString(m_config->supabaseAnonKey()));

    nlohmann::json body = {
        {"email", email.toStdString()},
        {"password", password.toStdString()}
    };
    QByteArray payload = QByteArray::fromStdString(body.dump());

    m_apiClient->executeRawRequest(req, "POST", payload, core::constants::StandardTimeoutMs,
        [this, callback](int statusCode, const QByteArray& data, const QString& netErr) {
            if (!netErr.isEmpty() && statusCode == 0) {
                if (callback) callback(false, netErr);
                emit authErrorOccurred(netErr);
                return;
            }

            try {
                auto j = nlohmann::json::parse(data.toStdString());
                if (statusCode >= 200 && statusCode < 300 && j.contains("access_token")) {
                    std::string access = j.at("access_token").get<std::string>();
                    std::string refresh = j.value("refresh_token", "");

                    m_apiClient->setTokens(access, refresh);

                    // Fetch user profile from ASP.NET backend
                    fetchCurrentUserProfile([callback](bool ok) {
                        if (callback) callback(ok, ok ? "" : "Failed to load user profile");
                    });
                    return;
                }

                auto err = models::ApiError::fromJson(j, statusCode);
                QString errStr = QString::fromStdString(err.message);
                if (callback) callback(false, errStr);
                emit authErrorOccurred(errStr);
            } catch (const std::exception& e) {
                QString errStr = QString("Failed to parse login response: %1").arg(e.what());
                if (callback) callback(false, errStr);
                emit authErrorOccurred(errStr);
            }
        });
}

void AuthService::registerUser(
    const QString& email,
    const QString& password,
    const QString& fullName,
    std::function<void(bool success, bool confirmationRequired, const QString& error)> callback
) {
    QString supabaseUrl = QString::fromStdString(m_config->supabaseUrl());
    if (supabaseUrl.endsWith('/')) {
        supabaseUrl.chop(1);
    }
    QUrl url(supabaseUrl + endpoints::supabase::Signup);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("apikey", QByteArray::fromStdString(m_config->supabaseAnonKey()));

    nlohmann::json body = {
        {"email", email.toStdString()},
        {"password", password.toStdString()},
        {"data", {
            {"full_name", fullName.toStdString()}
        }}
    };
    QByteArray payload = QByteArray::fromStdString(body.dump());

    m_apiClient->executeRawRequest(req, "POST", payload, core::constants::StandardTimeoutMs,
        [this, email, callback](int statusCode, const QByteArray& data, const QString& netErr) {
            if (!netErr.isEmpty() && statusCode == 0) {
                if (callback) callback(false, false, netErr);
                emit authErrorOccurred(netErr);
                return;
            }

            try {
                auto j = nlohmann::json::parse(data.toStdString());
                if (statusCode >= 200 && statusCode < 300) {
                    // Check if a session was created or if email confirmation is required
                    if (j.contains("access_token") && !j["access_token"].is_null()) {
                        std::string access = j.at("access_token").get<std::string>();
                        std::string refresh = j.value("refresh_token", "");
                        m_apiClient->setTokens(access, refresh);

                        fetchCurrentUserProfile([callback](bool ok) {
                            if (callback) callback(ok, false, ok ? "" : "Failed to load user profile");
                        });
                    } else {
                        // Email confirmation required (no session returned)
                        emit emailConfirmationRequired(email);
                        if (callback) callback(true, true, "");
                    }
                    return;
                }

                auto err = models::ApiError::fromJson(j, statusCode);
                QString errStr = QString::fromStdString(err.message);
                if (callback) callback(false, false, errStr);
                emit authErrorOccurred(errStr);
            } catch (const std::exception& e) {
                QString errStr = QString("Failed to parse registration response: %1").arg(e.what());
                if (callback) callback(false, false, errStr);
                emit authErrorOccurred(errStr);
            }
        });
}

void AuthService::refreshToken(std::function<void(bool success, const QString& error)> callback) {
    if (m_isRefreshing) {
        if (callback) callback(false, "Refresh already in flight");
        return;
    }

    std::string currentRefreshToken = m_apiClient->refreshToken();
    if (currentRefreshToken.empty()) {
        models::ApiError err{401, "No refresh token available", "", false};
        m_apiClient->onRefreshFailed(err);
        if (callback) callback(false, "No refresh token available");
        return;
    }

    m_isRefreshing = true;

    QString supabaseUrl = QString::fromStdString(m_config->supabaseUrl());
    if (supabaseUrl.endsWith('/')) {
        supabaseUrl.chop(1);
    }
    QUrl url(supabaseUrl + endpoints::supabase::RefreshToken);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("apikey", QByteArray::fromStdString(m_config->supabaseAnonKey()));

    nlohmann::json body = {
        {"refresh_token", currentRefreshToken}
    };
    QByteArray payload = QByteArray::fromStdString(body.dump());

    m_apiClient->executeRawRequest(req, "POST", payload, core::constants::StandardTimeoutMs,
        [this, callback](int statusCode, const QByteArray& data, const QString& netErr) {
            m_isRefreshing = false;

            if (!netErr.isEmpty() && statusCode == 0) {
                models::ApiError err{0, netErr.toStdString(), "", true};
                m_apiClient->onRefreshFailed(err);
                if (callback) callback(false, netErr);
                return;
            }

            try {
                auto j = nlohmann::json::parse(data.toStdString());
                if (statusCode >= 200 && statusCode < 300 && j.contains("access_token")) {
                    std::string newAccess = j.at("access_token").get<std::string>();
                    std::string newRefresh = j.value("refresh_token", "");

                    m_apiClient->onRefreshSucceeded(newAccess, newRefresh);
                    if (callback) callback(true, "");
                    return;
                }

                auto err = models::ApiError::fromJson(j, statusCode);
                m_apiClient->onRefreshFailed(err);
                if (callback) callback(false, QString::fromStdString(err.message));
            } catch (const std::exception& e) {
                models::ApiError err{statusCode, e.what(), data.toStdString(), false};
                m_apiClient->onRefreshFailed(err);
                if (callback) callback(false, e.what());
            }
        });
}

void AuthService::restoreSession(std::function<void(bool success)> callback) {
    if (m_apiClient->accessToken().empty()) {
        if (callback) callback(false);
        return;
    }

    fetchCurrentUserProfile([this, callback](bool ok) {
        if (ok) {
            if (callback) callback(true);
        } else {
            // Access token might be expired, attempt refresh
            refreshToken([callback](bool refreshOk, const QString&) {
                if (callback) callback(refreshOk);
            });
        }
    });
}

void AuthService::fetchCurrentUserProfile(std::function<void(bool success)> callback) {
    m_apiClient->get<models::AuthMeResponse>(endpoints::api::AuthMe,
        [this, callback](const std::variant<models::AuthMeResponse, models::ApiError>& res) {
            if (std::holds_alternative<models::AuthMeResponse>(res)) {
                const auto& me = std::get<models::AuthMeResponse>(res);
                models::UserProfile profile;
                profile.id = me.id;
                profile.email = me.email;
                profile.fullName = me.fullName;
                profile.avatarUrl = me.avatarUrl;
                profile.role = me.role;

                m_currentUser = profile;
                emit userChanged();
                if (callback) callback(true);
            } else {
                if (callback) callback(false);
            }
        });
}

void AuthService::mockLogin(models::UserRole role) {
    models::UserRole effectiveRole = role;

#ifdef NDEBUG
    // In Release mode: never allow admin via mock
    effectiveRole = models::UserRole::User;
#else
    // In Debug mode: offline mode never gives admin
    if (m_config && m_config->isOfflineModeEnabled() && effectiveRole == models::UserRole::Admin) {
        effectiveRole = models::UserRole::User;
    }
#endif

    models::UserProfile mockUser;
    mockUser.id = "mock-user-123";
    mockUser.email = (effectiveRole == models::UserRole::Admin) ? "admin@devdialogue.local" : "candidate@devdialogue.local";
    mockUser.fullName = (effectiveRole == models::UserRole::Admin) ? "Mock Administrator" : "Mock Candidate";
    mockUser.role = effectiveRole;
    mockUser.dailyInterviewsCount = 0;
    mockUser.createdAt = "2026-01-01T00:00:00Z";

    m_currentUser = mockUser;
    emit userChanged();
}

void AuthService::logout() {
    m_currentUser = std::nullopt;
    if (m_apiClient) {
        m_apiClient->clearTokens();
    }
    emit userChanged();
}

void AuthService::handleUnauthorizedDetected() {
    if (!m_apiClient->refreshToken().empty() && !m_isRefreshing) {
        refreshToken([this](bool ok, const QString&) {
            if (!ok) {
                logout();
                emit sessionExpired();
            }
        });
    } else {
        logout();
        emit sessionExpired();
    }
}

} // namespace devdialogue::services
