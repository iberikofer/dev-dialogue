#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <memory>

#include "core/AppConfig.hpp"
#include "core/Constants.hpp"
#include "viewmodels/NavigationViewModel.hpp"

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

  auto appConfig = std::make_shared<devdialogue::core::AppConfig>();
  auto navVM = std::make_unique<devdialogue::viewmodels::NavigationViewModel>();

  QQmlApplicationEngine engine;
  QQmlContext *rootContext = engine.rootContext();

  rootContext->setContextProperty("navVM", navVM.get());

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
