#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <memory>

#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "network/ApiClient.hpp"
#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "services/InterviewService.hpp"
#include "services/QtKeychainSecureStorage.hpp"
#include "viewmodels/AdminViewModel.hpp"
#include "viewmodels/AuthViewModel.hpp"
#include "viewmodels/BackendStatusViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/FormValidator.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/SettingsViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"

int main(int argc, char *argv[]) {
  using namespace devdialogue;

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

  // View models exposed to QML
  auto navVM = std::make_unique<viewmodels::NavigationViewModel>();
  auto authVM = std::make_unique<viewmodels::AuthViewModel>(authService);
  auto settingsVM =
      std::make_unique<viewmodels::SettingsViewModel>(authService);
  auto backendVM =
      std::make_unique<viewmodels::BackendStatusViewModel>(apiClient);
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

  // While the backend is known to be down, screens go straight to their
  // local sample data instead of waiting for requests to time out.
  const auto backendOnline = [client = apiClient.get()]() {
    return client->isOnline();
  };
  dashboardVM->setBackendOnlineProbe(backendOnline);
  chatVM->setBackendOnlineProbe(backendOnline);
  reportVM->setBackendOnlineProbe(backendOnline);
  adminVM->setBackendOnlineProbe(backendOnline);

  // A session created on the setup screen opens in the chat room.
  QObject::connect(setupVM.get(),
                   &viewmodels::SetupViewModel::interviewStarted,
                   chatVM.get(), &viewmodels::ChatViewModel::beginSession);

  QQmlApplicationEngine engine;
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
  rootContext->setContextProperty("apiClient", apiClient.get());

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
