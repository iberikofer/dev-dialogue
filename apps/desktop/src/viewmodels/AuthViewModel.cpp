#include "viewmodels/AuthViewModel.hpp"

#include <QRegularExpression>
#include <QStringList>

#include "services/AuthService.hpp"

namespace devdialogue::viewmodels {

AuthViewModel::AuthViewModel(std::shared_ptr<services::AuthService> authService,
                             QObject *parent)
    : QObject(parent), m_authService(std::move(authService)) {
  connect(m_authService.get(), &services::AuthService::userChanged, this,
          &AuthViewModel::authStateChanged);
}

bool AuthViewModel::isAuthenticated() const {
  return m_authService->isAuthenticated();
}

bool AuthViewModel::isAdmin() const {
  const auto user = m_authService->currentUser();
  return user.has_value() && user->role == models::UserRole::Admin;
}

bool AuthViewModel::isDemoSession() const {
  return m_authService->isMockSession();
}

bool AuthViewModel::isAdminViewActive() const { return m_isAdminViewActive; }

void AuthViewModel::setAdminViewActive(bool active) {
  if (m_isAdminViewActive == active)
    return;
  m_isAdminViewActive = active;
  emit adminViewChanged();
}

QString AuthViewModel::userEmail() const {
  const auto user = m_authService->currentUser();
  return user ? QString::fromStdString(user->email) : QString();
}

QString AuthViewModel::userFullName() const {
  const auto user = m_authService->currentUser();
  if (!user)
    return QString();
  if (user->fullName && !user->fullName->empty())
    return QString::fromStdString(*user->fullName);
  return QString::fromStdString(user->email);
}

QString AuthViewModel::userInitials() const {
  const QString name = userFullName().trimmed();
  if (name.isEmpty())
    return QStringLiteral("?");

  const QStringList parts = name.split(QRegularExpression("\\s+"),
                                       Qt::SkipEmptyParts);

  QString initials;
  for (const QString &part : parts) {
    // Decorations such as the "(Admin)" suffix of the demo profile must not
    // end up in the avatar badge.
    QString cleaned = part;
    for (int index = cleaned.size() - 1; index >= 0; --index) {
      if (!cleaned.at(index).isLetter())
        cleaned.remove(index, 1);
    }

    if (cleaned.isEmpty())
      continue;

    initials.append(cleaned.left(1));
    if (initials.size() == 2)
      break;
  }

  if (initials.isEmpty())
    return QStringLiteral("?");

  return initials.toUpper();
}

QString AuthViewModel::userRoleLabel() const {
  return isAdmin() ? QStringLiteral("admin") : QStringLiteral("user");
}

void AuthViewModel::setLoginModalOpen(bool open) {
  if (m_isLoginModalOpen == open)
    return;
  m_isLoginModalOpen = open;
  if (open)
    setError(QString());
  emit loginModalChanged();
}

void AuthViewModel::loginWithEmail(const QString &,
                                   const QString &) {
  setError(tr("Email sign-in is handled by the web identity provider. "
              "Use demo mode to explore the desktop client."));
}

void AuthViewModel::registerUser(const QString &, const QString &,
                                 const QString &) {
  setError(tr("Registration is handled by the web identity provider. "
              "Use demo mode to explore the desktop client."));
}

void AuthViewModel::mockLogin(const QString &roleStr) {
  m_authService->mockLogin(roleStr == QLatin1String("admin")
                               ? models::UserRole::Admin
                               : models::UserRole::User);
  setLoginModalOpen(false);
  setAdminViewActive(true);
}

void AuthViewModel::toggleAdminView() {
  setAdminViewActive(!m_isAdminViewActive);
}

void AuthViewModel::logout() {
  m_authService->logout();
  setLoginModalOpen(false);
  setAdminViewActive(true);
}

void AuthViewModel::clearError() { setError(QString()); }

void AuthViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyStateChanged();
}

void AuthViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorOccurred(m_errorMessage);
}

} // namespace devdialogue::viewmodels
