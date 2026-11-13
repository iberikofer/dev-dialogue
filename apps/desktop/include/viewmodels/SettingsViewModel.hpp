#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "services/AuthService.hpp"

namespace devdialogue::viewmodels {

// Read-only view of the signed-in account for the sidebar and the account
// page. Profile editing is not offered: the backend has no endpoint for it.
class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(QString email READ email NOTIFY profileChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY profileChanged)
    Q_PROPERTY(QString role READ role NOTIFY profileChanged)
    Q_PROPERTY(bool isDemoAccount READ isDemoAccount NOTIFY profileChanged)

public:
    explicit SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent = nullptr);

    // Full name, or the part of the email before "@" when there is none.
    QString displayName() const;
    QString email() const;
    QString avatarUrl() const;
    // "admin" or "user"
    QString role() const;
    // True for the offline demo accounts created by AuthService::mockLogin.
    bool isDemoAccount() const;

signals:
    void profileChanged();

private:
    std::shared_ptr<services::AuthService> m_authService;
};

} // namespace devdialogue::viewmodels
