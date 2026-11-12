#include "viewmodels/SettingsViewModel.hpp"

namespace devdialogue::viewmodels {

namespace {

const QString KeyTheme = QStringLiteral("ui/theme");
const QString KeySound = QStringLiteral("ui/soundEnabled");
const QString KeyPersona = QStringLiteral("ui/aiPersona");

const QString FieldNickname = QStringLiteral("nickname");
const QString FieldEmail = QStringLiteral("email");
const QString FieldAvatar = QStringLiteral("avatarUrl");
const QString FieldProviders = QStringLiteral("linkedProviders");

} // namespace

SettingsViewModel::SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_authService(std::move(authService)) {
    if (m_authService) {
        connect(m_authService.get(), &services::AuthService::userChanged, this, [this]() {
            emit profileChanged();
            emit linkedProvidersChanged();
        });
    }
}

QString SettingsViewModel::profileKey(const QString& field) const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    if (!user.has_value()) {
        return QString();
    }
    // Demo accounts of both roles share one id, so the role is part of the key.
    return QStringLiteral("profiles/%1-%2/%3").arg(QString::fromStdString(user->id), role(), field);
}

QString SettingsViewModel::profileValue(const QString& field) const {
    const QString key = profileKey(field);
    return key.isEmpty() ? QString() : m_settings.value(key).toString();
}

QString SettingsViewModel::displayName() const {
    const QString stored = profileValue(FieldNickname);
    if (!stored.isEmpty()) {
        return stored;
    }
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
    const QString stored = profileValue(FieldEmail);
    if (!stored.isEmpty()) {
        return stored;
    }
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    return user.has_value() ? QString::fromStdString(user->email) : QString();
}

QString SettingsViewModel::avatarUrl() const {
    const QString stored = profileValue(FieldAvatar);
    if (!stored.isEmpty()) {
        return stored;
    }
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

QStringList SettingsViewModel::defaultProviders() const {
    // Same starting points as the web client's demo accounts.
    if (role() == QLatin1String("admin")) {
        return {QStringLiteral("github"), QStringLiteral("google")};
    }
    return {QStringLiteral("email"), QStringLiteral("github")};
}

QStringList SettingsViewModel::linkedProviders() const {
    const QString key = profileKey(FieldProviders);
    if (key.isEmpty()) {
        return {};
    }
    return m_settings.contains(key) ? m_settings.value(key).toStringList() : defaultProviders();
}

QString SettingsViewModel::theme() const {
    return m_settings.value(KeyTheme, QStringLiteral("oled")).toString();
}

void SettingsViewModel::setTheme(const QString& theme) {
    if (theme != this->theme()) {
        m_settings.setValue(KeyTheme, theme);
        emit themeChanged();
    }
}

bool SettingsViewModel::soundEnabled() const {
    return m_settings.value(KeySound, true).toBool();
}

void SettingsViewModel::setSoundEnabled(bool enabled) {
    if (enabled != soundEnabled()) {
        m_settings.setValue(KeySound, enabled);
        emit soundEnabledChanged();
    }
}

QString SettingsViewModel::aiPersona() const {
    return m_settings.value(KeyPersona, QStringLiteral("balanced")).toString();
}

void SettingsViewModel::setAiPersona(const QString& persona) {
    if (persona != aiPersona()) {
        m_settings.setValue(KeyPersona, persona);
        emit aiPersonaChanged();
    }
}

void SettingsViewModel::saveProfile(const QString& nickname, const QString& email, const QString& avatarUrl) {
    if (profileKey(FieldNickname).isEmpty()) {
        return;
    }
    m_settings.setValue(profileKey(FieldNickname), nickname.trimmed());
    m_settings.setValue(profileKey(FieldEmail), email.trimmed());
    m_settings.setValue(profileKey(FieldAvatar), avatarUrl.trimmed());
    emit profileChanged();
}

bool SettingsViewModel::toggleProvider(const QString& provider) {
    const QString key = profileKey(FieldProviders);
    if (key.isEmpty()) {
        return false;
    }
    QStringList providers = linkedProviders();
    const bool wasLinked = providers.contains(provider);
    if (wasLinked) {
        providers.removeAll(provider);
    } else {
        providers.append(provider);
    }
    m_settings.setValue(key, providers);
    emit linkedProvidersChanged();
    return !wasLinked;
}

} // namespace devdialogue::viewmodels
