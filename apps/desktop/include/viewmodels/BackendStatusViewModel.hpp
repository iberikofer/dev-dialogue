#pragma once

#include <QObject>
#include <memory>

#include "network/ApiClient.hpp"

namespace devdialogue::viewmodels {

// Backend connectivity state for the navbar chip and the warning banner
// (port of the web backendStatusStore). The online flag itself is owned by
// ApiClient; this class adds the banner's dismiss / retry state.
class BackendStatusViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isOffline READ isOffline NOTIFY statusChanged)
    Q_PROPERTY(bool isDismissed READ isDismissed NOTIFY dismissedChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY checkingChanged)

public:
    explicit BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent = nullptr);

    bool isOffline() const;
    bool isDismissed() const { return m_isDismissed; }
    bool isChecking() const { return m_isChecking; }

    Q_INVOKABLE void checkHealth();
    Q_INVOKABLE void dismissBanner();
    Q_INVOKABLE void reopenBanner();

signals:
    void statusChanged();
    void dismissedChanged();
    void checkingChanged();

private:
    void setChecking(bool checking);
    void setDismissed(bool dismissed);

    std::shared_ptr<network::ApiClient> m_apiClient;
    bool m_isDismissed{false};
    bool m_isChecking{false};
};

} // namespace devdialogue::viewmodels
