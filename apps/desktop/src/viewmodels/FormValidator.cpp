#include "viewmodels/FormValidator.hpp"

#include <QRegularExpression>

namespace devdialogue::viewmodels {

namespace {

constexpr int MinNicknameLength = 3;
constexpr int MinPasswordLength = 8;
constexpr int MaxPasswordLength = 72;

} // namespace

FormValidator::FormValidator(QObject* parent) : QObject(parent) {}

bool FormValidator::isValidNickname(const QString& text) {
    static const QRegularExpression pattern(QStringLiteral("^[a-zA-Z0-9_]+$"));
    return text.size() >= MinNicknameLength && pattern.match(text).hasMatch();
}

bool FormValidator::isValidPassword(const QString& text) {
    static const QRegularExpression letter(QStringLiteral("[a-zA-Z]"));
    static const QRegularExpression digit(QStringLiteral("[0-9]"));
    return text.size() >= MinPasswordLength && text.size() <= MaxPasswordLength
        && letter.match(text).hasMatch() && digit.match(text).hasMatch();
}

bool FormValidator::isValidEmail(const QString& text) {
    static const QRegularExpression pattern(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
    return pattern.match(text).hasMatch();
}

} // namespace devdialogue::viewmodels
