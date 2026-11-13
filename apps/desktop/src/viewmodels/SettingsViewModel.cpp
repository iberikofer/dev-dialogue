#include "viewmodels/SettingsViewModel.hpp"

namespace devdialogue::viewmodels {

namespace {

// Id given to demo accounts by AuthService::mockLogin.
constexpr const char* DemoUserId = "mock-user-123";

} // namespace

SettingsViewModel::SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_authService(std::move(authService)) {
    if (m_authService) {
        connect(m_authService.get(), &services::AuthService::userChanged, this, &SettingsViewModel::profileChanged);
    }
}

QString SettingsViewModel::displayName() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    if (!user.has_value()) {
        return QString();
    }
    if (user->fullName.has_value() && !user->fullName->empty()) {
        return QString::fromStdString(*user->fullName);
    }
    return QString::fromStdString(user->email).section(QLatin1Char('@'), 0, 0);
}

QString SettingsViewModel::email() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    return user.has_value() ? QString::fromStdString(user->email) : QString();
}

QString SettingsViewModel::avatarUrl() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    if (user.has_value() && user->avatarUrl.has_value()) {
        return QString::fromStdString(*user->avatarUrl);
    }
    return QString();
}

QString SettingsViewModel::role() const {
    return m_authService && m_authService->isAdmin() ? QStringLiteral("admin") : QStringLiteral("user");
}

bool SettingsViewModel::isDemoAccount() const {
    if (!m_authService) {
        return false;
    }
    const auto user = m_authService->currentUser();
    return user.has_value() && user->id == DemoUserId;
}

} // namespace devdialogue::viewmodels
