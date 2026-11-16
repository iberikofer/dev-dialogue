#pragma once

#include <QObject>
#include <memory>

#include "network/ApiClient.hpp"

namespace devdialogue::viewmodels {

// Backend connectivity state for the sidebar indicator and the account
// page. The online flag itself is owned by ApiClient; this class adds the
// on-demand check and its "checking" state.
class BackendStatusViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isOffline READ isOffline NOTIFY statusChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY checkingChanged)

public:
    explicit BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent = nullptr);

    bool isOffline() const;
    bool isChecking() const { return m_isChecking; }

    Q_INVOKABLE void checkHealth();

signals:
    void statusChanged();
    void checkingChanged();

private:
    void setChecking(bool checking);

    std::shared_ptr<network::ApiClient> m_apiClient;
    bool m_isChecking{false};
};

} // namespace devdialogue::viewmodels
