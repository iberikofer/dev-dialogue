#include "viewmodels/DashboardViewModel.hpp"

#include <QString>

#include <utility>
#include <variant>

#include "models/InterviewListModel.hpp"
#include "services/InterviewService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

DashboardViewModel::DashboardViewModel(
    std::shared_ptr<services::InterviewService> interviewService,
    NavigationViewModel *navigation, QObject *parent)
    : QObject(parent), m_interviewService(std::move(interviewService)),
      m_navigation(navigation),
      m_interviewsModel(new models::InterviewListModel(this)) {}

void DashboardViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyChanged();
}

void DashboardViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorMessageChanged();
}

void DashboardViewModel::applyStats(const models::UserDashboardStats &stats) {
  m_totalInterviews = stats.totalInterviews;
  m_averageScore = stats.averageScore;
  m_dailyInterviewsCount = stats.dailyInterviewsCount;
  m_dailyLimit = stats.dailyLimit;
  m_remainingToday = stats.remainingToday;
  emit statsChanged();
}

void DashboardViewModel::refresh() {
  setBusy(true);
  setError(QString());

  m_interviewService->getStats(
      [this](const std::variant<models::UserDashboardStats,
                                network::ApiError> &result) {
        if (const auto *stats =
                std::get_if<models::UserDashboardStats>(&result)) {
          applyStats(*stats);
        } else {
          setError(QString::fromStdString(
              std::get<network::ApiError>(result).message));
        }
      });

  m_interviewService->listInterviews(
      1, 20,
      [this](const std::variant<models::PagedResult<models::InterviewSummary>,
                                network::ApiError> &result) {
        setBusy(false);
        if (const auto *page =
                std::get_if<models::PagedResult<models::InterviewSummary>>(
                    &result)) {
          m_interviewsModel->setInterviews(page->items);
        } else {
          const auto &error = std::get<network::ApiError>(result);
          if (!error.message.empty())
            setError(QString::fromStdString(error.message));
        }
      });
}

void DashboardViewModel::openInterview(const QString &id,
                                       const QString &status) {
  if (!m_navigation)
    return;

  if (status == QLatin1String("completed"))
    m_navigation->navigateToReport(id);
  else
    m_navigation->navigateToChat(id);
}

void DashboardViewModel::startNewInterview() {
  if (m_navigation)
    m_navigation->navigateToSetup();
}

void DashboardViewModel::openAdmin() {
  if (m_navigation)
    m_navigation->navigateToAdmin();
}

void DashboardViewModel::clearError() { setError(QString()); }

} // namespace devdialogue::viewmodels
