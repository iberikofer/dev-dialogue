#include "viewmodels/ReportViewModel.hpp"

#include <QDesktopServices>
#include <QUrl>

#include <utility>
#include <variant>

#include "core/EnumUtils.hpp"
#include "services/InterviewService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

ReportViewModel::ReportViewModel(
    std::shared_ptr<services::InterviewService> interviewService,
    NavigationViewModel *navigation, QObject *parent)
    : QObject(parent), m_interviewService(std::move(interviewService)),
      m_navigation(navigation) {}

void ReportViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyChanged();
}

void ReportViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorMessageChanged();
}

void ReportViewModel::applyReport(const models::InterviewDetail &detail) {
  m_interviewId = QString::fromStdString(detail.id);
  m_direction = core::enums::toString(detail.direction);
  m_level = core::enums::toString(detail.level);
  m_language = core::enums::toString(detail.language);
  m_status = core::enums::toString(detail.status);
  m_score = detail.score.value_or(0);

  m_strengths.clear();
  m_weaknesses.clear();
  m_recommendations.clear();
  m_questionScores.clear();
  m_summary.clear();
  m_hasReport = false;

  if (detail.feedback.has_value()) {
    const models::InterviewFeedbackReport &feedback = *detail.feedback;
    m_hasReport = true;
    m_score = feedback.score;

    if (m_score == 0 && detail.score.has_value())
      m_score = *detail.score;

    m_summary = QString::fromStdString(feedback.summary);

    for (const std::string &strength : feedback.strengths)
      m_strengths.append(QString::fromStdString(strength));

    for (const std::string &weakness : feedback.weaknesses)
      m_weaknesses.append(QString::fromStdString(weakness));

    for (const models::RecommendationItem &item : feedback.recommendations) {
      QVariantMap map;
      map.insert(QStringLiteral("topic"),
                 QString::fromStdString(item.topic));
      map.insert(QStringLiteral("resource"),
                 QString::fromStdString(item.resource));
      m_recommendations.append(map);
    }

    for (const models::QuestionScoreItem &item : feedback.questionScores) {
      QVariantMap map;
      map.insert(QStringLiteral("question"),
                 QString::fromStdString(item.question));
      map.insert(QStringLiteral("score"), item.score);
      map.insert(QStringLiteral("comment"),
                 QString::fromStdString(item.comment));
      m_questionScores.append(map);
    }
  } else if (detail.score.has_value()) {
    m_hasReport = true;
  }

  emit reportChanged();
}

void ReportViewModel::loadReport(const QString &id) {
  if (id.isEmpty())
    return;

  m_interviewId = id;
  setError(QString());
  setBusy(true);

  m_interviewService->getInterview(
      id, [this](const std::variant<models::InterviewDetail, network::ApiError>
                     &result) {
        setBusy(false);

        if (const auto *detail =
                std::get_if<models::InterviewDetail>(&result)) {
          applyReport(*detail);
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void ReportViewModel::startNewInterview() {
  if (m_navigation)
    m_navigation->navigateToSetup();
}

void ReportViewModel::backToDashboard() {
  if (m_navigation)
    m_navigation->navigateToDashboard();
}

void ReportViewModel::openRecommendation(const QString &url) {
  if (!url.isEmpty())
    QDesktopServices::openUrl(QUrl(url));
}

void ReportViewModel::clearError() { setError(QString()); }

} // namespace devdialogue::viewmodels
