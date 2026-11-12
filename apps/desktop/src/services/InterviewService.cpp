#include "services/InterviewService.hpp"

#include <QDateTime>
#include <QUuid>

#include <algorithm>
#include <utility>

#include "core/AppConfig.hpp"

namespace devdialogue::services {

namespace {

QString nowIso() {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString newLocalId() {
  return QStringLiteral("local-") +
         QUuid::createUuid().toString(QUuid::WithoutBraces);
}

models::InterviewMessage makeMessage(const std::string &role,
                                     const std::string &content) {
  models::InterviewMessage message;
  message.role = role;
  message.content = content;
  message.timestamp = nowIso().toStdString();
  return message;
}

} // namespace

InterviewService::InterviewService(std::shared_ptr<network::ApiClient> apiClient,
                                   std::shared_ptr<core::AppConfig> config,
                                   QObject *parent)
    : QObject(parent), m_apiClient(std::move(apiClient)),
      m_config(std::move(config)), m_localDay(QDate::currentDate()) {}

bool InterviewService::isOfflineMode() const {
  if (m_config && m_config->isOfflineModeEnabled())
    return true;
  return m_apiClient && !m_apiClient->isOnline();
}

void InterviewService::countLocalInterviewForToday() {
  const QDate today = QDate::currentDate();
  if (m_localDay != today) {
    m_localDay = today;
    m_localDailyCount = 0;
  }
  ++m_localDailyCount;
}

void InterviewService::cacheDetail(const models::InterviewDetail &detail) {
  m_sessions[detail.id] = detail;
}

std::optional<models::InterviewDetail>
InterviewService::findCached(const QString &id) const {
  auto it = m_sessions.find(id.toStdString());
  if (it == m_sessions.end())
    return std::nullopt;
  return it->second;
}

models::InterviewDetail InterviewService::createLocalInterview(
    models::InterviewDirection direction, models::InterviewLevel level,
    models::InterviewLanguage language) {
  models::InterviewDetail detail;
  detail.id = newLocalId().toStdString();
  detail.userId = "local-user";
  detail.direction = direction;
  detail.level = level;
  detail.language = language;
  detail.stage = models::InterviewStage::HardSkills;
  detail.status = models::InterviewStatus::Active;
  detail.questionCount = 1;
  detail.createdAt = nowIso().toStdString();
  detail.updatedAt = detail.createdAt;

  detail.transcript.push_back(makeMessage(
      "assistant", m_mockEngine.generateFirstQuestion(direction, level, language)));

  cacheDetail(detail);
  countLocalInterviewForToday();
  return detail;
}

void InterviewService::createInterview(models::InterviewDirection direction,
                                       models::InterviewLevel level,
                                       models::InterviewLanguage language,
                                       CreateCallback callback) {
  if (isOfflineMode() || !m_apiClient) {
    callback(createLocalInterview(direction, level, language));
    return;
  }

  models::CreateInterviewRequest request{direction, level, language};
  m_apiClient->post<models::CreateInterviewRequest,
                    models::CreateInterviewResponse>(
      "/api/interviews", request,
      [this, direction, level, language,
       callback = std::move(callback)](
          const std::variant<models::CreateInterviewResponse,
                             network::ApiError> &result) {
        if (const auto *response =
                std::get_if<models::CreateInterviewResponse>(&result)) {
          models::InterviewDetail detail;
          detail.id = response->interviewId;
          detail.userId = "api-user";
          detail.direction = direction;
          detail.level = level;
          detail.language = language;
          detail.stage = models::InterviewStage::HardSkills;
          detail.status = models::InterviewStatus::Active;
          detail.questionCount = 1;
          detail.createdAt = nowIso().toStdString();
          detail.updatedAt = detail.createdAt;
          detail.transcript.push_back(
              makeMessage("assistant", response->firstQuestion));

          cacheDetail(detail);
          callback(detail);
          return;
        }

        // Network or server failure: keep the UX alive with a local session.
        callback(createLocalInterview(direction, level, language));
      });
}

models::PagedResult<models::InterviewSummary>
InterviewService::localInterviews(int page, int pageSize) const {
  std::vector<models::InterviewSummary> items;
  items.reserve(m_sessions.size());

  for (const auto &[id, detail] : m_sessions) {
    (void)id;
    items.push_back(static_cast<const models::InterviewSummary &>(detail));
  }

  std::sort(items.begin(), items.end(),
            [](const models::InterviewSummary &a,
               const models::InterviewSummary &b) {
              return a.createdAt > b.createdAt;
            });

  const int safePage = std::max(page, 1);
  const int safePageSize = std::max(pageSize, 1);
  const int total = static_cast<int>(items.size());
  const int start = std::min((safePage - 1) * safePageSize, total);
  const int end = std::min(start + safePageSize, total);

  models::PagedResult<models::InterviewSummary> result;
  result.items.assign(items.begin() + start, items.begin() + end);
  result.page = safePage;
  result.pageSize = safePageSize;
  result.totalCount = total;
  result.totalPages = (total + safePageSize - 1) / safePageSize;
  return result;
}

void InterviewService::listInterviews(int page, int pageSize,
                                      ListCallback callback) {
  if (isOfflineMode() || !m_apiClient) {
    callback(localInterviews(page, pageSize));
    return;
  }

  const QString endpoint =
      QStringLiteral("/api/interviews?page=%1&pageSize=%2")
          .arg(page)
          .arg(pageSize);

  m_apiClient->get<models::PagedResult<models::InterviewSummary>>(
      endpoint,
      [this, page, pageSize,
       callback = std::move(callback)](
          const std::variant<models::PagedResult<models::InterviewSummary>,
                             network::ApiError> &result) {
        if (std::holds_alternative<
                models::PagedResult<models::InterviewSummary>>(result)) {
          callback(std::get<models::PagedResult<models::InterviewSummary>>(result));
          return;
        }
        callback(localInterviews(page, pageSize));
      });
}

void InterviewService::getInterview(const QString &id, DetailCallback callback) {
  if (const auto cached = findCached(id)) {
    callback(*cached);
    return;
  }

  if (isOfflineMode() || !m_apiClient) {
    network::ApiError error;
    error.message = "Interview is not available offline.";
    callback(error);
    return;
  }

  m_apiClient->get<models::InterviewDetail>(
      QStringLiteral("/api/interviews/%1").arg(id),
      [this, callback = std::move(callback)](
          const std::variant<models::InterviewDetail, network::ApiError> &result) {
        if (const auto *detail = std::get_if<models::InterviewDetail>(&result)) {
          cacheDetail(*detail);
          callback(*detail);
          return;
        }
        callback(result);
      });
}

void InterviewService::sendMessage(const QString &id, const QString &content,
                                   SendCallback callback) {
  const auto cached = findCached(id);

  // Local/demo sessions, and API sessions that dropped offline mid-interview,
  // are answered by the embedded engine.
  if (cached.has_value() && (isOfflineMode() || id.startsWith("local-") ||
                             !m_apiClient)) {
    models::InterviewDetail detail = *cached;
    detail.transcript.push_back(makeMessage("user", content.toStdString()));

    const std::string nextQuestion = m_mockEngine.generateNextQuestion(
        detail.direction, detail.level, detail.language, detail.questionCount,
        content.toStdString());
    detail.transcript.push_back(makeMessage("assistant", nextQuestion));
    detail.questionCount = static_cast<int>(
        std::count_if(detail.transcript.begin(), detail.transcript.end(),
                      [](const models::InterviewMessage &message) {
                        return message.role == "user";
                      })) +
                           1;
    detail.updatedAt = nowIso().toStdString();
    cacheDetail(detail);

    models::SendMessageResponse response;
    response.message = nextQuestion;
    response.questionCount = detail.questionCount;
    response.isCompleted = false;
    callback(response);
    return;
  }

  if (!m_apiClient) {
    network::ApiError error;
    error.message = "Interview is not available offline.";
    callback(error);
    return;
  }

  models::SendMessageRequest request{content.toStdString()};
  m_apiClient->post<models::SendMessageRequest, models::SendMessageResponse>(
      QStringLiteral("/api/interviews/%1/message").arg(id), request,
      [this, id, content, callback = std::move(callback)](
          const std::variant<models::SendMessageResponse, network::ApiError>
              &result) {
        if (const auto *response =
                std::get_if<models::SendMessageResponse>(&result)) {
          if (auto detail = findCached(id)) {
            detail->transcript.push_back(
                makeMessage("user", content.toStdString()));
            detail->transcript.push_back(
                makeMessage("assistant", response->message));
            detail->questionCount = response->questionCount;
            detail->updatedAt = nowIso().toStdString();
            cacheDetail(*detail);
          }
          callback(*response);
          return;
        }

        // Degrade gracefully: answer locally so the conversation continues.
        if (auto detail = findCached(id)) {
          models::InterviewDetail local = *detail;
          local.transcript.push_back(
              makeMessage("user", content.toStdString()));
          const std::string nextQuestion = m_mockEngine.generateNextQuestion(
              local.direction, local.level, local.language,
              local.questionCount, content.toStdString());
          local.transcript.push_back(makeMessage("assistant", nextQuestion));
          local.questionCount = static_cast<int>(
              std::count_if(local.transcript.begin(), local.transcript.end(),
                            [](const models::InterviewMessage &message) {
                              return message.role == "user";
                            })) +
                                1;
          local.updatedAt = nowIso().toStdString();
          cacheDetail(local);

          models::SendMessageResponse response;
          response.message = nextQuestion;
          response.questionCount = local.questionCount;
          response.isCompleted = false;
          callback(response);
          return;
        }

        callback(result);
      });
}

void InterviewService::completeInterview(const QString &id,
                                        CompleteCallback callback) {
  const auto cached = findCached(id);

  const auto completeLocally = [this, id, callback]() {
    auto detail = findCached(id);
    if (!detail.has_value()) {
      network::ApiError error;
      error.message = "Interview is not available offline.";
      callback(error);
      return;
    }

    models::InterviewFeedbackReport report = m_mockEngine.evaluateInterview(
        detail->direction, detail->level, detail->language, detail->transcript);

    detail->feedback = report;
    detail->score = report.score;
    detail->status = models::InterviewStatus::Completed;
    detail->updatedAt = nowIso().toStdString();
    cacheDetail(*detail);

    models::CompleteInterviewResponse response;
    response.score = report.score;
    response.feedback = report;
    callback(response);
  };

  if (cached.has_value() && (isOfflineMode() || id.startsWith("local-") ||
                             !m_apiClient)) {
    completeLocally();
    return;
  }

  if (!m_apiClient) {
    network::ApiError error;
    error.message = "Interview is not available offline.";
    callback(error);
    return;
  }

  m_apiClient->post<nlohmann::json, models::CompleteInterviewResponse>(
      QStringLiteral("/api/interviews/%1/complete").arg(id),
      nlohmann::json::object(),
      [this, id, completeLocally, callback](
          const std::variant<models::CompleteInterviewResponse,
                             network::ApiError> &result) {
        if (const auto *response =
                std::get_if<models::CompleteInterviewResponse>(&result)) {
          if (auto detail = findCached(id)) {
            detail->feedback = response->feedback;
            detail->score = response->score;
            detail->status = models::InterviewStatus::Completed;
            detail->updatedAt = nowIso().toStdString();
            cacheDetail(*detail);
          }
          callback(*response);
          return;
        }

        completeLocally();
      });
}

models::UserDashboardStats InterviewService::localStats() const {
  models::UserDashboardStats stats;
  stats.dailyLimit = 10;

  int completed = 0;
  int scoreSum = 0;
  for (const auto &[id, detail] : m_sessions) {
    (void)id;
    if (detail.status == models::InterviewStatus::Completed) {
      ++completed;
      scoreSum += detail.score.value_or(detail.feedback
                                            ? detail.feedback->score
                                            : 0);
    }
  }

  stats.totalInterviews = completed;
  stats.averageScore =
      completed > 0 ? static_cast<double>(scoreSum) / completed : 0.0;

  const bool today = (m_localDay == QDate::currentDate());
  stats.dailyInterviewsCount = today ? m_localDailyCount : 0;
  stats.remainingToday =
      std::max(0, stats.dailyLimit - stats.dailyInterviewsCount);

  return stats;
}

void InterviewService::getStats(StatsCallback callback) {
  if (isOfflineMode() || !m_apiClient) {
    callback(localStats());
    return;
  }

  m_apiClient->get<models::UserDashboardStats>(
      "/api/interviews/stats",
      [this, callback = std::move(callback)](
          const std::variant<models::UserDashboardStats, network::ApiError>
              &result) {
        if (const auto *stats =
                std::get_if<models::UserDashboardStats>(&result)) {
          callback(*stats);
          return;
        }
        callback(localStats());
      });
}

} // namespace devdialogue::services
