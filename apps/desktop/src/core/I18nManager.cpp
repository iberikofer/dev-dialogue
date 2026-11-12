#include "core/I18nManager.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>
#include <QSettings>

namespace devdialogue::core {

namespace {
constexpr auto LanguageSettingKey = "ui/language";
constexpr char Ukrainian[] = "uk";
constexpr char English[] = "en";
} // namespace

I18nManager::I18nManager(QObject *parent) : QObject(parent) {}

QString I18nManager::normalize(const QString &languageCode) {
  const QString lowered = languageCode.trimmed().toLower();
  return lowered.startsWith(QLatin1String(Ukrainian))
             ? QString::fromLatin1(Ukrainian)
             : QString::fromLatin1(English);
}

QString I18nManager::systemLanguage() {
  return QLocale::system().language() == QLocale::Ukrainian
             ? QString::fromLatin1(Ukrainian)
             : QString::fromLatin1(English);
}

QString I18nManager::currentLanguageLabel() const {
  return languageLabel(m_currentLanguage);
}

bool I18nManager::isUkrainian() const {
  return m_currentLanguage == QLatin1String(Ukrainian);
}

QString I18nManager::languageLabel(const QString &languageCode) const {
  return normalize(languageCode) == QLatin1String(Ukrainian)
             ? QString::fromUtf8("Українська")
             : QStringLiteral("English");
}

bool I18nManager::installTranslator(const QString &languageCode) {
  if (m_translatorInstalled) {
    QCoreApplication::removeTranslator(&m_translator);
    m_translatorInstalled = false;
  }

  if (languageCode == QLatin1String(English))
    return true;

  const QString resource =
      QStringLiteral(":/i18n/devdialogue_%1.qm").arg(languageCode);
  if (!m_translator.load(resource))
    return false;

  m_translatorInstalled = QCoreApplication::installTranslator(&m_translator);
  return m_translatorInstalled;
}

void I18nManager::initialize(QQmlEngine *engine) {
  m_engine = engine;

  QSettings settings;
  const QString saved =
      settings.value(QString::fromLatin1(LanguageSettingKey)).toString();
  const QString language =
      saved.isEmpty() ? systemLanguage() : normalize(saved);

  m_currentLanguage = language;
  installTranslator(language);
}

void I18nManager::setLanguage(const QString &languageCode) {
  const QString language = normalize(languageCode);
  const bool wantsTranslator = language != QLatin1String(English);

  if (language == m_currentLanguage && m_translatorInstalled == wantsTranslator)
    return;

  if (!installTranslator(language) && wantsTranslator) {
    // The catalog is missing: keep the English source strings rather than
    // leaving the UI half-translated.
    m_currentLanguage = QString::fromLatin1(English);
  } else {
    m_currentLanguage = language;
  }

  QSettings settings;
  settings.setValue(QString::fromLatin1(LanguageSettingKey), m_currentLanguage);

  if (m_engine)
    m_engine->retranslate();

  emit languageChanged();
}

void I18nManager::toggleLanguage() {
  setLanguage(isUkrainian() ? QString::fromLatin1(English)
                            : QString::fromLatin1(Ukrainian));
}

} // namespace devdialogue::core
