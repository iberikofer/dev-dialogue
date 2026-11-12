#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

#include "core/AppConfig.hpp"
#include "services/SecureStorage.hpp"

namespace devdialogue::network {

struct ApiError {
  int statusCode{0};
  std::string message;
  std::string rawPayload;
  bool isNetworkDown{false};
};

template <typename T>
using ApiCallback = std::function<void(const std::variant<T, ApiError> &)>;

class ApiClient : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineStatusChanged)
  Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY
                 errorOccurred)

public:
  explicit ApiClient(std::shared_ptr<core::AppConfig> config,
                     std::shared_ptr<services::SecureStorage> storage,
                     QObject *parent = nullptr);

  bool isOnline() const { return m_isOnline; }
  QString lastErrorMessage() const { return m_lastError; }

  void setBearerToken(const std::string &token);
  void clearBearerToken();

  Q_INVOKABLE void performHealthCheck();

  template <typename TResp>
  void get(const QString &endpoint, ApiCallback<TResp> callback) {
    send("GET", endpoint, std::nullopt,
         makeHandler<TResp>(std::move(callback)));
  }

  template <typename TReq, typename TResp>
  void post(const QString &endpoint, const TReq &body,
            ApiCallback<TResp> callback) {
    send("POST", endpoint, std::optional<nlohmann::json>(body),
         makeHandler<TResp>(std::move(callback)));
  }

  template <typename TReq, typename TResp>
  void put(const QString &endpoint, const TReq &body,
           ApiCallback<TResp> callback) {
    send("PUT", endpoint, std::optional<nlohmann::json>(body),
         makeHandler<TResp>(std::move(callback)));
  }

  void del(const QString &endpoint,
           std::function<void(bool success, const ApiError &)> callback);

signals:
  void onlineStatusChanged(bool isOnline);
  void errorOccurred(const QString &message);
  void unauthorizedDetected();

private:
  using JsonHandler =
      std::function<void(const nlohmann::json &, const ApiError *)>;

  template <typename TResp>
  static JsonHandler makeHandler(ApiCallback<TResp> callback) {
    return [callback = std::move(callback)](const nlohmann::json &json,
                                            const ApiError *error) mutable {
      if (error != nullptr) {
        callback(*error);
        return;
      }
      try {
        callback(json.get<TResp>());
      } catch (const std::exception &ex) {
        ApiError parseError;
        parseError.message = ex.what();
        callback(parseError);
      }
    };
  }

  void send(const QByteArray &verb, const QString &endpoint,
            const std::optional<nlohmann::json> &body, JsonHandler handler);

  QNetworkRequest createRequest(const QString &endpoint);

  std::shared_ptr<core::AppConfig> m_config;
  std::shared_ptr<services::SecureStorage> m_storage;
  QNetworkAccessManager m_networkManager;
  std::string m_cachedToken;
  bool m_isOnline{true};
  QString m_lastError;
};

} // namespace devdialogue::network
