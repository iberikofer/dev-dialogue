#include "services/InterviewService.hpp"
#include "network/NetworkEndpoints.hpp"
#include <QUrlQuery>

namespace devdialogue::services {

InterviewService::InterviewService(
    std::shared_ptr<network::ApiClient> apiClient,
    QObject* parent
) : QObject(parent), m_apiClient(std::move(apiClient)) {}

void InterviewService::getOptions(network::ApiCallback<models::OptionsResponse> callback) {
    m_apiClient->get<models::OptionsResponse>(endpoints::api::Options, callback);
}

void InterviewService::createInterview(
    const models::CreateInterviewRequest& req,
    network::ApiCallback<models::CreateInterviewResponse> callback
) {
    m_apiClient->post<models::CreateInterviewRequest, models::CreateInterviewResponse>(
        endpoints::api::Interviews, req, callback);
}

void InterviewService::getInterviews(
    int page,
    int pageSize,
    network::ApiCallback<models::PagedResult<models::InterviewSummary>> callback
) {
    QString endpoint = QString("%1?page=%2&pageSize=%3")
        .arg(endpoints::api::Interviews)
        .arg(page)
        .arg(pageSize);
    m_apiClient->get<models::PagedResult<models::InterviewSummary>>(endpoint, callback);
}

void InterviewService::getInterview(
    const QString& id,
    network::ApiCallback<models::InterviewDetail> callback
) {
    m_apiClient->get<models::InterviewDetail>(endpoints::api::interviewById(id), callback);
}

void InterviewService::sendMessage(
    const QString& id,
    const QString& content,
    network::ApiCallback<models::SendMessageResponse> callback
) {
    models::SendMessageRequest req{content.toStdString()};
    m_apiClient->post<models::SendMessageRequest, models::SendMessageResponse>(
        endpoints::api::interviewMessage(id), req, callback);
}

void InterviewService::completeInterview(
    const QString& id,
    network::ApiCallback<models::CompleteInterviewResponse> callback
) {
    nlohmann::json emptyBody = nlohmann::json::object();
    m_apiClient->post<nlohmann::json, models::CompleteInterviewResponse>(
        endpoints::api::interviewComplete(id), emptyBody, callback);
}

void InterviewService::getStats(
    network::ApiCallback<models::UserDashboardStats> callback
) {
    m_apiClient->get<models::UserDashboardStats>(endpoints::api::InterviewStats, callback);
}

} // namespace devdialogue::services
