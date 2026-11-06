#include "viewmodels/AuthViewModel.hpp"

namespace devdialogue::viewmodels {

AuthViewModel::AuthViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_authService(std::move(authService)) {

    if (m_authService) {
        connect(m_authService.get(), &services::AuthService::userChanged,
                this, &AuthViewModel::authStateChanged);

        connect(m_authService.get(), &services::AuthService::authErrorOccurred,
                this, [this](const QString& err) {
                    m_errorMessage = err;
                    emit errorOccurred(m_errorMessage);
                });

        connect(m_authService.get(), &services::AuthService::emailConfirmationRequired,
                this, [this](const QString& email) {
                    m_errorMessage = QString("Please check %1 to confirm your email address.").arg(email);
                    emit errorOccurred(m_errorMessage);
                });
    }
}

bool AuthViewModel::isAuthenticated() const {
    return m_authService && m_authService->isAuthenticated();
}

bool AuthViewModel::isAdmin() const {
    return m_authService && m_authService->isAdmin();
}

bool AuthViewModel::isAdminViewActive() const {
    return m_isAdminViewActive;
}

void AuthViewModel::setAdminViewActive(bool active) {
    if (m_isAdminViewActive != active) {
        m_isAdminViewActive = active;
        emit adminViewChanged();
    }
}

QString AuthViewModel::userEmail() const {
    if (m_authService) {
        auto user = m_authService->currentUser();
        if (user.has_value()) {
            return QString::fromStdString(user->email);
        }
    }
    return QString();
}

QString AuthViewModel::userFullName() const {
    if (m_authService) {
        auto user = m_authService->currentUser();
        if (user.has_value() && user->fullName.has_value()) {
            return QString::fromStdString(user->fullName.value());
        }
    }
    return QString();
}

void AuthViewModel::setLoginModalOpen(bool open) {
    if (m_isLoginModalOpen != open) {
        m_isLoginModalOpen = open;
        emit loginModalChanged();
    }
}

void AuthViewModel::loginWithEmail(const QString& email, const QString& password) {
    if (!m_authService) return;

    m_isBusy = true;
    emit busyStateChanged();
    m_errorMessage.clear();
    emit errorOccurred(m_errorMessage);

    m_authService->loginWithEmail(email, password, [this](bool success, const QString& error) {
        m_isBusy = false;
        emit busyStateChanged();

        if (success) {
            setLoginModalOpen(false);
        } else {
            m_errorMessage = error;
            emit errorOccurred(m_errorMessage);
        }
    });
}

void AuthViewModel::registerUser(const QString& email, const QString& password, const QString& fullName) {
    if (!m_authService) return;

    m_isBusy = true;
    emit busyStateChanged();
    m_errorMessage.clear();
    emit errorOccurred(m_errorMessage);

    m_authService->registerUser(email, password, fullName, [this](bool success, bool confirmationRequired, const QString& error) {
        m_isBusy = false;
        emit busyStateChanged();

        if (success) {
            setLoginModalOpen(false);
            if (confirmationRequired) {
                m_errorMessage = "Registration successful! Please confirm your email before logging in.";
                emit errorOccurred(m_errorMessage);
            }
        } else {
            m_errorMessage = error;
            emit errorOccurred(m_errorMessage);
        }
    });
}

void AuthViewModel::mockLogin(const QString& roleStr) {
    if (!m_authService) return;

    models::UserRole role = (roleStr.toLower() == "admin") ? models::UserRole::Admin : models::UserRole::User;
    m_authService->mockLogin(role);
    setLoginModalOpen(false);
}

void AuthViewModel::toggleAdminView() {
    setAdminViewActive(!m_isAdminViewActive);
}

void AuthViewModel::logout() {
    if (m_authService) {
        m_authService->logout();
    }
}

} // namespace devdialogue::viewmodels
