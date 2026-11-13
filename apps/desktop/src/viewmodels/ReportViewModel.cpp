#include "viewmodels/ReportViewModel.hpp"

#include <QPointer>
#include <QVariantMap>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

// Local demo sessions never reached the server, so there is nothing to fetch.
bool isOfflineSessionId(const QString& interviewId) {
    return interviewId.isEmpty() || interviewId.startsWith(QLatin1String("session-"));
}

QStringList toStringList(const std::vector<std::string>& items) {
    QStringList result;
    result.reserve(static_cast<qsizetype>(items.size()));
    for (const std::string& item : items) {
        result.append(QString::fromStdString(item));
    }
    return result;
}

} // namespace

ReportViewModel::ReportViewModel(std::shared_ptr<services::InterviewService> interviewService, QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)) {}

QStringList ReportViewModel::strengths() const {
    return toStringList(m_report.strengths);
}

QStringList ReportViewModel::weaknesses() const {
    return toStringList(m_report.weaknesses);
}

QVariantList ReportViewModel::recommendations() const {
    QVariantList result;
    for (const models::RecommendationItem& item : m_report.recommendations) {
        result.append(QVariantMap{
            {QStringLiteral("topic"), QString::fromStdString(item.topic)},
            {QStringLiteral("resource"), QString::fromStdString(item.resource)},
        });
    }
    return result;
}

QVariantList ReportViewModel::questionScores() const {
    QVariantList result;
    for (const models::QuestionScoreItem& item : m_report.questionScores) {
        result.append(QVariantMap{
            {QStringLiteral("question"), QString::fromStdString(item.question)},
            {QStringLiteral("score"), item.score},
            {QStringLiteral("comment"), QString::fromStdString(item.comment)},
        });
    }
    return result;
}

void ReportViewModel::setUiLanguage(const QString& language) {
    if (m_uiLanguage == language) {
        return;
    }
    m_uiLanguage = language;
    emit uiLanguageChanged();
    // The sample report follows the interface language.
    if (m_hasReport && m_isDemoData) {
        apply(demo::report(m_uiLanguage), true);
    }
}

void ReportViewModel::load(const QString& interviewId) {
    const int generation = ++m_generation;
    setLoading(true);

    if (!m_interviewService || isOfflineSessionId(interviewId) || !backendLikelyOnline()) {
        apply(demo::report(m_uiLanguage), true);
        return;
    }

    QPointer<ReportViewModel> self(this);
    m_interviewService->getInterview(
        interviewId, [self, generation](const std::variant<models::InterviewDetail, models::ApiError>& result) {
            if (!self || generation != self->m_generation) {
                return;
            }
            const auto* detail = std::get_if<models::InterviewDetail>(&result);
            if (detail != nullptr && detail->feedback.has_value()) {
                self->apply(*detail->feedback, false);
            } else {
                self->apply(demo::report(self->m_uiLanguage), true);
            }
        });
}

void ReportViewModel::setLoading(bool value) {
    if (m_loading != value) {
        m_loading = value;
        emit loadingChanged();
    }
}

void ReportViewModel::apply(const models::InterviewFeedbackReport& report, bool isDemo) {
    m_report = report;
    m_isDemoData = isDemo;
    m_hasReport = true;
    emit reportChanged();
    setLoading(false);
}

} // namespace devdialogue::viewmodels
