#include "viewmodels/BackendStatusViewModel.hpp"

#include <QPointer>
#include <QTimer>
#include <variant>

#include "network/NetworkEndpoints.hpp"

namespace devdialogue::viewmodels {

namespace {

// ApiClient reports only changes of the online flag, not the end of a health
// check, so the "Checking..." state is shown for a fixed moment.
constexpr int CheckingIndicatorMs = 1500;

} // namespace

BackendStatusViewModel::BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent)
    : QObject(parent), m_apiClient(std::move(apiClient)) {
    if (!m_apiClient) {
        return;
    }

    connect(m_apiClient.get(), &network::ApiClient::onlineStatusChanged, this, [this](bool online) {
        setChecking(false);
        emit statusChanged();
        if (online) {
            refreshDbMode();
        }
    });

    // Learn the real state right after start-up instead of waiting for the
    // first periodic heartbeat.
    QTimer::singleShot(0, this, [this]() { m_apiClient->performHealthCheck(); });
}

bool BackendStatusViewModel::isOffline() const {
    return m_apiClient && !m_apiClient->isOnline();
}

void BackendStatusViewModel::checkHealth() {
    if (!m_apiClient || m_isChecking) {
        return;
    }
    setChecking(true);
    m_apiClient->performHealthCheck();
    refreshDbMode();
    QTimer::singleShot(CheckingIndicatorMs, this, [this]() { setChecking(false); });
}

void BackendStatusViewModel::refreshDbMode() {
    if (!m_apiClient || !m_apiClient->isOnline()) {
        return;
    }
    QPointer<BackendStatusViewModel> self(this);
    m_apiClient->get<models::DbModeStatus>(
        network::endpoints::api::HealthDbMode,
        [self](const std::variant<models::DbModeStatus, models::ApiError>& result) {
            if (!self) {
                return;
            }
            if (const auto* status = std::get_if<models::DbModeStatus>(&result)) {
                QString provider = QString::fromStdString(status->database);
                if (!status->cipher.empty()) {
                    provider += QStringLiteral(" (") + QString::fromStdString(status->cipher) + QStringLiteral(")");
                }
                self->applyDbMode(status->useSqlite, provider);
            }
        });
}

void BackendStatusViewModel::toggleDatabaseMode() {
    if (!m_apiClient || !m_apiClient->isOnline() || m_isSwitchingDb) {
        return;
    }
    setSwitchingDb(true);
    models::DbModeSwitchRequest req;
    req.provider = m_useSqlite ? "default" : "sqlite";

    QPointer<BackendStatusViewModel> self(this);
    m_apiClient->post<models::DbModeSwitchRequest, models::DbModeStatus>(
        network::endpoints::api::HealthDbMode,
        req,
        [self](const std::variant<models::DbModeStatus, models::ApiError>& result) {
            if (!self) {
                return;
            }
            self->setSwitchingDb(false);
            if (std::holds_alternative<models::DbModeStatus>(result)) {
                self->refreshDbMode();
            }
        });
}

void BackendStatusViewModel::setChecking(bool checking) {
    if (m_isChecking != checking) {
        m_isChecking = checking;
        emit checkingChanged();
    }
}

void BackendStatusViewModel::setSwitchingDb(bool switching) {
    if (m_isSwitchingDb != switching) {
        m_isSwitchingDb = switching;
        emit dbSwitchingChanged();
    }
}

void BackendStatusViewModel::applyDbMode(bool useSqlite, const QString& provider) {
    if (m_useSqlite != useSqlite || m_databaseProvider != provider) {
        m_useSqlite = useSqlite;
        m_databaseProvider = provider;
        emit dbModeChanged();
    }
}

} // namespace devdialogue::viewmodels
