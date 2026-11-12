#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

#include "services/InterviewService.hpp"
#include "viewmodels/MessageListModel.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

// Interview room controller (port of the web ChatRoom.tsx): the transcript,
// sending answers, the session clock and finishing the interview.
class ChatViewModel : public QObject, public support::BackendAware {
    Q_OBJECT
    Q_PROPERTY(QString interviewId READ interviewId NOTIFY sessionChanged)
    Q_PROPERTY(QString currentDirection READ currentDirection NOTIFY sessionChanged)
    Q_PROPERTY(QString currentLevel READ currentLevel NOTIFY sessionChanged)
    Q_PROPERTY(QString sessionLanguage READ sessionLanguage NOTIFY sessionChanged)
    Q_PROPERTY(QAbstractItemModel* messagesModel READ messagesModel CONSTANT)
    Q_PROPERTY(int candidateAnswersCount READ candidateAnswersCount NOTIFY messagesChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool isCompleting READ isCompleting NOTIFY completingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY elapsedChanged)

public:
    explicit ChatViewModel(std::shared_ptr<services::InterviewService> interviewService,
                           NavigationViewModel* navigation, QObject* parent = nullptr);

    QString interviewId() const { return m_interviewId; }
    QString currentDirection() const { return m_direction; }
    QString currentLevel() const { return m_level; }
    QString sessionLanguage() const { return m_language; }
    QAbstractItemModel* messagesModel() { return &m_messages; }
    int candidateAnswersCount() const { return m_messages.userMessageCount(); }
    bool isBusy() const { return m_isBusy; }
    bool isCompleting() const { return m_isCompleting; }
    QString errorMessage() const { return m_errorMessage; }
    // Session time as "mm:ss".
    QString elapsedText() const;

    // Posts the candidate's answer and appends the interviewer's reply. If
    // the backend is unreachable a scripted demo reply is used instead.
    Q_INVOKABLE void sendAnswer(const QString& text);

    // Finishes the interview and opens the report.
    Q_INVOKABLE void completeInterview();

    // Resumes an existing interview by loading its transcript.
    Q_INVOKABLE void openSession(const QString& interviewId);

    // Milliseconds before a scripted demo reply appears (tests shorten it).
    void setFallbackReplyDelay(int milliseconds) { m_fallbackReplyDelayMs = milliseconds; }

public slots:
    // Starts a freshly created interview (wired to SetupViewModel::interviewStarted).
    void beginSession(const QString& interviewId, const QString& direction, const QString& level,
                      const QString& language, const QString& firstQuestion, bool offline);

signals:
    void sessionChanged();
    void messagesChanged();
    void busyChanged();
    void completingChanged();
    void errorMessageChanged();
    void elapsedChanged();
    void interviewCompleted(const QString& interviewId);

private:
    void resetSession(const QString& interviewId, const QString& direction, const QString& level,
                      const QString& language, bool offline);
    void appendMessage(const QString& role, const QString& content);
    void appendWelcome();
    void scheduleFallbackReply(int answersBefore);
    void setBusy(bool busy);
    void setCompleting(bool completing);
    void setErrorMessage(const QString& message);

    std::shared_ptr<services::InterviewService> m_interviewService;
    NavigationViewModel* m_navigation;
    MessageListModel m_messages;
    QTimer m_clock;

    QString m_interviewId;
    QString m_direction;
    QString m_level;
    QString m_language;
    QString m_errorMessage;
    // Incremented for every new session so late replies of an old one are ignored.
    int m_sessionToken{0};
    int m_elapsedSeconds{0};
    int m_fallbackReplyDelayMs{900};
    bool m_offline{false};
    bool m_isBusy{false};
    bool m_isCompleting{false};
};

} // namespace devdialogue::viewmodels
