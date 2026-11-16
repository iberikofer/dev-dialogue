#include "viewmodels/ReportViewModel.hpp"

#include <QPointer>
#include <QVariantMap>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

// Local demo sessions and the sample interviews never reached the server,
// so there is nothing to fetch.
bool isOfflineSessionId(const QString& interviewId) {
    return interviewId.isEmpty() || interviewId.startsWith(QLatin1String("session-")) ||
           interviewId.startsWith(QLatin1String("mock-"));
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
    if (m_hasReport && m_followsUiLanguage) {
        apply(demo::report(m_interviewId, m_uiLanguage), true);
    }
}

void ReportViewModel::load(const QString& interviewId) {
    const int generation = ++m_generation;
    m_interviewId = interviewId;
    setLoading(true);

    // Sample interviews and interviews finished offline exist only in the
    // demo history.
    if (isOfflineSessionId(interviewId) || demo::finishedInterview(interviewId) != nullptr) {
        applyDemo(interviewId);
        return;
    }
    // Any other interview lives on the server; a stand-in report would show
    // numbers that are not its own.
    if (!m_interviewService || !backendLikelyOnline()) {
        fail(tr("The server can't be reached, so this report can't be loaded."));
        return;
    }

    QPointer<ReportViewModel> self(this);
    m_interviewService->getInterview(
        interviewId, [self, generation](const std::variant<models::InterviewDetail, models::ApiError>& result) {
            if (!self || generation != self->m_generation) {
                return;
            }
            const auto* detail = std::get_if<models::InterviewDetail>(&result);
            if (detail == nullptr) {
                const auto& error = std::get<models::ApiError>(result);
                if (error.isNetworkDown) {
                    self->fail(tr("The server can't be reached, so this report can't be loaded."));
                } else if (!error.message.empty()) {
                    self->fail(tr("The report could not be loaded: %1").arg(QString::fromStdString(error.message)));
                } else {
                    self->fail(tr("The report could not be loaded."));
                }
                return;
            }
            self->setContext(*detail);
            self->m_followsUiLanguage = false;
            if (detail->feedback.has_value()) {
                self->apply(*detail->feedback, false);
            } else {
                // Not evaluated yet: score what the transcript holds.
                self->apply(demo::reportFromTranscript(support::enumToString(detail->direction), detail->transcript,
                                                       support::enumToString(detail->language)),
                            true);
            }
        });
}

void ReportViewModel::applyDemo(const QString& interviewId) {
    models::InterviewSummary context;
    for (const models::InterviewSummary& item : demo::interviews()) {
        if (QString::fromStdString(item.id) == interviewId) {
            context = item;
            break;
        }
    }
    setContext(context);
    // Recorded interviews are reported in their own language.
    m_followsUiLanguage = demo::finishedInterview(interviewId) == nullptr;
    apply(demo::report(interviewId, m_uiLanguage), true);
}

void ReportViewModel::setContext(const models::InterviewSummary& interview) {
    if (interview.id.empty()) {
        m_direction.clear();
        m_level.clear();
        m_dateText.clear();
        return;
    }
    m_direction = support::enumToString(interview.direction);
    m_level = support::enumToString(interview.level);
    m_dateText = support::formatDateTime(interview.createdAt);
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
    m_errorMessage.clear();
    emit reportChanged();
    setLoading(false);
}

void ReportViewModel::fail(const QString& message) {
    m_report = {};
    m_isDemoData = false;
    m_hasReport = false;
    m_followsUiLanguage = false;
    m_errorMessage = message;
    setContext({});
    emit reportChanged();
    setLoading(false);
}

} // namespace devdialogue::viewmodels
