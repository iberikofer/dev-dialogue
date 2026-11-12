#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace devdialogue::services {
class InterviewService;
}

namespace devdialogue::viewmodels {

class NavigationViewModel;

/// Backs the interview setup screen: track/level/language selection and
/// creation of a new session.
class SetupViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY
                 selectionChanged)
  Q_PROPERTY(QString level READ level WRITE setLevel NOTIFY selectionChanged)
  Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY
                 selectionChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  SetupViewModel(std::shared_ptr<services::InterviewService> interviewService,
                 NavigationViewModel *navigation, QObject *parent = nullptr);

  QString direction() const { return m_direction; }
  void setDirection(const QString &direction);

  QString level() const { return m_level; }
  void setLevel(const QString &level);

  QString language() const { return m_language; }
  void setLanguage(const QString &language);

  bool isBusy() const { return m_isBusy; }
  QString errorMessage() const { return m_errorMessage; }

  Q_INVOKABLE void startInterview();
  Q_INVOKABLE void resetSelection();
  Q_INVOKABLE void clearError();

signals:
  void selectionChanged();
  void busyChanged();
  void errorMessageChanged();
  void interviewCreated(const QString &interviewId);

private:
  void setBusy(bool busy);
  void setError(const QString &error);

  std::shared_ptr<services::InterviewService> m_interviewService;
  NavigationViewModel *m_navigation;

  QString m_direction{QStringLiteral("frontend")};
  QString m_level{QStringLiteral("junior")};
  QString m_language{QStringLiteral("uk")};
  bool m_isBusy{false};
  QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
