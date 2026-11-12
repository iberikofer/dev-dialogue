#pragma once

#include <QDate>
#include <QObject>
#include <QString>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include "models/ApiContracts.hpp"
#include "network/ApiClient.hpp"
#include "services/MockAiEngine.hpp"

namespace devdialogue::core {
class AppConfig;
}

namespace devdialogue::services {

/// Async facade over the interview REST endpoints with a transparent local
/// fallback, so every screen works in offline/demo mode.
class InterviewService : public QObject {
  Q_OBJECT

public:
  using CreateCallback = std::function<void(
      const std::variant<models::InterviewDetail, network::ApiError> &)>;
  using ListCallback = std::function<void(
      const std::variant<models::PagedResult<models::InterviewSummary>,
                         network::ApiError> &)>;
  using DetailCallback = std::function<void(
      const std::variant<models::InterviewDetail, network::ApiError> &)>;
  using SendCallback = std::function<void(
      const std::variant<models::SendMessageResponse, network::ApiError> &)>;
  using CompleteCallback = std::function<void(
      const std::variant<models::CompleteInterviewResponse,
                         network::ApiError> &)>;
  using StatsCallback = std::function<void(
      const std::variant<models::UserDashboardStats, network::ApiError> &)>;

  InterviewService(std::shared_ptr<network::ApiClient> apiClient,
                   std::shared_ptr<core::AppConfig> config,
                   QObject *parent = nullptr);

  void createInterview(models::InterviewDirection direction,
                       models::InterviewLevel level,
                       models::InterviewLanguage language,
                       CreateCallback callback);

  void listInterviews(int page, int pageSize, ListCallback callback);

  void getInterview(const QString &id, DetailCallback callback);

  void sendMessage(const QString &id, const QString &content,
                   SendCallback callback);

  void completeInterview(const QString &id, CompleteCallback callback);

  void getStats(StatsCallback callback);

  bool isOfflineMode() const;

private:
  models::InterviewDetail createLocalInterview(
      models::InterviewDirection direction, models::InterviewLevel level,
      models::InterviewLanguage language);

  models::PagedResult<models::InterviewSummary> localInterviews(int page,
                                                                int pageSize) const;

  models::UserDashboardStats localStats() const;

  void cacheDetail(const models::InterviewDetail &detail);
  std::optional<models::InterviewDetail> findCached(const QString &id) const;

  void countLocalInterviewForToday();

  std::shared_ptr<network::ApiClient> m_apiClient;
  std::shared_ptr<core::AppConfig> m_config;
  MockAiEngine m_mockEngine;
  std::map<std::string, models::InterviewDetail> m_sessions;
  QDate m_localDay;
  int m_localDailyCount{0};
};

} // namespace devdialogue::services
