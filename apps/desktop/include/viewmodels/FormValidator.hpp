#pragma once

#include <QObject>
#include <QString>

namespace devdialogue::viewmodels {

// Input rules of the sign-up and profile forms, kept out of QML so they can
// be unit tested. The rules match the web client's login page.
class FormValidator : public QObject {
    Q_OBJECT

public:
    explicit FormValidator(QObject* parent = nullptr);

    // At least 3 characters; English letters, digits and underscores only.
    Q_INVOKABLE static bool isValidNickname(const QString& text);

    // 8 to 72 characters with at least one letter and one digit.
    Q_INVOKABLE static bool isValidPassword(const QString& text);

    // Loose shape check (something@domain.tld); the server has the final say.
    Q_INVOKABLE static bool isValidEmail(const QString& text);
};

} // namespace devdialogue::viewmodels
