#pragma once

#include <QDateTime>
#include <QString>
#include <functional>
#include <string>

#include <nlohmann/json.hpp>

#include "models/ApiContracts.hpp"

// Small helpers shared by the screen view models.
namespace devdialogue::viewmodels::support {

// Contract enums travel as lower-case strings ("frontend", "junior", ...).
// The QML layer works with the same strings, so conversion goes through the
// JSON serializers of ApiContracts.hpp to stay in sync with the API.
template <typename TEnum>
QString enumToString(TEnum value) {
    const nlohmann::json json = value;
    return QString::fromStdString(json.get<std::string>());
}

template <typename TEnum>
TEnum enumFromString(const QString& text) {
    return nlohmann::json(text.toStdString()).get<TEnum>();
}

inline QDateTime parseTimestamp(const std::string& iso) {
    const QString text = QString::fromStdString(iso);
    QDateTime parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(text, Qt::ISODate);
    }
    return parsed;
}

// "12.11.2026  13:28" in the user's time zone. Numeric on purpose: month
// names would follow the system locale, not the interface language.
inline QString formatDateTime(const std::string& iso) {
    const QDateTime parsed = parseTimestamp(iso);
    if (!parsed.isValid()) {
        return QString::fromStdString(iso);
    }
    return parsed.toLocalTime().toString(QStringLiteral("dd.MM.yyyy  HH:mm"));
}

// "13:28" in the user's time zone.
inline QString formatTime(const std::string& iso) {
    const QDateTime parsed = parseTimestamp(iso);
    if (!parsed.isValid()) {
        return QString();
    }
    return parsed.toLocalTime().toString(QStringLiteral("HH:mm"));
}

inline std::string nowIso() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
}

// Lets a view model skip a request it already knows will fail. The probe is
// optional; without one every request is attempted.
class BackendAware {
public:
    using Probe = std::function<bool()>;

    void setBackendOnlineProbe(Probe probe) { m_probe = std::move(probe); }

protected:
    bool backendLikelyOnline() const { return !m_probe || m_probe(); }

private:
    Probe m_probe;
};

} // namespace devdialogue::viewmodels::support
