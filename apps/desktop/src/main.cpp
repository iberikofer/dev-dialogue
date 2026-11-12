#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <memory>

#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "core/I18nManager.hpp"
#include "network/ApiClient.hpp"
#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "services/InterviewService.hpp"
#include "services/SecureStorage.hpp"
#include "viewmodels/AdminViewModel.hpp"
#include "viewmodels/AuthViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"

int main(int argc, char *argv[]) {
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QGuiApplication app(argc, argv);
  app.setOrganizationName(
      QString::fromUtf8(devdialogue::core::constants::OrganizationName.data()));
  app.setOrganizationDomain(QString::fromUtf8(
      devdialogue::core::constants::OrganizationDomain.data()));
  app.setApplicationName(
      QString::fromUtf8(devdialogue::core::constants::AppName.data()));
  app.setApplicationVersion(
      QString::fromUtf8(devdialogue::core::constants::AppVersion.data()));

  QQuickStyle::setStyle("Basic");

  // ---------------------------------------------------------------------
  // Service graph (shared ownership keeps the async callbacks alive)
  // ---------------------------------------------------------------------
  auto appConfig = std::make_shared<devdialogue::core::AppConfig>();
  auto secureStorage =
      std::make_shared<devdialogue::services::SecureStorage>();
  auto apiClient = std::make_shared<devdialogue::network::ApiClient>(
      appConfig, secureStorage);
  auto authService = std::make_shared<devdialogue::services::AuthService>(
      apiClient, secureStorage, appConfig);
  auto interviewService =
      std::make_shared<devdialogue::services::InterviewService>(apiClient,
                                                                appConfig);
  auto adminService = std::make_shared<devdialogue::services::AdminService>(
      apiClient, appConfig);

  // ---------------------------------------------------------------------
  // ViewModels (parented to the application so QML bindings stay valid)
  // ---------------------------------------------------------------------
  auto navVM = std::make_unique<devdialogue::viewmodels::NavigationViewModel>();
  auto authVM = std::make_unique<devdialogue::viewmodels::AuthViewModel>(
      authService);
  auto dashboardVM =
      std::make_unique<devdialogue::viewmodels::DashboardViewModel>(
          interviewService, navVM.get());
  auto setupVM = std::make_unique<devdialogue::viewmodels::SetupViewModel>(
      interviewService, navVM.get());
  auto chatVM = std::make_unique<devdialogue::viewmodels::ChatViewModel>(
      interviewService, navVM.get());
  auto reportVM = std::make_unique<devdialogue::viewmodels::ReportViewModel>(
      interviewService, navVM.get());
  auto adminVM = std::make_unique<devdialogue::viewmodels::AdminViewModel>(
      adminService);

  // Declared before the engine so it outlives every QML binding that reads
  // the `i18n` context property during engine teardown.
  auto i18n = std::make_unique<devdialogue::core::I18nManager>();

  // A rejected bearer token invalidates the local session instead of leaving
  // the UI in a half-authenticated state.
  QObject::connect(apiClient.get(),
                   &devdialogue::network::ApiClient::unauthorizedDetected,
                   &app, [&authService]() { authService->logout(); });

  authService->restoreSession();

  QQmlApplicationEngine engine;
  QQmlContext *rootContext = engine.rootContext();

  i18n->initialize(&engine);
  rootContext->setContextProperty("i18n", i18n.get());

  rootContext->setContextProperty("apiClient", apiClient.get());
  rootContext->setContextProperty("navVM", navVM.get());
  rootContext->setContextProperty("authVM", authVM.get());
  rootContext->setContextProperty("dashboardVM", dashboardVM.get());
  rootContext->setContextProperty("setupVM", setupVM.get());
  rootContext->setContextProperty("chatVM", chatVM.get());
  rootContext->setContextProperty("reportVM", reportVM.get());
  rootContext->setContextProperty("adminVM", adminVM.get());

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

  return app.exec();
}
