#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <queue>
#include <mutex>

#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "models/ApiContracts.hpp"
#include "services/ISecureStorage.hpp"

class ApiClientTest;

namespace devdialogue::services {
class AuthService;
}

namespace devdialogue::network {

template<typename T>
using ApiCallback = std::function<void(const std::variant<T, models::ApiError>&)>;

class ApiClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineStatusChanged)
    Q_PROPERTY(bool isWakingUp READ isWakingUp NOTIFY wakingUpChanged)
    Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorOccurred)

    friend class devdialogue::services::AuthService;
    friend class ::ApiClientTest;

public:
    explicit ApiClient(
        std::shared_ptr<core::AppConfig> config,
        std::shared_ptr<services::ISecureStorage> storage,
        QObject* parent = nullptr
    );
    ~ApiClient() override = default;

    bool isOnline() const { return m_isOnline; }
    bool isWakingUp() const { return m_isWakingUp; }
    QString lastErrorMessage() const { return m_lastError; }

    std::string accessToken() const { return m_accessToken; }
    std::string refreshToken() const { return m_refreshToken; }
    void setTokens(const std::string& accessToken, const std::string& refreshToken);
    void clearTokens();

    void performHealthCheck();

    // Template REST Operations
    template<typename TReq, typename TResp>
    void post(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback);

    template<typename TResp>
    void get(const QString& endpoint, ApiCallback<TResp> callback);

    template<typename TReq, typename TResp>
    void put(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback);

    void del(const QString& endpoint, std::function<void(bool success, const models::ApiError&)> callback);

    // Direct HTTP Request (for Supabase Auth & raw endpoints)
    void executeRawRequest(
        const QNetworkRequest& request,
        const QByteArray& verb,
        const QByteArray& data,
        int timeoutMs,
        std::function<void(int statusCode, const QByteArray& responseData, const QString& networkError)> callback
    );

signals:
    void onlineStatusChanged(bool isOnline);
    void wakingUpChanged(bool isWakingUp);
    void errorOccurred(const QString& message);
    void unauthorizedDetected(); // Emitted when 401 occurs to trigger refresh

private:
    // Refresh Queue Management (Called by AuthService or friend test)
    void onRefreshSucceeded(const std::string& newAccessToken, const std::string& newRefreshToken);
    void onRefreshFailed(const models::ApiError& error);
    struct PendingRequest {
        QNetworkRequest request;
        QByteArray verb;
        QByteArray data;
        std::function<void(int, const QByteArray&, const models::ApiError*)> handler;
        int retryCount{0};
    };

    std::shared_ptr<core::AppConfig> m_config;
    std::shared_ptr<services::ISecureStorage> m_storage;
    QNetworkAccessManager m_networkManager;

    std::string m_accessToken;
    std::string m_refreshToken;

    bool m_isOnline{true};
    bool m_isWakingUp{false};
    bool m_isFirstRequest{true};
    bool m_isRefreshing{false};
    QString m_lastError;

    std::queue<PendingRequest> m_refreshWaitQueue;
    std::mutex m_queueMutex;

    QNetworkRequest createApiRequest(const QString& endpoint);
    void setWakingUp(bool wakingUp);
    void sendWithTimeoutAndRetry(
        QNetworkRequest request,
        const QByteArray& verb,
        const QByteArray& data,
        int timeoutMs,
        int maxRetries,
        std::function<void(int statusCode, const QByteArray& response, const models::ApiError* err)> handler
    );
};

// ==========================================
// Template Implementations (Header-inlined)
// ==========================================

template<typename TReq, typename TResp>
void ApiClient::post(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback) {
    nlohmann::json j = body;
    QByteArray payload = QByteArray::fromStdString(j.dump());
    QNetworkRequest req = createApiRequest(endpoint);

    int timeoutMs = m_isFirstRequest ? core::constants::ColdStartTimeoutMs : core::constants::StandardTimeoutMs;
    m_isFirstRequest = false;

    // POST is non-idempotent: 0 retries on connection error, but replay on 401 refresh is handled separately
    sendWithTimeoutAndRetry(req, "POST", payload, timeoutMs, 0, [callback](int statusCode, const QByteArray& data, const models::ApiError* err) {
        if (err) {
            callback(*err);
            return;
        }

        try {
            auto jsonDoc = nlohmann::json::parse(data.toStdString());
            TResp resp = jsonDoc.get<TResp>();
            callback(resp);
        } catch (const std::exception& e) {
            models::ApiError parseErr;
            parseErr.statusCode = statusCode;
            parseErr.message = std::string("JSON Deserialization failed: ") + e.what();
            parseErr.rawPayload = data.toStdString();
            callback(parseErr);
        }
    });
}

template<typename TResp>
void ApiClient::get(const QString& endpoint, ApiCallback<TResp> callback) {
    QNetworkRequest req = createApiRequest(endpoint);

    int timeoutMs = m_isFirstRequest ? core::constants::ColdStartTimeoutMs : core::constants::StandardTimeoutMs;
    m_isFirstRequest = false;

    // GET is idempotent: up to MaxGetRetries allowed
    sendWithTimeoutAndRetry(req, "GET", QByteArray(), timeoutMs, core::constants::MaxGetRetries, [callback](int statusCode, const QByteArray& data, const models::ApiError* err) {
        if (err) {
            callback(*err);
            return;
        }

        try {
            auto jsonDoc = nlohmann::json::parse(data.toStdString());
            TResp resp = jsonDoc.get<TResp>();
            callback(resp);
        } catch (const std::exception& e) {
            models::ApiError parseErr;
            parseErr.statusCode = statusCode;
            parseErr.message = std::string("JSON Deserialization failed: ") + e.what();
            parseErr.rawPayload = data.toStdString();
            callback(parseErr);
        }
    });
}

template<typename TReq, typename TResp>
void ApiClient::put(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback) {
    nlohmann::json j = body;
    QByteArray payload = QByteArray::fromStdString(j.dump());
    QNetworkRequest req = createApiRequest(endpoint);

    int timeoutMs = core::constants::StandardTimeoutMs;

    sendWithTimeoutAndRetry(req, "PUT", payload, timeoutMs, 0, [callback](int statusCode, const QByteArray& data, const models::ApiError* err) {
        if (err) {
            callback(*err);
            return;
        }

        try {
            auto jsonDoc = nlohmann::json::parse(data.toStdString());
            TResp resp = jsonDoc.get<TResp>();
            callback(resp);
        } catch (const std::exception& e) {
            models::ApiError parseErr;
            parseErr.statusCode = statusCode;
            parseErr.message = std::string("JSON Deserialization failed: ") + e.what();
            parseErr.rawPayload = data.toStdString();
            callback(parseErr);
        }
    });
}

} // namespace devdialogue::network
