#include "viewmodels/BackendStatusViewModel.hpp"

#include <QTimer>

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
        if (online) {
            // A fresh outage should announce itself again.
            setDismissed(false);
        }
        setChecking(false);
        emit statusChanged();
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
    QTimer::singleShot(CheckingIndicatorMs, this, [this]() { setChecking(false); });
}

void BackendStatusViewModel::dismissBanner() {
    setDismissed(true);
}

void BackendStatusViewModel::reopenBanner() {
    setDismissed(false);
}

void BackendStatusViewModel::setChecking(bool checking) {
    if (m_isChecking != checking) {
        m_isChecking = checking;
        emit checkingChanged();
    }
}

void BackendStatusViewModel::setDismissed(bool dismissed) {
    if (m_isDismissed != dismissed) {
        m_isDismissed = dismissed;
        emit dismissedChanged();
    }
}

} // namespace devdialogue::viewmodels
