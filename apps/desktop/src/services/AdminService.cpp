#include "services/AdminService.hpp"

#include <QDateTime>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <utility>

#include "core/AppConfig.hpp"

namespace devdialogue::services {

namespace {

QString enumKey(auto value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

QString nowIso() {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

} // namespace

AdminService::AdminService(std::shared_ptr<network::ApiClient> apiClient,
                           std::shared_ptr<core::AppConfig> config,
                           QObject *parent)
    : QObject(parent), m_apiClient(std::move(apiClient)),
      m_config(std::move(config)) {
  seedLocalBank();
}

bool AdminService::isOfflineMode() const {
  if (m_config && m_config->isOfflineModeEnabled())
    return true;
  return m_apiClient && !m_apiClient->isOnline();
}

void AdminService::seedLocalBank() {
  using D = models::InterviewDirection;
  using L = models::InterviewLevel;
  using S = models::InterviewStage;

  const struct {
    D direction;
    S stage;
    L level;
    const char *topic;
    const char *text;
  } seeds[] = {
      {D::Frontend, S::HardSkills, L::Junior, "JavaScript",
       "How does the event loop handle microtasks versus macrotasks?"},
      {D::Frontend, S::HardSkills, L::Middle, "React",
       "How would you design a shared state layer for a large React app?"},
      {D::Frontend, S::SoftSkills, L::Junior, "Collaboration",
       "Tell me about a time you disagreed with a teammate on a technical "
       "decision."},
      {D::Backend, S::HardSkills, L::Junior, "REST",
       "What makes an HTTP API resource-oriented and idempotent?"},
      {D::Backend, S::HardSkills, L::Middle, "Databases",
       "How do you diagnose and fix a slow SQL query in production?"},
      {D::Backend, S::English, L::Junior, "English",
       "Describe your last project and your role in it."},
      {D::DevOps, S::HardSkills, L::Junior, "Docker",
       "How do you reduce a Docker image size without breaking the runtime?"},
      {D::DevOps, S::HardSkills, L::Middle, "Kubernetes",
       "Design a zero-downtime deployment for a stateful service."},
      {D::Qa, S::HardSkills, L::Junior, "Testing",
       "What do you automate first, and how do you keep the suite stable?"},
      {D::Qa, S::SoftSkills, L::Junior, "Communication",
       "How do you report a critical bug so the team can act on it quickly?"},
      {D::Pm, S::HardSkills, L::Middle, "Prioritisation",
       "How do you prioritise a backlog when engineering capacity drops?"},
      {D::Pm, S::English, L::Junior, "English",
       "Walk me through a product decision you would make differently today."},
  };

  int index = 1;
  for (const auto &seed : seeds) {
    models::QuestionBankItem item;
    item.id = "seed-" + std::to_string(index++);
    item.direction = seed.direction;
    item.stage = seed.stage;
    item.level = seed.level;
    item.topic = seed.topic;
    item.questionText = seed.text;
    item.isActive = true;
    item.createdAt =
        QDateTime::currentDateTimeUtc()
            .addSecs(-3600 * (index * 3))
            .toString(Qt::ISODate)
            .toStdString();
    m_localBank.push_back(std::move(item));
  }
}

models::QuestionBankItem *AdminService::findLocal(const QString &id) {
  auto it = std::find_if(m_localBank.begin(), m_localBank.end(),
                         [&id](const models::QuestionBankItem &item) {
                           return QString::fromStdString(item.id) == id;
                         });
  if (it == m_localBank.end())
    return nullptr;
  return &(*it);
}

models::PagedResult<models::QuestionBankItem> AdminService::localQuestions(
    const QString &direction, const QString &level, const QString &stage,
    std::optional<bool> isActive, int page, int pageSize) const {
  std::vector<models::QuestionBankItem> filtered;
  filtered.reserve(m_localBank.size());

  for (const auto &item : m_localBank) {
    if (!direction.isEmpty() && enumKey(item.direction) != direction)
      continue;
    if (!level.isEmpty() && enumKey(item.level) != level)
      continue;
    if (!stage.isEmpty() && enumKey(item.stage) != stage)
      continue;
    if (isActive.has_value() && item.isActive != *isActive)
      continue;
    filtered.push_back(item);
  }

  std::sort(filtered.begin(), filtered.end(),
            [](const models::QuestionBankItem &a,
               const models::QuestionBankItem &b) {
              return a.createdAt > b.createdAt;
            });

  const int safePage = std::max(page, 1);
  const int safePageSize = std::max(pageSize, 1);
  const int total = static_cast<int>(filtered.size());
  const int start = std::min((safePage - 1) * safePageSize, total);
  const int end = std::min(start + safePageSize, total);

  models::PagedResult<models::QuestionBankItem> result;
  result.items.assign(filtered.begin() + start, filtered.begin() + end);
  result.page = safePage;
  result.pageSize = safePageSize;
  result.totalCount = total;
  result.totalPages = (total + safePageSize - 1) / safePageSize;
  return result;
}

void AdminService::listQuestions(const QString &direction, const QString &level,
                                 const QString &stage,
                                 std::optional<bool> isActive, int page,
                                 int pageSize, QuestionsCallback callback) {
  if (isOfflineMode() || !m_apiClient) {
    callback(localQuestions(direction, level, stage, isActive, page, pageSize));
    return;
  }

  QStringList query;
  query << QStringLiteral("page=%1").arg(page);
  query << QStringLiteral("pageSize=%1").arg(pageSize);
  if (!direction.isEmpty())
    query << QStringLiteral("direction=%1").arg(direction);
  if (!level.isEmpty())
    query << QStringLiteral("level=%1").arg(level);
  if (!stage.isEmpty())
    query << QStringLiteral("stage=%1").arg(stage);
  if (isActive.has_value())
    query << QStringLiteral("isActive=%1").arg(*isActive ? "true" : "false");

  const QString endpoint =
      QStringLiteral("/api/admin/questions?") + query.join('&');

  m_apiClient->get<models::PagedResult<models::QuestionBankItem>>(
      endpoint,
      [this, direction, level, stage, isActive, page, pageSize,
       callback = std::move(callback)](
          const std::variant<models::PagedResult<models::QuestionBankItem>,
                             network::ApiError> &result) {
        if (std::holds_alternative<
                models::PagedResult<models::QuestionBankItem>>(result)) {
          callback(std::get<models::PagedResult<models::QuestionBankItem>>(
              result));
          return;
        }
        callback(localQuestions(direction, level, stage, isActive, page,
                                pageSize));
      });
}

void AdminService::createQuestion(const models::CreateQuestionRequest &request,
                                  QuestionCallback callback) {
  const auto createLocal = [this, request]() {
    models::QuestionBankItem item;
    item.id = "local-q-" + QUuid::createUuid().toString(QUuid::WithoutBraces)
                             .toStdString();
    item.direction = request.direction;
    item.stage = request.stage;
    item.level = request.level;
    item.topic = request.topic;
    item.questionText = request.questionText;
    item.isActive = true;
    item.createdAt = nowIso().toStdString();
    m_localBank.insert(m_localBank.begin(), item);
    return item;
  };

  if (isOfflineMode() || !m_apiClient) {
    callback(createLocal());
    return;
  }

  m_apiClient->post<models::CreateQuestionRequest, models::QuestionBankItem>(
      "/api/admin/questions", request,
      [createLocal, callback = std::move(callback)](
          const std::variant<models::QuestionBankItem, network::ApiError>
              &result) {
        if (const auto *item =
                std::get_if<models::QuestionBankItem>(&result)) {
          callback(*item);
          return;
        }
        callback(createLocal());
      });
}

void AdminService::updateQuestion(const QString &id,
                                  const models::UpdateQuestionRequest &request,
                                  QuestionCallback callback) {
  const auto updateLocal = [this, id, request]() -> std::optional<models::QuestionBankItem> {
    auto *item = findLocal(id);
    if (item == nullptr)
      return std::nullopt;

    if (request.direction)
      item->direction = *request.direction;
    if (request.stage)
      item->stage = *request.stage;
    if (request.level)
      item->level = *request.level;
    if (request.topic)
      item->topic = *request.topic;
    if (request.questionText)
      item->questionText = *request.questionText;
    if (request.isActive)
      item->isActive = *request.isActive;
    return *item;
  };

  if (isOfflineMode() || !m_apiClient) {
    const auto updated = updateLocal();
    if (updated) {
      callback(*updated);
    } else {
      network::ApiError error;
      error.statusCode = 404;
      error.message = "Question not found.";
      callback(error);
    }
    return;
  }

  m_apiClient->put<models::UpdateQuestionRequest, models::QuestionBankItem>(
      QStringLiteral("/api/admin/questions/%1").arg(id), request,
      [updateLocal, callback = std::move(callback)](
          const std::variant<models::QuestionBankItem, network::ApiError>
              &result) {
        if (const auto *item =
                std::get_if<models::QuestionBankItem>(&result)) {
          callback(*item);
          return;
        }
        if (const auto local = updateLocal()) {
          callback(*local);
          return;
        }
        callback(result);
      });
}

void AdminService::deleteQuestion(const QString &id, DeleteCallback callback) {
  const auto deleteLocal = [this, id]() -> bool {
    auto it = std::find_if(m_localBank.begin(), m_localBank.end(),
                           [&id](const models::QuestionBankItem &item) {
                             return QString::fromStdString(item.id) == id;
                           });
    if (it == m_localBank.end())
      return false;
    m_localBank.erase(it);
    return true;
  };

  if (isOfflineMode() || !m_apiClient) {
    const bool removed = deleteLocal();
    callback(removed, removed ? QString() : QStringLiteral("Question not found."));
    return;
  }

  m_apiClient->del(
      QStringLiteral("/api/admin/questions/%1").arg(id),
      [deleteLocal, callback = std::move(callback)](bool success,
                                                    const network::ApiError &error) {
        if (success) {
          callback(true, QString());
          return;
        }
        const bool removed = deleteLocal();
        callback(removed, removed ? QString()
                                  : QString::fromStdString(error.message));
      });
}

} // namespace devdialogue::services
