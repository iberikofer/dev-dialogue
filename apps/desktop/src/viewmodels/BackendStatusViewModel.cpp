#include "viewmodels/BackendStatusViewModel.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>
#include <variant>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "network/NetworkEndpoints.hpp"

namespace devdialogue::viewmodels {

namespace {

// ApiClient reports only changes of the online flag, not the end of a health
// check, so the "Checking..." state is shown for a fixed moment.
constexpr int CheckingIndicatorMs = 1500;

bool isTcpPortOpen(const QString& host, quint16 port, int timeoutMs = 150) {
    QTcpSocket probe;
    probe.connectToHost(host, port);
    const bool connected = probe.waitForConnected(timeoutMs);
    if (connected) {
        probe.disconnectFromHost();
    }
    return connected;
}

QString findOllamaBinaryPath() {
    const QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QStringList candidates = {
        localAppData + QStringLiteral("/Programs/Ollama/ollama.exe"),
        QDir::homePath() + QStringLiteral("/AppData/Local/Programs/Ollama/ollama.exe"),
        QStringLiteral("C:/Program Files/Ollama/ollama.exe")
    };
    for (const QString& c : candidates) {
        if (QFileInfo::exists(c)) {
            return c;
        }
    }
    return QStandardPaths::findExecutable(QStringLiteral("ollama"));
}

} // namespace

BackendStatusViewModel::BackendStatusViewModel(std::shared_ptr<network::ApiClient> apiClient, QObject* parent)
    : QObject(parent), m_apiClient(std::move(apiClient)) {
    if (!m_apiClient) {
        return;
    }

    connect(m_apiClient.get(), &network::ApiClient::onlineStatusChanged, this, [this](bool online) {
        setChecking(false);
        emit statusChanged();
        emit localModelChanged();
        if (online) {
            refreshDbMode();
            refreshLocalModelStatus();
        }
    });

    m_modelPollTimer.setInterval(1000);
    connect(&m_modelPollTimer, &QTimer::timeout, this, &BackendStatusViewModel::refreshLocalModelStatus);

    m_backendBootTimer.setInterval(250);
    connect(&m_backendBootTimer, &QTimer::timeout, this, [this]() {
        if (!m_apiClient) {
            m_backendBootTimer.stop();
            return;
        }
        if (m_apiClient->isOnline()) {
            m_backendBootTimer.stop();
            refreshDbMode();
            refreshLocalModelStatus();
            return;
        }
        ++m_backendBootAttempts;
        m_apiClient->performHealthCheck();
        if (m_backendBootAttempts >= 48) {
            m_backendBootTimer.stop();
        }
    });

    // Learn the real state right after start-up instead of waiting for the
    // first periodic heartbeat.
    QTimer::singleShot(0, this, [this]() { m_apiClient->performHealthCheck(); });
}

BackendStatusViewModel::~BackendStatusViewModel() {
    if (m_localBackendProcess) {
        if (m_localBackendProcess->state() != QProcess::NotRunning) {
            m_localBackendProcess->terminate();
            if (!m_localBackendProcess->waitForFinished(1200)) {
                m_localBackendProcess->kill();
                m_localBackendProcess->waitForFinished(500);
            }
        }
        m_localBackendProcess->deleteLater();
        m_localBackendProcess = nullptr;
    }
}

void BackendStatusViewModel::setLocalApiBaseUrl(const QString& url) {
    if (!url.trimmed().isEmpty()) {
        m_localApiBaseUrl = url.trimmed();
    }
}

void BackendStatusViewModel::ensureLocalBackendRunning() {
    if (m_localBackendProcess && m_localBackendProcess->state() != QProcess::NotRunning) {
        return;
    }

    const QUrl parsed = QUrl::fromUserInput(m_localApiBaseUrl);
    const QString host = parsed.host().toLower();
    if (host != QLatin1String("localhost") && host != QLatin1String("127.0.0.1")) {
        return;
    }

    const quint16 port = static_cast<quint16>(parsed.port(5238));
    QTcpSocket probe;
    probe.connectToHost(host, port);
    if (probe.waitForConnected(150)) {
        probe.disconnectFromHost();
        if (m_apiClient) {
            m_apiClient->performHealthCheck();
        }
        return;
    }

    // Locate apps/api (or deployed ./api) relative to the executable.
    QString apiDir;
    QString apiExe;
    const QString appDir = QCoreApplication::applicationDirPath();
    QDir probeDir(appDir);
    for (int i = 0; i < 6; ++i) {
        const QString deployedApi = probeDir.filePath(QStringLiteral("api/DevDialogue.Api.exe"));
        if (QFileInfo::exists(deployedApi)) {
            apiDir = QFileInfo(deployedApi).absolutePath();
            apiExe = deployedApi;
            break;
        }
        const QString repoApi = probeDir.filePath(QStringLiteral("apps/api"));
        if (QFileInfo::exists(repoApi + QStringLiteral("/DevDialogue.Api.csproj"))) {
            apiDir = QDir(repoApi).absolutePath();
            const QString debugExe = apiDir + QStringLiteral("/bin/Debug/net9.0/DevDialogue.Api.exe");
            const QString releaseExe = apiDir + QStringLiteral("/bin/Release/net9.0/DevDialogue.Api.exe");
            if (QFileInfo::exists(debugExe)) {
                apiExe = debugExe;
            } else if (QFileInfo::exists(releaseExe)) {
                apiExe = releaseExe;
            }
            break;
        }
        if (!probeDir.cdUp()) {
            break;
        }
    }

    if (apiDir.isEmpty()) {
        return;
    }

    const QString listenUrl = QStringLiteral("http://%1:%2").arg(host).arg(port);
    auto* proc = new QProcess(this);
    m_localBackendProcess = proc;
    proc->setWorkingDirectory(apiDir);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ASPNETCORE_ENVIRONMENT"), QStringLiteral("Development"));
    env.insert(QStringLiteral("Database__Provider"), QStringLiteral("Sqlite"));
    env.insert(QStringLiteral("AI__Provider"), QStringLiteral("Ollama"));
    env.insert(QStringLiteral("AI__OllamaUrl"), QStringLiteral("http://127.0.0.1:11434"));
    if (env.value(QStringLiteral("AI__OllamaModel")).isEmpty()) {
        env.insert(QStringLiteral("AI__OllamaModel"), QStringLiteral("gemma4:e4b"));
    }
    proc->setProcessEnvironment(env);

#ifdef Q_OS_WIN
    proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif

    connect(proc, &QProcess::finished, this, [this, proc](int, QProcess::ExitStatus) {
        if (m_localBackendProcess == proc) {
            m_localBackendProcess = nullptr;
        }
        proc->deleteLater();
    });

    if (!apiExe.isEmpty()) {
        proc->start(apiExe, {QStringLiteral("--urls"), listenUrl});
    } else {
        proc->start(QStringLiteral("dotnet"),
                    {QStringLiteral("run"), QStringLiteral("--no-build"), QStringLiteral("--urls"), listenUrl});
    }

    if (proc->waitForStarted(3000)) {
        m_backendBootAttempts = 0;
        m_backendBootTimer.start();
    } else {
        m_localBackendProcess = nullptr;
        proc->deleteLater();
    }
}

bool BackendStatusViewModel::isOffline() const {
    return m_apiClient && !m_apiClient->isOnline();
}

void BackendStatusViewModel::checkHealth() {
    checkAllServices();
}

void BackendStatusViewModel::setManagingServices(bool managing, const QString& actionText) {
    if (m_isManagingServices != managing || m_servicesActionText != actionText) {
        m_isManagingServices = managing;
        m_servicesActionText = actionText;
        emit localModelChanged();
    }
}

void BackendStatusViewModel::startOllamaProcessIfInstalled() {
    if (isTcpPortOpen(QStringLiteral("127.0.0.1"), 11434, 140)) {
        m_ollamaRunning = true;
        return;
    }
    const QString ollamaExe = findOllamaBinaryPath();
    if (ollamaExe.isEmpty()) {
        return;
    }
    auto* proc = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("OLLAMA_ORIGINS"), QStringLiteral("*"));
    proc->setProcessEnvironment(env);
#ifdef Q_OS_WIN
    proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
    proc->start(ollamaExe, {QStringLiteral("serve")});
}

void BackendStatusViewModel::start3DServersLocallyIfPresent() {
    const QString appDir = QCoreApplication::applicationDirPath();
    QDir probeDir(appDir);
    QString avatarDir;
    for (int i = 0; i < 6; ++i) {
        const QString candidate = probeDir.filePath(QStringLiteral("3D Face Import"));
        if (QFileInfo::exists(candidate + QStringLiteral("/model.html"))) {
            avatarDir = QDir(candidate).absolutePath();
            break;
        }
        if (!probeDir.cdUp()) {
            break;
        }
    }
    if (avatarDir.isEmpty()) {
        return;
    }

    QString pythonExe = avatarDir + QStringLiteral("/.venv/Scripts/python.exe");
    if (!QFileInfo::exists(pythonExe)) {
        pythonExe = QStringLiteral("B:/PROJECTS/models/3dface/.venv/Scripts/python.exe");
    }
    if (!QFileInfo::exists(pythonExe)) {
        pythonExe = QStandardPaths::findExecutable(QStringLiteral("python"));
    }
    if (pythonExe.isEmpty()) {
        return;
    }

    if (!isTcpPortOpen(QStringLiteral("127.0.0.1"), 8080, 120)) {
        auto* httpProc = new QProcess(this);
        httpProc->setWorkingDirectory(avatarDir);
#ifdef Q_OS_WIN
        httpProc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
            args->flags |= CREATE_NO_WINDOW;
        });
#endif
        connect(httpProc, &QProcess::finished, httpProc, &QObject::deleteLater);
        httpProc->start(pythonExe, {QStringLiteral("-m"), QStringLiteral("http.server"), QStringLiteral("8080")});
    }

    const QString ttsScript = avatarDir + QStringLiteral("/tools/tts_backend.py");
    if (QFileInfo::exists(ttsScript) && !isTcpPortOpen(QStringLiteral("127.0.0.1"), 8085, 120)) {
        auto* ttsProc = new QProcess(this);
        ttsProc->setWorkingDirectory(avatarDir);
#ifdef Q_OS_WIN
        ttsProc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
            args->flags |= CREATE_NO_WINDOW;
        });
#endif
        connect(ttsProc, &QProcess::finished, ttsProc, &QObject::deleteLater);
        ttsProc->start(pythonExe, {ttsScript, QStringLiteral("8085")});
    }
}

void BackendStatusViewModel::ensureAllServicesRunning() {
    setManagingServices(true, tr("Перевірка та запуск Backend, 3D-серверів і локальних моделей..."));
    ensureLocalBackendRunning();
    startOllamaProcessIfInstalled();
    start3DServersLocallyIfPresent();

    if (m_apiClient && m_apiClient->isOnline()) {
        QPointer<BackendStatusViewModel> self(this);
        m_apiClient->post<nlohmann::json, nlohmann::json>(
            network::endpoints::api::HealthServicesEnsure,
            nlohmann::json::object(),
            [self](const std::variant<nlohmann::json, models::ApiError>& result) {
                if (!self) {
                    return;
                }
                if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                    self->applyStatusJson(*json);
                } else {
                    self->refreshLocalModelStatus();
                }
                self->setManagingServices(false);
            });
    } else {
        if (m_apiClient) {
            m_apiClient->performHealthCheck();
        }
        refreshLocalModelStatus();
        QTimer::singleShot(1400, this, [this]() {
            refreshLocalModelStatus();
            setManagingServices(false);
        });
    }
}

void BackendStatusViewModel::checkAllServices() {
    if (!m_apiClient) {
        return;
    }
    setChecking(true);
    setManagingServices(true, tr("Перевірка стану серверів, 3D-моделі та AI-моделей..."));
    m_apiClient->performHealthCheck();
    refreshDbMode();
    refreshLocalModelStatus();
    QTimer::singleShot(CheckingIndicatorMs, this, [this]() {
        setChecking(false);
        setManagingServices(false);
    });
}

void BackendStatusViewModel::restartApiBackend() {
    setManagingServices(true, tr("Перезапуск основного API Backend (:5238)..."));
    if (m_localBackendProcess && m_localBackendProcess->state() != QProcess::NotRunning) {
        m_localBackendProcess->terminate();
        if (!m_localBackendProcess->waitForFinished(800)) {
            m_localBackendProcess->kill();
            m_localBackendProcess->waitForFinished(400);
        }
        m_localBackendProcess->deleteLater();
        m_localBackendProcess = nullptr;
    }
#ifdef Q_OS_WIN
    QProcess::execute(QStringLiteral("taskkill"), {QStringLiteral("/F"), QStringLiteral("/IM"), QStringLiteral("DevDialogue.Api.exe")});
#endif
    QTimer::singleShot(350, this, [this]() {
        ensureLocalBackendRunning();
        QTimer::singleShot(1600, this, [this]() {
            if (m_apiClient) {
                m_apiClient->performHealthCheck();
            }
            refreshLocalModelStatus();
            emit avatarReloadRequested();
            setManagingServices(false);
        });
    });
}

void BackendStatusViewModel::restartAvatarServers() {
    setManagingServices(true, tr("Перезапуск серверів 3D-моделі (:8080 / :8085 / WebView2)..."));
    start3DServersLocallyIfPresent();
    if (m_apiClient && m_apiClient->isOnline()) {
        QPointer<BackendStatusViewModel> self(this);
        nlohmann::json body{{"target", "avatar"}};
        m_apiClient->post<nlohmann::json, nlohmann::json>(
            network::endpoints::api::HealthServicesRestart,
            body,
            [self](const std::variant<nlohmann::json, models::ApiError>& result) {
                if (!self) {
                    return;
                }
                if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                    self->applyStatusJson(*json);
                }
                emit self->avatarReloadRequested();
                self->setManagingServices(false);
            });
    } else {
        emit avatarReloadRequested();
        QTimer::singleShot(800, this, [this]() {
            refreshLocalModelStatus();
            setManagingServices(false);
        });
    }
}

void BackendStatusViewModel::restartLocalModels() {
    setManagingServices(true, tr("Перезапуск Ollama (:11434 · %1) та Parakeet STT...").arg(m_localModelName));
    startOllamaProcessIfInstalled();
    if (m_apiClient && m_apiClient->isOnline()) {
        QPointer<BackendStatusViewModel> self(this);
        nlohmann::json body{{"target", "models"}};
        m_apiClient->post<nlohmann::json, nlohmann::json>(
            network::endpoints::api::HealthServicesRestart,
            body,
            [self](const std::variant<nlohmann::json, models::ApiError>& result) {
                if (!self) {
                    return;
                }
                if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                    self->applyStatusJson(*json);
                }
                self->setManagingServices(false);
            });
    } else {
        QTimer::singleShot(1000, this, [this]() {
            checkDirectOllamaStatus();
            setManagingServices(false);
        });
    }
}

void BackendStatusViewModel::restartAllServices() {
    setManagingServices(true, tr("Повний перезапуск Backend, 3D-серверів та локальних моделей..."));
    ensureLocalBackendRunning();
    startOllamaProcessIfInstalled();
    start3DServersLocallyIfPresent();

    if (m_apiClient && m_apiClient->isOnline()) {
        QPointer<BackendStatusViewModel> self(this);
        nlohmann::json body{{"target", "all"}};
        m_apiClient->post<nlohmann::json, nlohmann::json>(
            network::endpoints::api::HealthServicesRestart,
            body,
            [self](const std::variant<nlohmann::json, models::ApiError>& result) {
                if (!self) {
                    return;
                }
                if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                    self->applyStatusJson(*json);
                } else {
                    self->refreshLocalModelStatus();
                }
                emit self->avatarReloadRequested();
                self->setManagingServices(false);
            });
    } else {
        restartApiBackend();
    }
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

void BackendStatusViewModel::refreshDebugInfo() {
    if (!m_apiClient) {
        return;
    }
    m_isDebugLoading = true;
    emit debugInfoChanged();

    QPointer<BackendStatusViewModel> self(this);
    m_apiClient->get<nlohmann::json>(
        network::endpoints::api::HealthDebugInfo,
        [self](const std::variant<nlohmann::json, models::ApiError>& result) {
            if (!self) {
                return;
            }
            self->m_isDebugLoading = false;
            if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                QVariantMap dbMap;
                if (json->contains("database") && json->at("database").is_object()) {
                    const auto& db = json->at("database");
                    dbMap["mode"] = QString::fromStdString(db.value("mode", ""));
                    dbMap["provider"] = QString::fromStdString(db.value("provider", ""));
                    dbMap["useSqlite"] = db.value("useSqlite", false);
                    dbMap["encrypted"] = db.value("encrypted", false);
                    dbMap["canConnect"] = db.value("canConnect", false);
                    dbMap["cipher"] = db.contains("cipher") && !db.at("cipher").is_null()
                        ? QString::fromStdString(db.at("cipher").get<std::string>())
                        : QString();
                    dbMap["sqliteFile"] = QString::fromStdString(db.value("sqliteFile", ""));
                    if (db.contains("counts") && db.at("counts").is_object()) {
                        const auto& counts = db.at("counts");
                        dbMap["usersCount"] = counts.value("users", 0);
                        dbMap["interviewsCount"] = counts.value("interviews", 0);
                        dbMap["questionsCount"] = counts.value("questions_bank", 0);
                    }
                }
                self->m_debugDatabase = dbMap;

                QVariantMap aiMap;
                if (json->contains("ai") && json->at("ai").is_object()) {
                    const auto& ai = json->at("ai");
                    aiMap["provider"] = QString::fromStdString(ai.value("provider", ""));
                    aiMap["model"] = QString::fromStdString(ai.value("model", ""));
                    aiMap["ollamaUrl"] = QString::fromStdString(ai.value("ollamaUrl", ""));
                }
                self->m_debugAi = aiMap;

                QVariantList poolList;
                if (json->contains("questionsPool") && json->at("questionsPool").is_array()) {
                    for (const auto& item : json->at("questionsPool")) {
                        QVariantMap q;
                        q["direction"] = QString::fromStdString(item.value("direction", ""));
                        q["level"] = QString::fromStdString(item.value("level", ""));
                        q["topic"] = QString::fromStdString(item.value("topic", ""));
                        q["questionText"] = QString::fromStdString(item.value("questionText", ""));
                        q["hasExpectedAnswer"] = item.value("hasExpectedAnswer", false);
                        poolList.append(q);
                    }
                }
                self->m_debugQuestionsPool = poolList;

                QVariantList traceList;
                if (json->contains("recentTrace") && json->at("recentTrace").is_array()) {
                    for (const auto& t : json->at("recentTrace")) {
                        QVariantMap entry;
                        entry["timestamp"] = QString::fromStdString(t.value("timestamp", ""));
                        entry["stage"] = QString::fromStdString(t.value("stage", ""));
                        entry["detail"] = QString::fromStdString(t.value("detail", ""));
                        traceList.append(entry);
                    }
                }
                self->m_debugTrace = traceList;
            }
            emit self->debugInfoChanged();
        });
}

void BackendStatusViewModel::applyStatusJson(const nlohmann::json& json) {
    const bool required = json.value("required", true);
    const bool downloaded = json.value("modelDownloaded", false);
    const bool downloading = json.value("isDownloading", false);
    const int progress = json.value("downloadProgress", downloaded ? 100 : 0);
    const QString model = QString::fromStdString(json.value("model", "gemma4:e4b"));
    const QString status = QString::fromStdString(json.value("statusText", ""));
    const QString error = QString::fromStdString(json.value("error", ""));

    m_ollamaRunning = json.value("ollamaRunning", isTcpPortOpen(QStringLiteral("127.0.0.1"), 11434, 100));
    m_avatarServerReady = json.value("avatarServerReady", true);
    m_avatarHttp8080Running = json.value("avatarHttp8080Running", isTcpPortOpen(QStringLiteral("127.0.0.1"), 8080, 100));
    m_avatarTts8085Running = json.value("avatarTts8085Running", isTcpPortOpen(QStringLiteral("127.0.0.1"), 8085, 100));
    m_rhubarbAvailable = json.value("rhubarbAvailable", true);
    if (json.contains("avatarStatusText") && json["avatarStatusText"].is_string()) {
        m_avatarStatusText = QString::fromStdString(json.value("avatarStatusText", ""));
    }
    m_sttModelReady = json.value("sttModelReady", true);
    if (json.contains("sttModelName") && json["sttModelName"].is_string()) {
        m_sttModelName = QString::fromStdString(json.value("sttModelName", "parakeet-tdt-0.6b-v3"));
    }

    const bool ready = (!required || downloaded) && m_sttModelReady;
    applyLocalModelState(ready, true, downloading, progress, model, status, error);
    if (downloading) {
        if (!m_modelPollTimer.isActive()) {
            m_modelPollTimer.start();
        }
    } else {
        m_modelPollTimer.stop();
    }
}

void BackendStatusViewModel::refreshLocalModelStatus() {
    if (!m_apiClient) {
        return;
    }

    if (!m_apiClient->isOnline()) {
        checkDirectOllamaStatus();
        return;
    }

    QPointer<BackendStatusViewModel> self(this);
    m_apiClient->get<nlohmann::json>(
        network::endpoints::api::HealthLocalModel,
        [self](const std::variant<nlohmann::json, models::ApiError>& result) {
            if (!self) {
                return;
            }
            if (const auto* json = std::get_if<nlohmann::json>(&result)) {
                self->applyStatusJson(*json);
                return;
            }
            self->checkDirectOllamaStatus();
        });
}

void BackendStatusViewModel::downloadLocalModel() {
    if (m_isDownloadingModel) {
        return;
    }

    if (m_apiClient && m_apiClient->isOnline()) {
        applyLocalModelState(m_localModelReady, true, true, 2, m_localModelName,
                             tr("Starting local model download..."), QString());
        if (!m_modelPollTimer.isActive()) {
            m_modelPollTimer.start();
        }
        QPointer<BackendStatusViewModel> self(this);
        m_apiClient->post<nlohmann::json, nlohmann::json>(
            network::endpoints::api::HealthLocalModelDownload,
            nlohmann::json::object(),
            [self](const std::variant<nlohmann::json, models::ApiError>& result) {
                if (!self) {
                    return;
                }
                if (const auto* err = std::get_if<models::ApiError>(&result)) {
                    self->m_modelPollTimer.stop();
                    self->applyLocalModelState(false, true, false, 0, self->m_localModelName,
                                               tr("Download failed"), QString::fromStdString(err->message));
                    return;
                }
                self->refreshLocalModelStatus();
            });
        return;
    }

    // Fallback when API backend is offline: run scripts/install-local-model.ps1 or ollama pull locally.
    if (m_localDownloadProcess) {
        return;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList scriptCandidates = {
        appDir + QStringLiteral("/install-local-model.ps1"),
        appDir + QStringLiteral("/../../scripts/install-local-model.ps1"),
        appDir + QStringLiteral("/../../../scripts/install-local-model.ps1")
    };
    QString scriptPath;
    for (const QString& candidate : scriptCandidates) {
        if (QFileInfo::exists(candidate)) {
            scriptPath = QFileInfo(candidate).canonicalFilePath();
            break;
        }
    }

    m_localDownloadProcess = new QProcess(this);
    QPointer<BackendStatusViewModel> self(this);
    connect(m_localDownloadProcess, &QProcess::finished, this,
            [self](int exitCode, QProcess::ExitStatus status) {
                if (!self) {
                    return;
                }
                if (self->m_localDownloadProcess) {
                    self->m_localDownloadProcess->deleteLater();
                    self->m_localDownloadProcess = nullptr;
                }
                if (status != QProcess::NormalExit || exitCode != 0) {
                    self->applyLocalModelState(false, true, false, 0, self->m_localModelName,
                                               tr("Download failed"),
                                               tr("Local model installer exited with code %1.").arg(exitCode));
                } else {
                    self->checkDirectOllamaStatus();
                }
            });

    applyLocalModelState(false, true, true, 10, m_localModelName,
                         tr("Downloading Ollama / %1...").arg(m_localModelName), QString());

    if (!scriptPath.isEmpty()) {
        m_localDownloadProcess->start(QStringLiteral("powershell.exe"),
                                      {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"),
                                       QStringLiteral("Bypass"), QStringLiteral("-File"), scriptPath,
                                       QStringLiteral("-Model"), m_localModelName});
    } else {
        m_localDownloadProcess->start(QStringLiteral("ollama"), {QStringLiteral("pull"), m_localModelName});
    }

    if (!m_localDownloadProcess->waitForStarted(3000)) {
        m_localDownloadProcess->deleteLater();
        m_localDownloadProcess = nullptr;
        applyLocalModelState(false, true, false, 0, m_localModelName,
                             tr("Download failed"),
                             tr("Could not launch PowerShell or Ollama installer."));
    }
}

void BackendStatusViewModel::checkDirectOllamaStatus() {
    auto hasManifestOnDisk = [](const QString& modelName) {
        const QStringList parts = modelName.split(QLatin1Char(':'));
        const QString repo = parts.value(0);
        const QString tag = parts.size() > 1 ? parts.value(1) : QStringLiteral("latest");
        const QString manifestPath = QDir::homePath()
            + QStringLiteral("/.ollama/models/manifests/registry.ollama.ai/library/")
            + repo + QLatin1Char('/') + tag;
        return QFileInfo::exists(manifestPath);
    };

    if (!m_apiClient) {
        const bool onDisk = hasManifestOnDisk(m_localModelName);
        applyLocalModelState(onDisk, true, false, onDisk ? 100 : 0, m_localModelName,
                             onDisk ? tr("Ready") : tr("Model not downloaded"), QString());
        return;
    }

    QNetworkRequest req(QUrl(QStringLiteral("http://127.0.0.1:11434/api/tags")));
    QPointer<BackendStatusViewModel> self(this);
    const QString targetModel = m_localModelName;
    m_apiClient->executeRawRequest(
        req, "GET", QByteArray(), 1500,
        [self, targetModel, hasManifestOnDisk](int statusCode, const QByteArray& data, const QString& netErr) {
            if (!self) {
                return;
            }
            bool found = false;
            if (netErr.isEmpty() && statusCode >= 200 && statusCode < 300) {
                self->m_ollamaRunning = true;
                try {
                    const auto parsed = nlohmann::json::parse(data.constData(), data.constData() + data.size());
                    if (parsed.contains("models") && parsed["models"].is_array()) {
                        for (const auto& item : parsed["models"]) {
                            const std::string name = item.value("name", "");
                            const std::string model = item.value("model", "");
                            if (QString::fromStdString(name).compare(targetModel, Qt::CaseInsensitive) == 0
                                || QString::fromStdString(model).compare(targetModel, Qt::CaseInsensitive) == 0) {
                                found = true;
                                break;
                            }
                        }
                    }
                } catch (...) {
                }
            } else {
                self->m_ollamaRunning = false;
                found = hasManifestOnDisk(targetModel);
            }

            self->m_avatarHttp8080Running = isTcpPortOpen(QStringLiteral("127.0.0.1"), 8080, 100);
            self->m_avatarTts8085Running = isTcpPortOpen(QStringLiteral("127.0.0.1"), 8085, 100);
            self->m_avatarServerReady = self->m_avatarHttp8080Running || self->m_avatarTts8085Running || !self->isOffline();

            self->applyLocalModelState(found, true, self->m_isDownloadingModel,
                                       found ? 100 : self->m_modelDownloadProgress,
                                       targetModel,
                                       found ? tr("Ready") : tr("Model not downloaded"),
                                       QString());
        });
}

void BackendStatusViewModel::applyLocalModelState(bool ready, bool checked, bool downloading, int progress,
                                                  const QString& modelName, const QString& status, const QString& error) {
    m_localModelReady = ready;
    m_localModelChecked = checked;
    m_isDownloadingModel = downloading;
    m_modelDownloadProgress = progress;
    if (!modelName.isEmpty()) {
        m_localModelName = modelName;
    }
    m_localModelStatus = status;
    m_localModelError = error;
    emit localModelChanged();
}

QString BackendStatusViewModel::currentCommit() const {
#ifdef DEVDIALOGUE_GIT_COMMIT
    return QStringLiteral(DEVDIALOGUE_GIT_COMMIT);
#else
    return QStringLiteral("dev");
#endif
}

void BackendStatusViewModel::checkForUpdates(bool silent) {
    if (!m_apiClient) {
        return;
    }
    if (!silent) {
        m_updateStatusText = tr("Перевірка оновлень з гілки main...");
        emit updateStateChanged();
    }

    const QUrl manifestUrl(QStringLiteral(
        "https://github.com/iberikofer/dev-dialogue/releases/download/latest/latest.json"));
    QNetworkRequest req(manifestUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QPointer<BackendStatusViewModel> self(this);
    m_apiClient->executeRawRequest(
        req, "GET", QByteArray(), 6000,
        [self, silent](int statusCode, const QByteArray& data, const QString& netErr) {
            if (!self) {
                return;
            }
            if (!netErr.isEmpty() || statusCode < 200 || statusCode >= 300) {
                if (!silent) {
                    self->m_updateStatusText = tr("Не вдалося перевірити оновлення.");
                    emit self->updateStateChanged();
                }
                return;
            }
            try {
                const auto j = nlohmann::json::parse(data.constData(), data.constData() + data.size());
                const QString remoteVersion = QString::fromStdString(j.value("version", ""));
                const QString remoteCommit = QString::fromStdString(j.value("commit", ""));
                const QString installerUrl = QString::fromStdString(j.value("installerUrl", ""));
                const QString sha256 = QString::fromStdString(j.value("sha256", ""));

                const QString localCommit = self->currentCommit();
                const QString localVersion = QCoreApplication::applicationVersion();
                const bool commitChanged = !remoteCommit.isEmpty() && localCommit != QLatin1String("dev") &&
                                           remoteCommit.compare(localCommit, Qt::CaseInsensitive) != 0;
                const bool versionChanged = !remoteVersion.isEmpty() && remoteVersion != localVersion;

                self->m_latestVersion = remoteVersion;
                self->m_latestCommit = remoteCommit;
                self->m_latestInstallerUrl = installerUrl;
                self->m_latestInstallerSha256 = sha256;
                self->m_updateAvailable = !installerUrl.isEmpty() && (commitChanged || versionChanged);
                self->m_updateStatusText = self->m_updateAvailable
                    ? tr("Доступне оновлення %1 (%2)").arg(remoteVersion, remoteCommit)
                    : (silent ? QString() : tr("Встановлена актуальна версія (%1)").arg(localVersion));
                emit self->updateStateChanged();
            } catch (...) {
                if (!silent) {
                    self->m_updateStatusText = tr("Помилка формату маніфесту оновлень.");
                    emit self->updateStateChanged();
                }
            }
        });
}

void BackendStatusViewModel::installUpdate() {
    if (m_isUpdating || m_latestInstallerUrl.isEmpty()) {
        return;
    }

    m_isUpdating = true;
    m_updateProgress = 0;
    m_updateStatusText = tr("Завантаження інсталятора оновлення...");
    emit updateStateChanged();

    auto* nam = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(m_latestInstallerUrl)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = nam->get(req);
    QPointer<BackendStatusViewModel> self(this);

    connect(reply, &QNetworkReply::downloadProgress, this, [self](qint64 received, qint64 total) {
        if (!self || total <= 0) {
            return;
        }
        self->m_updateProgress = static_cast<int>(std::clamp((received * 100) / total, qint64(0), qint64(100)));
        self->m_updateStatusText = tr("Завантаження оновлення: %1%").arg(self->m_updateProgress);
        emit self->updateStateChanged();
    });

    connect(reply, &QNetworkReply::finished, this, [self, reply, nam]() {
        reply->deleteLater();
        nam->deleteLater();
        if (!self) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            self->m_isUpdating = false;
            self->m_updateStatusText = tr("Помилка завантаження: %1").arg(reply->errorString());
            emit self->updateStateChanged();
            return;
        }

        const QByteArray bytes = reply->readAll();
        if (!self->m_latestInstallerSha256.isEmpty()) {
            const QString actualHash = QString::fromLatin1(
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            if (actualHash.compare(self->m_latestInstallerSha256, Qt::CaseInsensitive) != 0) {
                self->m_isUpdating = false;
                self->m_updateStatusText = tr("Помилка перевірки SHA-256 хешу оновлення.");
                emit self->updateStateChanged();
                return;
            }
        }

        const QString tempInstaller = QDir::tempPath() + QStringLiteral("/DevDialogue-Setup-x64.exe");
        QFile file(tempInstaller);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            self->m_isUpdating = false;
            self->m_updateStatusText = tr("Не вдалося зберегти інсталятор у тимчасову папку.");
            emit self->updateStateChanged();
            return;
        }
        file.write(bytes);
        file.close();

        self->m_updateStatusText = tr("Запуск оновлення...");
        emit self->updateStateChanged();

        QProcess::startDetached(tempInstaller, {
            QStringLiteral("/SILENT"),
            QStringLiteral("/CLOSEAPPLICATIONS"),
            QStringLiteral("/RESTARTAPPLICATIONS")
        });
        QCoreApplication::quit();
    });
}

} // namespace devdialogue::viewmodels
