#include "network/ApiClient.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include <utility>

namespace devdialogue::network {

ApiClient::ApiClient(std::shared_ptr<core::AppConfig> config,
                     std::shared_ptr<services::SecureStorage> storage,
                     QObject *parent)
    : QObject(parent), m_config(std::move(config)),
      m_storage(std::move(storage)) {
  if (m_storage) {
    m_cachedToken = m_storage->readCredential("jwt_token").value_or("");
  }

  auto *healthTimer = new QTimer(this);
  connect(healthTimer, &QTimer::timeout, this, &ApiClient::performHealthCheck);
  healthTimer->start(30000);

  QTimer::singleShot(500, this, &ApiClient::performHealthCheck);
}

QNetworkRequest ApiClient::createRequest(const QString &endpoint) {
  QString base = QString::fromStdString(m_config->apiBaseUrl());
  while (base.endsWith('/'))
    base.chop(1);

  QString path = endpoint;
  if (!path.startsWith('/'))
    path.prepend('/');

  QNetworkRequest request{QUrl(base + path)};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Accept", "application/json");

  if (!m_cachedToken.empty()) {
    const QString bearer =
        QStringLiteral("Bearer ") + QString::fromStdString(m_cachedToken);
    request.setRawHeader("Authorization", bearer.toUtf8());
  }

  request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
  return request;
}

void ApiClient::setBearerToken(const std::string &token) {
  m_cachedToken = token;
  if (m_storage)
    m_storage->writeCredential("jwt_token", token);
}

void ApiClient::clearBearerToken() {
  m_cachedToken.clear();
  if (m_storage)
    m_storage->deleteCredential("jwt_token");
}

void ApiClient::send(const QByteArray &verb, const QString &endpoint,
                     const std::optional<nlohmann::json> &body,
                     JsonHandler handler) {
  const QNetworkRequest request = createRequest(endpoint);
  const QByteArray payload =
      body ? QByteArray::fromStdString(body->dump()) : QByteArray();

  QNetworkReply *reply = nullptr;
  if (verb == "GET") {
    reply = m_networkManager.get(request);
  } else if (verb == "POST") {
    reply = m_networkManager.post(request, payload);
  } else if (verb == "PUT") {
    reply = m_networkManager.put(request, payload);
  } else if (verb == "DELETE") {
    reply = m_networkManager.deleteResource(request);
  } else {
    reply = m_networkManager.sendCustomRequest(request, verb, payload);
  }

  connect(reply, &QNetworkReply::finished, this,
          [this, reply, handler = std::move(handler)]() mutable {
            reply->deleteLater();

            const QByteArray raw = reply->readAll();
            const QVariant statusAttribute =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            const int status =
                statusAttribute.isValid() ? statusAttribute.toInt() : 0;

            // Any HTTP response means the server is reachable. Only a
            // transport-level failure (no status code) means "offline".
            const bool networkDown =
                (status == 0 && reply->error() != QNetworkReply::NoError);
            const bool reachable = !networkDown;

            if (reachable != m_isOnline) {
              m_isOnline = reachable;
              emit onlineStatusChanged(m_isOnline);
            }

            if (status == 401) {
              emit unauthorizedDetected();
            }

            if (reply->error() != QNetworkReply::NoError ||
                status < 200 || status >= 300) {
              ApiError error;
              error.statusCode = status;
              error.rawPayload = raw.toStdString();
              error.isNetworkDown = networkDown;

              if (networkDown) {
                error.message = reply->errorString().toStdString();
              } else {
                try {
                  const nlohmann::json json = nlohmann::json::parse(
                      raw.toStdString().empty() ? "{}" : raw.toStdString());
                  if (json.contains("message") && json["message"].is_string())
                    error.message = json["message"].get<std::string>();
                  else if (json.contains("error") && json["error"].is_string())
                    error.message = json["error"].get<std::string>();
                  else if (json.contains("title") && json["title"].is_string())
                    error.message = json["title"].get<std::string>();
                  else if (json.contains("errors") && json["errors"].is_object()) {
                    for (auto it = json["errors"].begin();
                         it != json["errors"].end(); ++it) {
                      if (it.value().is_array() && !it.value().empty()) {
                        error.message = it.value().front().get<std::string>();
                        break;
                      }
                    }
                  }
                } catch (const std::exception &) {
                  // Non-JSON error body; keep the raw payload.
                }

                if (error.message.empty())
                  error.message = reply->errorString().toStdString();
              }

              m_lastError = QString::fromStdString(error.message);
              emit errorOccurred(m_lastError);
              handler(nlohmann::json::object(), &error);
              return;
            }

            if (raw.isEmpty()) {
              handler(nlohmann::json::object(), nullptr);
              return;
            }

            try {
              handler(nlohmann::json::parse(raw.toStdString()), nullptr);
            } catch (const std::exception &ex) {
              ApiError error;
              error.statusCode = status;
              error.rawPayload = raw.toStdString();
              error.message = ex.what();
              m_lastError = QString::fromStdString(error.message);
              emit errorOccurred(m_lastError);
              handler(nlohmann::json::object(), &error);
            }
          });
}

void ApiClient::del(
    const QString &endpoint,
    std::function<void(bool success, const ApiError &)> callback) {
  send("DELETE", endpoint, std::nullopt,
       [callback = std::move(callback)](const nlohmann::json &,
                                        const ApiError *error) {
         if (error != nullptr) {
           callback(false, *error);
           return;
         }
         callback(true, ApiError{});
       });
}

void ApiClient::performHealthCheck() {
  send("GET", "/api/health", std::nullopt,
       [](const nlohmann::json &, const ApiError *) {});
}

} // namespace devdialogue::network
