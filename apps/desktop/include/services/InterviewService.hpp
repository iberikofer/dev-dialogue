#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

#include "models/ApiContracts.hpp"
#include "network/ApiClient.hpp"

namespace devdialogue::services {

class InterviewService : public QObject {
    Q_OBJECT

public:
    explicit InterviewService(
        std::shared_ptr<network::ApiClient> apiClient,
        QObject* parent = nullptr
    );
    ~InterviewService() override = default;

    void getOptions(network::ApiCallback<models::OptionsResponse> callback);

    void createInterview(
        const models::CreateInterviewRequest& req,
        network::ApiCallback<models::CreateInterviewResponse> callback
    );

    void getInterviews(
        int page,
        int pageSize,
        network::ApiCallback<models::PagedResult<models::InterviewSummary>> callback
    );

    void getInterview(
        const QString& id,
        network::ApiCallback<models::InterviewDetail> callback
    );

    void sendMessage(
        const QString& id,
        const QString& content,
        network::ApiCallback<models::SendMessageResponse> callback
    );

    void completeInterview(
        const QString& id,
        network::ApiCallback<models::CompleteInterviewResponse> callback
    );

    void getStats(
        network::ApiCallback<models::UserDashboardStats> callback
    );

private:
    std::shared_ptr<network::ApiClient> m_apiClient;
};

} // namespace devdialogue::services
