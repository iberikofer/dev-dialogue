#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <optional>
#include <functional>

#include "models/ApiContracts.hpp"
#include "network/ApiClient.hpp"

namespace devdialogue::services {

class AdminService : public QObject {
    Q_OBJECT

public:
    explicit AdminService(
        std::shared_ptr<network::ApiClient> apiClient,
        QObject* parent = nullptr
    );
    ~AdminService() override = default;

    void getQuestions(
        std::optional<models::InterviewDirection> direction,
        std::optional<models::InterviewLevel> level,
        std::optional<models::InterviewStage> stage,
        std::optional<bool> isActive,
        int page,
        int pageSize,
        network::ApiCallback<models::PagedResult<models::QuestionBankItem>> callback
    );

    void createQuestion(
        const models::CreateQuestionRequest& req,
        network::ApiCallback<models::QuestionBankItem> callback
    );

    void updateQuestion(
        const QString& id,
        const models::UpdateQuestionRequest& req,
        network::ApiCallback<models::QuestionBankItem> callback
    );

    void deleteQuestion(
        const QString& id,
        std::function<void(bool success, const models::ApiError& error)> callback
    );

private:
    std::shared_ptr<network::ApiClient> m_apiClient;
};

} // namespace devdialogue::services
