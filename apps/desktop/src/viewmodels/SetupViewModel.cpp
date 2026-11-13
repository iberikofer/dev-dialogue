#include "viewmodels/SetupViewModel.hpp"

#include <QPointer>
#include <QRandomGenerator>
#include <variant>

#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

namespace {

// Ids of local demo sessions, e.g. "session-k3x9p0a" (same shape as the web).
QString makeOfflineSessionId() {
    static const QString alphabet = QStringLiteral("abcdefghijklmnopqrstuvwxyz0123456789");
    QString suffix;
    for (int i = 0; i < 7; ++i) {
        suffix.append(alphabet.at(QRandomGenerator::global()->bounded(static_cast<int>(alphabet.size()))));
    }
    return QStringLiteral("session-") + suffix;
}

// A request the server understood and refused (validation, daily limit,
// permissions) is reported to the user; anything else falls back to a demo
// session so the app stays usable without a backend.
bool isRejectedByServer(const models::ApiError& error) {
    return !error.isNetworkDown && error.statusCode >= 400 && error.statusCode < 500
        && error.statusCode != 401 && error.statusCode != 404;
}

} // namespace

SetupViewModel::SetupViewModel(std::shared_ptr<services::InterviewService> interviewService,
                               NavigationViewModel* navigation, QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)), m_navigation(navigation) {}

void SetupViewModel::setDirection(const QString& value) {
    if (m_direction != value) {
        m_direction = value;
        emit directionChanged();
    }
}

void SetupViewModel::setLevel(const QString& value) {
    if (m_level != value) {
        m_level = value;
        emit levelChanged();
    }
}

void SetupViewModel::setLanguage(const QString& value) {
    if (m_language != value) {
        m_language = value;
        emit languageChanged();
    }
}

void SetupViewModel::start() {
    if (m_isBusy) {
        return;
    }
    setErrorMessage(QString());

    if (!m_interviewService) {
        launch(makeOfflineSessionId(), QString(), true);
        return;
    }

    setBusy(true);
    models::CreateInterviewRequest request;
    request.direction = support::enumFromString<models::InterviewDirection>(m_direction);
    request.level = support::enumFromString<models::InterviewLevel>(m_level);
    request.language = support::enumFromString<models::InterviewLanguage>(m_language);

    QPointer<SetupViewModel> self(this);
    m_interviewService->createInterview(
        request, [self](const std::variant<models::CreateInterviewResponse, models::ApiError>& result) {
            if (!self) {
                return;
            }
            self->setBusy(false);
            if (const auto* created = std::get_if<models::CreateInterviewResponse>(&result)) {
                self->launch(QString::fromStdString(created->interviewId),
                             QString::fromStdString(created->firstQuestion), false);
                return;
            }
            const models::ApiError& error = std::get<models::ApiError>(result);
            if (isRejectedByServer(error)) {
                self->setErrorMessage(QString::fromStdString(error.message));
                return;
            }
            self->launch(makeOfflineSessionId(), QString(), true);
        });
}

void SetupViewModel::setBusy(bool busy) {
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit busyChanged();
    }
}

void SetupViewModel::setErrorMessage(const QString& message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

void SetupViewModel::launch(const QString& interviewId, const QString& firstQuestion, bool offline) {
    emit interviewStarted(interviewId, m_direction, m_level, m_language, firstQuestion, offline);
    if (m_navigation) {
        m_navigation->navigateToChat(interviewId);
    }
}

} // namespace devdialogue::viewmodels
