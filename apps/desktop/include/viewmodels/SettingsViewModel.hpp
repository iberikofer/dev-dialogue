#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <memory>

#include "services/AuthService.hpp"

namespace devdialogue::viewmodels {

// State behind the settings dialog (port of the profile part of the web
// authStore plus the localStorage-backed platform preferences).
//
// Profile edits and linked sign-in services are local to this device, as on
// the web client, where they only update the client-side session. They are
// stored with QSettings per user and layered over the signed-in profile.
class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(QString email READ email NOTIFY profileChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY profileChanged)
    Q_PROPERTY(QString role READ role NOTIFY profileChanged)
    Q_PROPERTY(QStringList linkedProviders READ linkedProviders NOTIFY linkedProvidersChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(QString aiPersona READ aiPersona WRITE setAiPersona NOTIFY aiPersonaChanged)

public:
    explicit SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent = nullptr);

    QString displayName() const;
    QString email() const;
    QString avatarUrl() const;
    QString role() const;
    QStringList linkedProviders() const;

    QString theme() const;
    void setTheme(const QString& value);
    bool soundEnabled() const;
    void setSoundEnabled(bool enabled);
    QString aiPersona() const;
    void setAiPersona(const QString& persona);

    // Stores the edited profile fields for the current user. Empty values
    // clear the override and fall back to the signed-in profile.
    Q_INVOKABLE void saveProfile(const QString& nickname, const QString& emailAddress, const QString& avatarLink);

    // Links or unlinks a sign-in service; returns true when it is now linked.
    Q_INVOKABLE bool toggleProvider(const QString& provider);

signals:
    void profileChanged();
    void linkedProvidersChanged();
    void themeChanged();
    void soundEnabledChanged();
    void aiPersonaChanged();

private:
    QString profileKey(const QString& field) const;
    QString profileValue(const QString& field) const;
    QStringList defaultProviders() const;

    std::shared_ptr<services::AuthService> m_authService;
    QSettings m_settings;
};

} // namespace devdialogue::viewmodels
