#include "viewmodels/ChatViewModel.hpp"

#include <QPointer>
#include <algorithm>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

const QString RoleAssistant = QStringLiteral("assistant");
const QString RoleUser = QStringLiteral("user");
const QString DefaultDirection = QStringLiteral("frontend");
const QString DefaultLevel = QStringLiteral("junior");
const QString DefaultLanguage = QStringLiteral("uk");

} // namespace

ChatViewModel::ChatViewModel(std::shared_ptr<services::InterviewService> interviewService,
                             NavigationViewModel* navigation, QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)), m_navigation(navigation) {
    m_clock.setInterval(1000);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        ++m_elapsedSeconds;
        emit elapsedChanged();
    });
}

QString ChatViewModel::elapsedText() const {
    return QStringLiteral("%1:%2")
        .arg(m_elapsedSeconds / 60, 2, 10, QLatin1Char('0'))
        .arg(m_elapsedSeconds % 60, 2, 10, QLatin1Char('0'));
}

void ChatViewModel::beginSession(const QString& interviewId, const QString& direction, const QString& level,
                                 const QString& language, const QString& firstQuestion, bool offline) {
    resetSession(interviewId, direction, level, language, offline);
    if (firstQuestion.isEmpty()) {
        appendWelcome();
    } else {
        appendMessage(RoleAssistant, firstQuestion);
    }
}

void ChatViewModel::openSession(const QString& interviewId) {
    resetSession(interviewId, DefaultDirection, DefaultLevel, DefaultLanguage, false);

    if (!m_interviewService || !backendLikelyOnline()) {
        m_offline = true;
        appendWelcome();
        return;
    }

    const int token = m_sessionToken;
    QPointer<ChatViewModel> self(this);
    m_interviewService->getInterview(
        interviewId, [self, token](const std::variant<models::InterviewDetail, models::ApiError>& result) {
            if (!self || token != self->m_sessionToken) {
                return;
            }
            if (const auto* detail = std::get_if<models::InterviewDetail>(&result)) {
                self->m_direction = support::enumToString(detail->direction);
                self->m_level = support::enumToString(detail->level);
                self->m_language = support::enumToString(detail->language);
                emit self->sessionChanged();

                self->m_messages.setMessages(detail->transcript);
                emit self->messagesChanged();
                if (self->m_messages.count() == 0) {
                    self->appendWelcome();
                }
                return;
            }
            self->m_offline = true;
            self->appendWelcome();
        });
}

void ChatViewModel::sendAnswer(const QString& text) {
    const QString answer = text.trimmed();
    if (answer.isEmpty() || m_isBusy || m_isCompleting || m_interviewId.isEmpty()) {
        return;
    }

    const int answersBefore = m_messages.userMessageCount();
    appendMessage(RoleUser, answer);
    setErrorMessage(QString());
    setBusy(true);

    if (m_offline || !m_interviewService) {
        scheduleFallbackReply(answersBefore);
        return;
    }

    const int token = m_sessionToken;
    QPointer<ChatViewModel> self(this);
    m_interviewService->sendMessage(
        m_interviewId, answer,
        [self, token, answersBefore](const std::variant<models::SendMessageResponse, models::ApiError>& result) {
            if (!self || token != self->m_sessionToken) {
                return;
            }
            if (const auto* reply = std::get_if<models::SendMessageResponse>(&result)) {
                if (!reply->message.empty()) {
                    self->appendMessage(RoleAssistant, QString::fromStdString(reply->message));
                }
                self->setBusy(false);
                return;
            }
            // Keep the conversation going without the backend; once the
            // network is down the rest of the session stays local.
            if (std::get<models::ApiError>(result).isNetworkDown) {
                self->m_offline = true;
            }
            self->scheduleFallbackReply(answersBefore);
        });
}

void ChatViewModel::completeInterview() {
    if (m_isCompleting || m_interviewId.isEmpty()) {
        return;
    }
    setCompleting(true);
    m_clock.stop();

    const QString interviewId = m_interviewId;
    QPointer<ChatViewModel> self(this);
    // The report opens whether or not the server call succeeds; the report
    // screen has its own fallback, exactly like the web client.
    auto finish = [self, interviewId]() {
        if (!self) {
            return;
        }
        self->setCompleting(false);
        emit self->interviewCompleted(interviewId);
        if (self->m_navigation) {
            self->m_navigation->navigateToReport(interviewId);
        }
    };

    if (m_offline || !m_interviewService) {
        finish();
        return;
    }
    m_interviewService->completeInterview(
        interviewId, [finish](const std::variant<models::CompleteInterviewResponse, models::ApiError>&) { finish(); });
}

void ChatViewModel::resetSession(const QString& interviewId, const QString& direction, const QString& level,
                                 const QString& language, bool offline) {
    ++m_sessionToken;
    m_interviewId = interviewId;
    m_direction = direction;
    m_level = level;
    m_language = language;
    m_offline = offline;
    emit sessionChanged();

    m_messages.clear();
    emit messagesChanged();

    setBusy(false);
    setCompleting(false);
    setErrorMessage(QString());

    m_elapsedSeconds = 0;
    emit elapsedChanged();
    m_clock.start();
}

void ChatViewModel::appendMessage(const QString& role, const QString& content) {
    m_messages.append(role, content);
    emit messagesChanged();
}

void ChatViewModel::appendWelcome() {
    appendMessage(RoleAssistant, demo::chatWelcome(m_direction, m_level, m_language));
}

void ChatViewModel::scheduleFallbackReply(int answersBefore) {
    const int token = m_sessionToken;
    QTimer::singleShot(m_fallbackReplyDelayMs, this, [this, token, answersBefore]() {
        if (token != m_sessionToken) {
            return;
        }
        const QStringList replies = demo::chatReplies(m_language);
        const qsizetype index = std::min<qsizetype>(answersBefore, replies.size() - 1);
        appendMessage(RoleAssistant, replies.at(index));
        setBusy(false);
    });
}

void ChatViewModel::setBusy(bool busy) {
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit busyChanged();
    }
}

void ChatViewModel::setCompleting(bool completing) {
    if (m_isCompleting != completing) {
        m_isCompleting = completing;
        emit completingChanged();
    }
}

void ChatViewModel::setErrorMessage(const QString& message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

} // namespace devdialogue::viewmodels
