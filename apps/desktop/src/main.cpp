#include <QGuiApplication>
#include <QIcon>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "network/ApiClient.hpp"
#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "services/AvatarWebViewHost.hpp"
#include "services/DemoPreparationService.hpp"
#include "services/InterviewService.hpp"
#include "services/MicrophoneService.hpp"
#include "services/QtKeychainSecureStorage.hpp"
#include "services/WebcamService.hpp"
#include "viewmodels/AdminViewModel.hpp"
#include "viewmodels/AuthViewModel.hpp"
#include "viewmodels/BackendStatusViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/FormValidator.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/PreparationViewModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/SettingsViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"
#include "viewmodels/SystemMonitorViewModel.hpp"

int main(int argc, char *argv[]) {
  using namespace devdialogue;

#ifdef _WIN32
  // Ensure plugins loaded from subdirectories (e.g. qml/, platforms/) can find
  // Qt and runtime DLLs in the application directory.
  wchar_t appPath[MAX_PATH];
  if (GetModuleFileNameW(nullptr, appPath, MAX_PATH) > 0) {
    if (auto *lastSlash = wcsrchr(appPath, L'\\')) {
      *lastSlash = L'\0';
      SetDllDirectoryW(appPath);
    }
  }
#endif

  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QGuiApplication app(argc, argv);
  app.setOrganizationName(
      QString::fromUtf8(core::constants::OrganizationName.data()));
  app.setOrganizationDomain(
      QString::fromUtf8(core::constants::OrganizationDomain.data()));
  app.setApplicationName(QString::fromUtf8(core::constants::AppName.data()));
  app.setApplicationVersion(
      QString::fromUtf8(core::constants::AppVersion.data()));

  app.setWindowIcon(QIcon(QStringLiteral(":/DevDialogue/App/resources/app/app-icon.png")));

  QQuickStyle::setStyle("Basic");

  // Core services
  auto appConfig = std::make_shared<core::AppConfig>();
  auto secureStorage = std::make_shared<services::QtKeychainSecureStorage>();
  auto apiClient =
      std::make_shared<network::ApiClient>(appConfig, secureStorage);
  auto authService = std::make_shared<services::AuthService>(
      apiClient, secureStorage, appConfig);
  auto interviewService =
      std::make_shared<services::InterviewService>(apiClient);
  auto adminService = std::make_shared<services::AdminService>(apiClient);
  auto avatarHost = std::make_unique<services::AvatarWebViewHost>();
  avatarHost->setBaseUrl(QString::fromStdString(appConfig->apiBaseUrl()));
  auto webcamService = std::make_unique<services::WebcamService>();
  webcamService->setApiBaseUrl(QString::fromStdString(appConfig->apiBaseUrl()));
  auto micService = std::make_unique<services::MicrophoneService>();
  micService->setApiBaseUrl(QString::fromStdString(appConfig->apiBaseUrl()));

  // View models exposed to QML
  auto navVM = std::make_unique<viewmodels::NavigationViewModel>();
  auto authVM = std::make_unique<viewmodels::AuthViewModel>(authService);
  auto settingsVM =
      std::make_unique<viewmodels::SettingsViewModel>(authService);
  settingsVM->installGlobalHotkeyFilter(&app);
  auto backendVM =
      std::make_unique<viewmodels::BackendStatusViewModel>(apiClient);
  backendVM->setLocalApiBaseUrl(QString::fromStdString(appConfig->apiBaseUrl()));
  auto validators = std::make_unique<viewmodels::FormValidator>();
  auto dashboardVM =
      std::make_unique<viewmodels::DashboardViewModel>(interviewService);
  auto setupVM = std::make_unique<viewmodels::SetupViewModel>(interviewService,
                                                              navVM.get());
  auto chatVM = std::make_unique<viewmodels::ChatViewModel>(interviewService,
                                                            navVM.get());
  auto reportVM =
      std::make_unique<viewmodels::ReportViewModel>(interviewService);
  auto adminVM =
      std::make_unique<viewmodels::AdminViewModel>(adminService, authService);
  auto preparationService =
      std::make_shared<services::DemoPreparationService>();
  auto preparationVM = std::make_unique<viewmodels::PreparationViewModel>(
      preparationService, navVM.get());
  auto systemMonitorVM =
      std::make_unique<viewmodels::SystemMonitorViewModel>();
  QObject::connect(settingsVM.get(),
                   &viewmodels::SettingsViewModel::systemMonitorToggleTriggered,
                   systemMonitorVM.get(),
                   &viewmodels::SystemMonitorViewModel::toggle);

  // While the backend is known to be down, screens go straight to their
  // local sample data instead of waiting for requests to time out.
  const auto backendOnline = [client = apiClient.get()]() {
    return client->isOnline();
  };
  dashboardVM->setBackendOnlineProbe(backendOnline);
  setupVM->setBackendOnlineProbe(backendOnline);
  setupVM->setLocalModelReadyProbe([b = backendVM.get()]() {
    return b->localModelReady();
  });
  setupVM->setEnsureServicesProbe([b = backendVM.get()]() {
    if (b) b->ensureAllServicesRunning();
  });
  chatVM->setBackendOnlineProbe(backendOnline);
  chatVM->setEnsureServicesProbe([b = backendVM.get()]() {
    if (b) b->ensureAllServicesRunning();
  });
  chatVM->setLocalModelReadyProbe([b = backendVM.get()]() {
    return b ? (!b->localModelChecked() || b->localModelReady()) : true;
  });
  chatVM->setVisualAssessmentProbe([cam = webcamService.get()]() {
    return cam ? cam->buildSessionVisualAssessment() : std::nullopt;
  });
  chatVM->setVisualContextProbe([cam = webcamService.get()]() {
    return cam ? cam->buildLiveVisualContext() : std::nullopt;
  });
  QObject::connect(settingsVM.get(), &viewmodels::SettingsViewModel::inputSettingsChanged,
                   webcamService.get(), [s = settingsVM.get(), cam = webcamService.get()]() {
                     if (s && cam) {
                       cam->setIncludeInScore(s->includeCameraInScore());
                     }
                   });
  QObject::connect(webcamService.get(), &services::WebcamService::visualMetricsChanged,
                   settingsVM.get(), [s = settingsVM.get(), cam = webcamService.get()]() {
                     if (s && cam && s->includeCameraInScore() != cam->includeInScore()) {
                       s->setIncludeCameraInScore(cam->includeInScore());
                     }
                   });
  reportVM->setBackendOnlineProbe(backendOnline);
  adminVM->setBackendOnlineProbe(backendOnline);

  // A session created on the setup screen opens in the chat room.
  QObject::connect(setupVM.get(),
                   &viewmodels::SetupViewModel::interviewStarted,
                   chatVM.get(), &viewmodels::ChatViewModel::beginSession);
  QObject::connect(setupVM.get(),
                   &viewmodels::SetupViewModel::interviewStarted,
                   webcamService.get(),
                   [cam = webcamService.get()](const QString& interviewId, const QString&,
                                               const QString&, const QString& language,
                                               const QString&, bool) {
                     if (cam) {
                       cam->setSessionContext(interviewId, language);
                     }
                   });

  // Refresh dashboard data automatically when auto-started backend comes online.
  QObject::connect(apiClient.get(), &network::ApiClient::onlineStatusChanged,
                   dashboardVM.get(),
                   [dashPtr = QPointer(dashboardVM.get()), authPtr = QPointer(authVM.get())](bool online) {
                     if (online && dashPtr && authPtr && authPtr->isAuthenticated()) {
                       dashPtr->refresh();
                     }
                   });

  QQmlApplicationEngine engine;
  engine.addImportPath(app.applicationDirPath() + QStringLiteral("/qml"));
  engine.addImportPath(app.applicationDirPath());
  engine.addImageProvider(
      QStringLiteral("webcam"),
      new services::WebcamImageProvider(webcamService->frameBuffer()));
  QQmlContext *rootContext = engine.rootContext();

  rootContext->setContextProperty("navVM", navVM.get());
  rootContext->setContextProperty("authVM", authVM.get());
  rootContext->setContextProperty("settingsVM", settingsVM.get());
  rootContext->setContextProperty("backendVM", backendVM.get());
  rootContext->setContextProperty("validators", validators.get());
  rootContext->setContextProperty("dashboardVM", dashboardVM.get());
  rootContext->setContextProperty("setupVM", setupVM.get());
  rootContext->setContextProperty("chatVM", chatVM.get());
  rootContext->setContextProperty("reportVM", reportVM.get());
  rootContext->setContextProperty("adminVM", adminVM.get());
  rootContext->setContextProperty("preparationVM", preparationVM.get());
  rootContext->setContextProperty("systemMonitorVM", systemMonitorVM.get());
  rootContext->setContextProperty("apiClient", apiClient.get());
  rootContext->setContextProperty("avatarHost", avatarHost.get());
  rootContext->setContextProperty("webcamService", webcamService.get());
  rootContext->setContextProperty("micService", micService.get());

  const QUrl url(QStringLiteral("qrc:/DevDialogue/App/qml/Main.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
          QCoreApplication::exit(-1);
        }
      },
      Qt::QueuedConnection);

  engine.load(url);

  // Pick up a session saved by a previous run.
  authService->restoreSession();

  return app.exec();
}
