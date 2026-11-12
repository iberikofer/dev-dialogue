#pragma once

#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "models/ApiContracts.hpp"
#include "network/ApiClient.hpp"

namespace devdialogue::core {
class AppConfig;
}

namespace devdialogue::services {

/// Question-bank administration with an in-memory bank for offline demos.
class AdminService : public QObject {
  Q_OBJECT

public:
  using QuestionsCallback = std::function<void(
      const std::variant<models::PagedResult<models::QuestionBankItem>,
                         network::ApiError> &)>;
  using QuestionCallback = std::function<void(
      const std::variant<models::QuestionBankItem, network::ApiError> &)>;
  using DeleteCallback =
      std::function<void(bool success, const QString &errorMessage)>;

  AdminService(std::shared_ptr<network::ApiClient> apiClient,
               std::shared_ptr<core::AppConfig> config,
               QObject *parent = nullptr);

  void listQuestions(const QString &direction, const QString &level,
                     const QString &stage,
                     std::optional<bool> isActive, int page, int pageSize,
                     QuestionsCallback callback);

  void createQuestion(const models::CreateQuestionRequest &request,
                      QuestionCallback callback);

  void updateQuestion(const QString &id,
                      const models::UpdateQuestionRequest &request,
                      QuestionCallback callback);

  void deleteQuestion(const QString &id, DeleteCallback callback);

  bool isOfflineMode() const;

private:
  void seedLocalBank();
  models::PagedResult<models::QuestionBankItem>
  localQuestions(const QString &direction, const QString &level,
                 const QString &stage, std::optional<bool> isActive,
                 int page, int pageSize) const;
  models::QuestionBankItem *findLocal(const QString &id);

  std::shared_ptr<network::ApiClient> m_apiClient;
  std::shared_ptr<core::AppConfig> m_config;
  std::vector<models::QuestionBankItem> m_localBank;
};

} // namespace devdialogue::services
