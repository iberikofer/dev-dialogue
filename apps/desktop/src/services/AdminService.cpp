#include "services/AdminService.hpp"
#include "network/NetworkEndpoints.hpp"
#include <QUrlQuery>

namespace devdialogue::services {

AdminService::AdminService(
    std::shared_ptr<network::ApiClient> apiClient,
    QObject* parent
) : QObject(parent), m_apiClient(std::move(apiClient)) {}

void AdminService::getQuestions(
    std::optional<models::InterviewDirection> direction,
    std::optional<models::InterviewLevel> level,
    std::optional<models::InterviewStage> stage,
    std::optional<bool> isActive,
    int page,
    int pageSize,
    network::ApiCallback<models::PagedResult<models::QuestionBankItem>> callback
) {
    QUrlQuery query;
    if (direction.has_value()) {
        nlohmann::json j = *direction;
        query.addQueryItem("direction", QString::fromStdString(j.get<std::string>()));
    }
    if (level.has_value()) {
        nlohmann::json j = *level;
        query.addQueryItem("level", QString::fromStdString(j.get<std::string>()));
    }
    if (stage.has_value()) {
        nlohmann::json j = *stage;
        query.addQueryItem("stage", QString::fromStdString(j.get<std::string>()));
    }
    if (isActive.has_value()) {
        query.addQueryItem("isActive", *isActive ? "true" : "false");
    }
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("pageSize", QString::number(pageSize));

    QString endpoint = endpoints::api::AdminQuestions;
    QString queryString = query.toString();
    if (!queryString.isEmpty()) {
        endpoint += "?" + queryString;
    }

    m_apiClient->get<models::PagedResult<models::QuestionBankItem>>(endpoint, callback);
}

void AdminService::createQuestion(
    const models::CreateQuestionRequest& req,
    network::ApiCallback<models::QuestionBankItem> callback
) {
    m_apiClient->post<models::CreateQuestionRequest, models::QuestionBankItem>(
        endpoints::api::AdminQuestions, req, callback);
}

void AdminService::updateQuestion(
    const QString& id,
    const models::UpdateQuestionRequest& req,
    network::ApiCallback<models::QuestionBankItem> callback
) {
    m_apiClient->put<models::UpdateQuestionRequest, models::QuestionBankItem>(
        endpoints::api::adminQuestionById(id), req, callback);
}

void AdminService::deleteQuestion(
    const QString& id,
    std::function<void(bool success, const models::ApiError& error)> callback
) {
    m_apiClient->del(endpoints::api::adminQuestionById(id), callback);
}

} // namespace devdialogue::services
