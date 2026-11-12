#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace devdialogue::services {
class AuthService;
}

namespace devdialogue::viewmodels {

class AuthViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStateChanged)
  Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY authStateChanged)
  Q_PROPERTY(bool isDemoSession READ isDemoSession NOTIFY authStateChanged)
  Q_PROPERTY(bool isAdminViewActive READ isAdminViewActive WRITE
                 setAdminViewActive NOTIFY adminViewChanged)
  Q_PROPERTY(QString userEmail READ userEmail NOTIFY authStateChanged)
  Q_PROPERTY(QString userFullName READ userFullName NOTIFY authStateChanged)
  Q_PROPERTY(QString userInitials READ userInitials NOTIFY authStateChanged)
  Q_PROPERTY(QString userRoleLabel READ userRoleLabel NOTIFY authStateChanged)
  Q_PROPERTY(bool isLoginModalOpen READ isLoginModalOpen WRITE
                 setLoginModalOpen NOTIFY loginModalChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyStateChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorOccurred)

public:
  explicit AuthViewModel(std::shared_ptr<services::AuthService> authService,
                         QObject *parent = nullptr);

  bool isAuthenticated() const;
  bool isAdmin() const;
  bool isDemoSession() const;
  bool isAdminViewActive() const;
  void setAdminViewActive(bool active);

  QString userEmail() const;
  QString userFullName() const;
  QString userInitials() const;
  QString userRoleLabel() const;

  bool isLoginModalOpen() const { return m_isLoginModalOpen; }
  void setLoginModalOpen(bool open);

  bool isBusy() const { return m_isBusy; }
  QString errorMessage() const { return m_errorMessage; }

  Q_INVOKABLE void loginWithEmail(const QString &email, const QString &password);
  Q_INVOKABLE void registerUser(const QString &email, const QString &password,
                                const QString &fullName);
  Q_INVOKABLE void mockLogin(const QString &roleStr = QStringLiteral("candidate"));
  Q_INVOKABLE void toggleAdminView();
  Q_INVOKABLE void logout();
  Q_INVOKABLE void clearError();

signals:
  void authStateChanged();
  void adminViewChanged();
  void loginModalChanged();
  void busyStateChanged();
  void errorOccurred(const QString &error);

private:
  void setBusy(bool busy);
  void setError(const QString &error);

  std::shared_ptr<services::AuthService> m_authService;
  bool m_isAdminViewActive{true};
  bool m_isLoginModalOpen{false};
  bool m_isBusy{false};
  QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
