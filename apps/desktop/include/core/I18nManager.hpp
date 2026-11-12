#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>

class QQmlEngine;

namespace devdialogue::core {

/// Runtime language switching for the QML layer.
///
/// Translations are compiled into the binary as `:/i18n/devdialogue_<code>.qm`.
/// Installing a `QTranslator` and calling `QQmlEngine::retranslate()` refreshes
/// every `qsTr()` binding without recreating the engine, so an active interview
/// keeps its transcript and composer state while the UI language changes.
class I18nManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)
  Q_PROPERTY(QString currentLanguageLabel READ currentLanguageLabel NOTIFY
                 languageChanged)
  Q_PROPERTY(bool isUkrainian READ isUkrainian NOTIFY languageChanged)

public:
  explicit I18nManager(QObject *parent = nullptr);

  QString currentLanguage() const { return m_currentLanguage; }
  QString currentLanguageLabel() const;
  bool isUkrainian() const;

  /// Installs the persisted (or system) language. Call before loading QML so
  /// the first render is already translated.
  void initialize(QQmlEngine *engine);

  Q_INVOKABLE void setLanguage(const QString &languageCode);
  Q_INVOKABLE void toggleLanguage();

  Q_INVOKABLE QString languageLabel(const QString &languageCode) const;

signals:
  void languageChanged();

private:
  static QString normalize(const QString &languageCode);
  static QString systemLanguage();
  bool installTranslator(const QString &languageCode);

  QQmlEngine *m_engine{nullptr};
  QTranslator m_translator;
  QString m_currentLanguage{QStringLiteral("en")};
  bool m_translatorInstalled{false};
};

} // namespace devdialogue::core
