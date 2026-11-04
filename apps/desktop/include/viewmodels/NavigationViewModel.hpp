#pragma once

#include <QObject>
#include <QString>

namespace devdialogue::viewmodels {

class NavigationViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(int currentViewIndex READ currentViewIndex WRITE
                 setCurrentViewIndex NOTIFY currentViewIndexChanged)
  Q_PROPERTY(QString activeInterviewId READ activeInterviewId NOTIFY
                 activeInterviewIdChanged)

public:
  enum ViewIndex {
    Landing = 0,
    Dashboard = 1,
    Setup = 2,
    ChatRoom = 3,
    Report = 4,
    Admin = 5
  };
  Q_ENUM(ViewIndex)

  explicit NavigationViewModel(QObject *parent = nullptr);

  int currentViewIndex() const;
  void setCurrentViewIndex(int index);

  QString activeInterviewId() const;

  Q_INVOKABLE void navigateToLanding();
  Q_INVOKABLE void navigateToDashboard();
  Q_INVOKABLE void navigateToSetup();
  Q_INVOKABLE void navigateToChat(const QString &interviewId);
  Q_INVOKABLE void navigateToReport(const QString &interviewId);
  Q_INVOKABLE void navigateToAdmin();

signals:
  void currentViewIndexChanged(int newIndex);
  void activeInterviewIdChanged(const QString &newId);

private:
  int m_currentViewIndex{0};
  QString m_activeInterviewId;
};

} // namespace devdialogue::viewmodels
