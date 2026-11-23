#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
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
    Q_PROPERTY(bool isDebugLoading READ isDebugLoading NOTIFY debugInfoChanged)
    Q_PROPERTY(QVariantMap debugDatabase READ debugDatabase NOTIFY debugInfoChanged)
    Q_PROPERTY(QVariantMap debugAi READ debugAi NOTIFY debugInfoChanged)
    Q_PROPERTY(QVariantList debugQuestionsPool READ debugQuestionsPool NOTIFY debugInfoChanged)
    Q_PROPERTY(QVariantList debugTrace READ debugTrace NOTIFY debugInfoChanged)
    Q_PROPERTY(bool localModelReady READ localModelReady NOTIFY localModelChanged)
    Q_PROPERTY(bool localModelChecked READ localModelChecked NOTIFY localModelChanged)
    Q_PROPERTY(bool isDownloadingModel READ isDownloadingModel NOTIFY localModelChanged)
    Q_PROPERTY(int modelDownloadProgress READ modelDownloadProgress NOTIFY localModelChanged)
    Q_PROPERTY(QString localModelName READ localModelName NOTIFY localModelChanged)
    Q_PROPERTY(QString localModelStatus READ localModelStatus NOTIFY localModelChanged)
    Q_PROPERTY(QString localModelError READ localModelError NOTIFY localModelChanged)
    Q_PROPERTY(bool ollamaRunning READ ollamaRunning NOTIFY localModelChanged)
    Q_PROPERTY(bool avatarServerReady READ avatarServerReady NOTIFY localModelChanged)
    Q_PROPERTY(bool avatarHttp8080Running READ avatarHttp8080Running NOTIFY localModelChanged)
    Q_PROPERTY(bool avatarTts8085Running READ avatarTts8085Running NOTIFY localModelChanged)
    Q_PROPERTY(bool rhubarbAvailable READ rhubarbAvailable NOTIFY localModelChanged)
    Q_PROPERTY(QString avatarStatusText READ avatarStatusText NOTIFY localModelChanged)
    Q_PROPERTY(bool sttModelReady READ sttModelReady NOTIFY localModelChanged)
    Q_PROPERTY(QString sttModelName READ sttModelName NOTIFY localModelChanged)
    Q_PROPERTY(bool allServicesReady READ allServicesReady NOTIFY localModelChanged)
    Q_PROPERTY(bool isManagingServices READ isManagingServices NOTIFY localModelChanged)
    Q_PROPERTY(QString servicesActionText READ servicesActionText NOTIFY localModelChanged)
    Q_PROPERTY(QString currentCommit READ currentCommit CONSTANT)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateStateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateStateChanged)
    Q_PROPERTY(QString latestCommit READ latestCommit NOTIFY updateStateChanged)
    Q_PROPERTY(bool isUpdating READ isUpdating NOTIFY updateStateChanged)
    Q_PROPERTY(int updateProgress READ updateProgress NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateStatusText READ updateStatusText NOTIFY updateStateChanged)

public:
    explicit BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent = nullptr);
    ~BackendStatusViewModel() override;

    void setLocalApiBaseUrl(const QString& url);
    Q_INVOKABLE void ensureLocalBackendRunning();
    Q_INVOKABLE void ensureAllServicesRunning();
    Q_INVOKABLE void checkAllServices();
    Q_INVOKABLE void restartAllServices();
    Q_INVOKABLE void restartApiBackend();
    Q_INVOKABLE void restartAvatarServers();
    Q_INVOKABLE void restartLocalModels();

    bool isOffline() const;
    bool isChecking() const { return m_isChecking; }
    bool useSqlite() const { return m_useSqlite; }
    bool isSwitchingDb() const { return m_isSwitchingDb; }
    QString databaseProvider() const { return m_databaseProvider; }
    bool isDebugLoading() const { return m_isDebugLoading; }
    QVariantMap debugDatabase() const { return m_debugDatabase; }
    QVariantMap debugAi() const { return m_debugAi; }
    QVariantList debugQuestionsPool() const { return m_debugQuestionsPool; }
    QVariantList debugTrace() const { return m_debugTrace; }
    bool localModelReady() const { return m_localModelReady; }
    bool localModelChecked() const { return m_localModelChecked; }
    bool isDownloadingModel() const { return m_isDownloadingModel; }
    int modelDownloadProgress() const { return m_modelDownloadProgress; }
    QString localModelName() const { return m_localModelName; }
    QString localModelStatus() const { return m_localModelStatus; }
    QString localModelError() const { return m_localModelError; }
    bool ollamaRunning() const { return m_ollamaRunning; }
    bool avatarServerReady() const { return m_avatarServerReady; }
    bool avatarHttp8080Running() const { return m_avatarHttp8080Running; }
    bool avatarTts8085Running() const { return m_avatarTts8085Running; }
    bool rhubarbAvailable() const { return m_rhubarbAvailable; }
    QString avatarStatusText() const { return m_avatarStatusText; }
    bool sttModelReady() const { return m_sttModelReady; }
    QString sttModelName() const { return m_sttModelName; }
    bool allServicesReady() const {
        return !isOffline() && m_avatarServerReady && m_ollamaRunning && m_localModelReady && m_sttModelReady;
    }
    bool isManagingServices() const { return m_isManagingServices || m_isChecking; }
    QString servicesActionText() const { return m_servicesActionText; }

    Q_INVOKABLE void checkHealth();
    Q_INVOKABLE void refreshDbMode();
    Q_INVOKABLE void toggleDatabaseMode();
    Q_INVOKABLE void refreshDebugInfo();
    Q_INVOKABLE void refreshLocalModelStatus();
    Q_INVOKABLE void downloadLocalModel();
    Q_INVOKABLE void checkForUpdates(bool silent = true);
    Q_INVOKABLE void installUpdate();

    QString currentCommit() const;
    bool updateAvailable() const { return m_updateAvailable; }
    QString latestVersion() const { return m_latestVersion; }
    QString latestCommit() const { return m_latestCommit; }
    bool isUpdating() const { return m_isUpdating; }
    int updateProgress() const { return m_updateProgress; }
    QString updateStatusText() const { return m_updateStatusText; }

signals:
    void statusChanged();
    void checkingChanged();
    void dbModeChanged();
    void dbSwitchingChanged();
    void debugInfoChanged();
    void localModelChanged();
    void avatarReloadRequested();
    void updateStateChanged();

private:
    void setChecking(bool checking);
    void setSwitchingDb(bool switching);
    void setManagingServices(bool managing, const QString& actionText = QString());
    void applyDbMode(bool useSqlite, const QString& provider);
    void checkDirectOllamaStatus();
    void applyStatusJson(const nlohmann::json& json);
    void applyLocalModelState(bool ready, bool checked, bool downloading, int progress,
                              const QString& modelName, const QString& status, const QString& error);
    void startOllamaProcessIfInstalled();
    void start3DServersLocallyIfPresent();

    std::shared_ptr<network::ApiClient> m_apiClient;
    QTimer m_modelPollTimer;
    QTimer m_backendBootTimer;
    QProcess* m_localDownloadProcess{nullptr};
    QProcess* m_localBackendProcess{nullptr};
    QString m_localApiBaseUrl{QStringLiteral("http://localhost:5238")};
    int m_backendBootAttempts{0};
    bool m_isChecking{false};
    bool m_useSqlite{false};
    bool m_isSwitchingDb{false};
    bool m_isDebugLoading{false};
    QString m_databaseProvider;
    QVariantMap m_debugDatabase;
    QVariantMap m_debugAi;
    QVariantList m_debugQuestionsPool;
    QVariantList m_debugTrace;
    bool m_localModelReady{false};
    bool m_localModelChecked{false};
    bool m_isDownloadingModel{false};
    int m_modelDownloadProgress{0};
    QString m_localModelName{QStringLiteral("gemma4:e4b")};
    QString m_localModelStatus;
    QString m_localModelError;
    bool m_ollamaRunning{false};
    bool m_avatarServerReady{false};
    bool m_avatarHttp8080Running{false};
    bool m_avatarTts8085Running{false};
    bool m_rhubarbAvailable{false};
    QString m_avatarStatusText;
    bool m_sttModelReady{true};
    QString m_sttModelName{QStringLiteral("parakeet-tdt-0.6b-v3")};
    bool m_isManagingServices{false};
    QString m_servicesActionText;
    bool m_updateAvailable{false};
    QString m_latestVersion;
    QString m_latestCommit;
    QString m_latestInstallerUrl;
    QString m_latestInstallerSha256;
    bool m_isUpdating{false};
    int m_updateProgress{0};
    QString m_updateStatusText;
};

} // namespace devdialogue::viewmodels
