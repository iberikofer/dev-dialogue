#include "network/ApiClient.hpp"
#include "network/NetworkEndpoints.hpp"
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace devdialogue::network {

ApiClient::ApiClient(
    std::shared_ptr<core::AppConfig> config,
    std::shared_ptr<services::ISecureStorage> storage,
    QObject* parent
) : QObject(parent),
    m_config(std::move(config)),
    m_storage(std::move(storage)),
    m_networkManager(this) {

    if (m_storage) {
        m_accessToken = m_storage->readCredential(core::constants::StorageKeyAccessToken).value_or("");
        m_refreshToken = m_storage->readCredential(core::constants::StorageKeyRefreshToken).value_or("");
    }

    auto* healthTimer = new QTimer(this);
    connect(healthTimer, &QTimer::timeout, this, &ApiClient::performHealthCheck);
    healthTimer->start(core::constants::HealthCheckIntervalMs);
}

void ApiClient::setTokens(const std::string& accessToken, const std::string& refreshToken) {
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;

    if (m_storage) {
        m_storage->writeCredential(core::constants::StorageKeyAccessToken, accessToken);
        m_storage->writeCredential(core::constants::StorageKeyRefreshToken, refreshToken);
    }
}

void ApiClient::clearTokens() {
    m_accessToken.clear();
    m_refreshToken.clear();

    if (m_storage) {
        m_storage->deleteCredential(core::constants::StorageKeyAccessToken);
        m_storage->deleteCredential(core::constants::StorageKeyRefreshToken);
    }
}

QNetworkRequest ApiClient::createApiRequest(const QString& endpoint) {
    QString baseUrl = QString::fromStdString(m_config->apiBaseUrl());
    if (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }
    QString path = endpoint.startsWith('/') ? endpoint : "/" + endpoint;

    QNetworkRequest req{QUrl(baseUrl + path)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");

    if (!m_accessToken.empty()) {
        const QString bearer = QString("Bearer %1").arg(QString::fromStdString(m_accessToken));
        req.setRawHeader("Authorization", bearer.toUtf8());
    }

    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    return req;
}

void ApiClient::setWakingUp(bool wakingUp) {
    if (m_isWakingUp != wakingUp) {
        m_isWakingUp = wakingUp;
        emit wakingUpChanged(m_isWakingUp);
    }
}

void ApiClient::performHealthCheck() {
    QNetworkRequest req = createApiRequest(endpoints::api::HealthPing);
    QNetworkReply* reply = m_networkManager.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        bool newStatus = (reply->error() == QNetworkReply::NoError);
        if (newStatus != m_isOnline) {
            m_isOnline = newStatus;
            emit onlineStatusChanged(m_isOnline);
        }
    });
}

void ApiClient::del(const QString& endpoint, std::function<void(bool success, const models::ApiError&)> callback) {
    QNetworkRequest req = createApiRequest(endpoint);
    int timeoutMs = core::constants::StandardTimeoutMs;

    sendWithTimeoutAndRetry(req, "DELETE", QByteArray(), timeoutMs, 0, [callback](int statusCode, const QByteArray& data, const models::ApiError* err) {
        if (err) {
            callback(false, *err);
            return;
        }

        if (statusCode >= 200 && statusCode < 300) {
            callback(true, {});
        } else {
            models::ApiError delErr;
            delErr.statusCode = statusCode;
            delErr.message = data.toStdString();
            callback(false, delErr);
        }
    });
}

void ApiClient::executeRawRequest(
    const QNetworkRequest& request,
    const QByteArray& verb,
    const QByteArray& data,
    int timeoutMs,
    std::function<void(int statusCode, const QByteArray& responseData, const QString& networkError)> callback
) {
    QNetworkReply* reply = nullptr;
    if (verb.toUpper() == "GET") {
        reply = m_networkManager.get(request);
    } else if (verb.toUpper() == "POST") {
        reply = m_networkManager.post(request, data);
    } else if (verb.toUpper() == "PUT") {
        reply = m_networkManager.put(request, data);
    } else if (verb.toUpper() == "DELETE") {
        reply = m_networkManager.deleteResource(request);
    } else {
        reply = m_networkManager.sendCustomRequest(request, verb, data);
    }

    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        reply->abort();
    });
    timeoutTimer->start(timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [reply, timeoutTimer, callback]() {
        timeoutTimer->stop();
        reply->deleteLater();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = reply->readAll();
        QString netErr;
        if (reply->error() != QNetworkReply::NoError) {
            netErr = reply->errorString();
        }
        callback(statusCode, responseData, netErr);
    });
}

void ApiClient::sendWithTimeoutAndRetry(
    QNetworkRequest request,
    const QByteArray& verb,
    const QByteArray& data,
    int timeoutMs,
    int maxRetries,
    std::function<void(int statusCode, const QByteArray& response, const models::ApiError* err)> handler
) {
    QNetworkReply* reply = nullptr;
    if (verb.toUpper() == "GET") {
        reply = m_networkManager.get(request);
    } else if (verb.toUpper() == "POST") {
        reply = m_networkManager.post(request, data);
    } else if (verb.toUpper() == "PUT") {
        reply = m_networkManager.put(request, data);
    } else if (verb.toUpper() == "DELETE") {
        reply = m_networkManager.deleteResource(request);
    } else {
        reply = m_networkManager.sendCustomRequest(request, verb, data);
    }

    auto* wakingTimer = new QTimer(reply);
    wakingTimer->setSingleShot(true);
    connect(wakingTimer, &QTimer::timeout, this, [this]() {
        setWakingUp(true);
    });
    wakingTimer->start(core::constants::WakingUpThresholdMs);

    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        reply->abort();
    });
    timeoutTimer->start(timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [this, reply, wakingTimer, timeoutTimer, request, verb, data, timeoutMs, maxRetries, handler]() {
        wakingTimer->stop();
        timeoutTimer->stop();
        setWakingUp(false);
        reply->deleteLater();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = reply->readAll();

        // 1. Handle 401 Unauthorized -> Refresh Token Flow
        if (statusCode == 401) {
            PendingRequest pending{request, verb, data, handler, 0};
            bool shouldTriggerRefresh = false;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_refreshWaitQueue.push(pending);
                if (!m_isRefreshing) {
                    m_isRefreshing = true;
                    shouldTriggerRefresh = true;
                }
            }

            if (shouldTriggerRefresh) {
                emit unauthorizedDetected();
            }
            return;
        }

        // 2. Handle Network Down / Timeout / Retries
        if (reply->error() != QNetworkReply::NoError && statusCode == 0) {
            // GET is idempotent: retry up to maxRetries
            if (verb.toUpper() == "GET" && maxRetries > 0) {
                QTimer::singleShot(1000, this, [this, request, verb, data, timeoutMs, maxRetries = maxRetries - 1, handler]() {
                    sendWithTimeoutAndRetry(request, verb, data, timeoutMs, maxRetries, handler);
                });
                return;
            }

            models::ApiError netErr;
            netErr.statusCode = 0;
            netErr.isNetworkDown = true;
            netErr.message = reply->errorString().toStdString();
            m_lastError = reply->errorString();
            emit errorOccurred(m_lastError);

            if (m_isOnline) {
                m_isOnline = false;
                emit onlineStatusChanged(false);
            }

            handler(0, responseData, &netErr);
            return;
        }

        // 3. Mark online on receiving any valid HTTP response
        if (!m_isOnline) {
            m_isOnline = true;
            emit onlineStatusChanged(true);
        }

        // 4. Handle HTTP Error (4xx, 5xx)
        if (statusCode >= 400) {
            models::ApiError apiErr;
            try {
                auto j = nlohmann::json::parse(responseData.toStdString(), nullptr, false);
                if (!j.is_discarded()) {
                    apiErr = models::ApiError::fromJson(j, statusCode);
                } else {
                    apiErr.statusCode = statusCode;
                    apiErr.message = responseData.isEmpty() ? reply->errorString().toStdString() : responseData.toStdString();
                    apiErr.rawPayload = responseData.toStdString();
                }
            } catch (...) {
                apiErr.statusCode = statusCode;
                apiErr.message = responseData.toStdString();
                apiErr.rawPayload = responseData.toStdString();
            }

            m_lastError = QString::fromStdString(apiErr.message);
            emit errorOccurred(m_lastError);
            handler(statusCode, responseData, &apiErr);
            return;
        }

        // 5. Successful 2xx response
        handler(statusCode, responseData, nullptr);
    });
}

void ApiClient::onRefreshSucceeded(const std::string& newAccessToken, const std::string& newRefreshToken) {
    setTokens(newAccessToken, newRefreshToken);

    std::queue<PendingRequest> queuedRequests;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::swap(queuedRequests, m_refreshWaitQueue);
        m_isRefreshing = false;
    }

    while (!queuedRequests.empty()) {
        auto item = queuedRequests.front();
        queuedRequests.pop();

        // Update Authorization header with the fresh token
        const QString bearer = QString("Bearer %1").arg(QString::fromStdString(newAccessToken));
        item.request.setRawHeader("Authorization", bearer.toUtf8());

        // Replay request (allowed for POST, GET, PUT, DELETE)
        sendWithTimeoutAndRetry(item.request, item.verb, item.data, core::constants::StandardTimeoutMs, 0, item.handler);
    }
}

void ApiClient::onRefreshFailed(const models::ApiError& error) {
    clearTokens();

    std::queue<PendingRequest> queuedRequests;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::swap(queuedRequests, m_refreshWaitQueue);
        m_isRefreshing = false;
    }

    while (!queuedRequests.empty()) {
        auto item = queuedRequests.front();
        queuedRequests.pop();

        item.handler(error.statusCode, QByteArray(), &error);
    }

    m_lastError = QString::fromStdString(error.message);
    emit errorOccurred(m_lastError);
    emit unauthorizedDetected();
}

} // namespace devdialogue::network
