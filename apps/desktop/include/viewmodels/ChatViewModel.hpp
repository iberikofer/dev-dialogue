#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "models/ApiContracts.hpp"

namespace devdialogue::models {
class MessageListModel;
}

namespace devdialogue::services {
class InterviewService;
}

namespace devdialogue::viewmodels {

class NavigationViewModel;

/// Backs the live interview chat room: transcript model, session metadata and
/// the send/complete actions.
class ChatViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(models::MessageListModel *messagesModel READ messagesModel
                 CONSTANT)
  Q_PROPERTY(QString interviewId READ interviewId NOTIFY sessionChanged)
  Q_PROPERTY(QString currentDirection READ currentDirection NOTIFY
                 sessionChanged)
  Q_PROPERTY(QString currentLevel READ currentLevel NOTIFY sessionChanged)
  Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY
                 sessionChanged)
  Q_PROPERTY(int candidateAnswersCount READ candidateAnswersCount NOTIFY
                 conversationChanged)
  Q_PROPERTY(int questionCount READ questionCount NOTIFY conversationChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(bool isCompleted READ isCompleted NOTIFY completedChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  ChatViewModel(std::shared_ptr<services::InterviewService> interviewService,
                NavigationViewModel *navigation, QObject *parent = nullptr);

  models::MessageListModel *messagesModel() const { return m_messagesModel; }
  QString interviewId() const { return m_interviewId; }
  QString currentDirection() const { return m_currentDirection; }
  QString currentLevel() const { return m_currentLevel; }
  QString currentLanguage() const { return m_currentLanguage; }
  int candidateAnswersCount() const { return m_candidateAnswersCount; }
  int questionCount() const { return m_questionCount; }
  bool isBusy() const { return m_isBusy; }
  bool isCompleted() const { return m_isCompleted; }
  QString errorMessage() const { return m_errorMessage; }

  Q_INVOKABLE void loadInterview(const QString &id);
  Q_INVOKABLE void sendAnswer(const QString &content);
  Q_INVOKABLE void completeInterview();
  Q_INVOKABLE void leaveSession();
  Q_INVOKABLE void clearError();

signals:
  void sessionChanged();
  void conversationChanged();
  void busyChanged();
  void completedChanged();
  void errorMessageChanged();

private:
  void setBusy(bool busy);
  void setError(const QString &error);
  void setSessionMetadata(const models::InterviewDetail &detail);

  std::shared_ptr<services::InterviewService> m_interviewService;
  NavigationViewModel *m_navigation;
  models::MessageListModel *m_messagesModel;

  QString m_interviewId;
  QString m_currentDirection{QStringLiteral("frontend")};
  QString m_currentLevel{QStringLiteral("junior")};
  QString m_currentLanguage{QStringLiteral("uk")};
  int m_candidateAnswersCount{0};
  int m_questionCount{0};
  bool m_isBusy{false};
  bool m_isCompleted{false};
  QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
