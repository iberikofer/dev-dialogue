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
    Q_PROPERTY(bool useSqlite READ useSqlite NOTIFY dbModeChanged)
    Q_PROPERTY(bool isSwitchingDb READ isSwitchingDb NOTIFY dbSwitchingChanged)
    Q_PROPERTY(QString databaseProvider READ databaseProvider NOTIFY dbModeChanged)

public:
    explicit BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent = nullptr);

    bool isOffline() const;
    bool isChecking() const { return m_isChecking; }
    bool useSqlite() const { return m_useSqlite; }
    bool isSwitchingDb() const { return m_isSwitchingDb; }
    QString databaseProvider() const { return m_databaseProvider; }

    Q_INVOKABLE void checkHealth();
    Q_INVOKABLE void refreshDbMode();
    Q_INVOKABLE void toggleDatabaseMode();

signals:
    void statusChanged();
    void checkingChanged();
    void dbModeChanged();
    void dbSwitchingChanged();

private:
    void setChecking(bool checking);
    void setSwitchingDb(bool switching);
    void applyDbMode(bool useSqlite, const QString& provider);

    std::shared_ptr<network::ApiClient> m_apiClient;
    bool m_isChecking{false};
    bool m_useSqlite{false};
    bool m_isSwitchingDb{false};
    QString m_databaseProvider;
};

} // namespace devdialogue::viewmodels
