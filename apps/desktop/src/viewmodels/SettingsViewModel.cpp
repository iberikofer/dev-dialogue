#include "viewmodels/SettingsViewModel.hpp"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QSettings>

namespace devdialogue::viewmodels {

namespace {

constexpr const char* DemoUserId = "mock-user-123";

constexpr const char* DefaultMicMode = "hold";
constexpr const char* DefaultMicTalkShortcut = "Alt+M";
constexpr const char* DefaultMicMuteShortcut = "Ctrl+M";
constexpr const char* DefaultCameraToggleShortcut = "Alt+C";
constexpr const char* DefaultAiMuteShortcut = "Alt+S";

bool isModifierOnlyKey(int key) {
    return key == Qt::Key_Shift || key == Qt::Key_Control ||
           key == Qt::Key_Alt || key == Qt::Key_AltGr ||
           key == Qt::Key_Meta || key == Qt::Key_Super_L || key == Qt::Key_Super_R;
}

Qt::KeyboardModifiers cleanModifiers(int rawMods) {
    return static_cast<Qt::KeyboardModifiers>(rawMods) &
           (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);
}

QString normalizeShortcut(const QString& raw) {
    const QKeySequence seq = QKeySequence::fromString(raw.trimmed(), QKeySequence::PortableText);
    return seq.isEmpty() ? raw.trimmed() : seq.toString(QKeySequence::PortableText);
}

} // namespace

SettingsViewModel::SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_authService(std::move(authService)) {
    if (m_authService) {
        connect(m_authService.get(), &services::AuthService::userChanged, this, &SettingsViewModel::profileChanged);
    }

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    const QString savedMode = settings.value(QStringLiteral("input/micMode"), QLatin1String(DefaultMicMode)).toString();
    m_micMode = (savedMode == QLatin1String("toggle")) ? QStringLiteral("toggle") : QStringLiteral("hold");
    m_includeCameraInScore = settings.value(QStringLiteral("input/includeCameraInScore"), true).toBool();
    m_micTalkShortcut = settings.value(QStringLiteral("shortcuts/micTalk"), QLatin1String(DefaultMicTalkShortcut)).toString();
    m_micMuteShortcut = settings.value(QStringLiteral("shortcuts/micMute"), QLatin1String(DefaultMicMuteShortcut)).toString();
    m_cameraToggleShortcut = settings.value(QStringLiteral("shortcuts/cameraToggle"), QLatin1String(DefaultCameraToggleShortcut)).toString();
    m_aiMuteShortcut = settings.value(QStringLiteral("shortcuts/aiMute"), QLatin1String(DefaultAiMuteShortcut)).toString();
}

QString SettingsViewModel::displayName() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    if (!user.has_value()) {
        return QString();
    }
    if (user->id == DemoUserId) {
        return m_authService->isAdmin() ? tr("Demo admin") : tr("Demo candidate");
    }
    if (user->fullName.has_value() && !user->fullName->empty()) {
        return QString::fromStdString(*user->fullName);
    }
    return QString::fromStdString(user->email).section(QLatin1Char('@'), 0, 0);
}

QString SettingsViewModel::email() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    return user.has_value() ? QString::fromStdString(user->email) : QString();
}

QString SettingsViewModel::avatarUrl() const {
    if (!m_authService) {
        return QString();
    }
    const auto user = m_authService->currentUser();
    if (user.has_value() && user->avatarUrl.has_value()) {
        return QString::fromStdString(*user->avatarUrl);
    }
    return QString();
}

QString SettingsViewModel::role() const {
    return m_authService && m_authService->isAdmin() ? QStringLiteral("admin") : QStringLiteral("user");
}

bool SettingsViewModel::isDemoAccount() const {
    if (!m_authService) {
        return false;
    }
    const auto user = m_authService->currentUser();
    return user.has_value() && user->id == DemoUserId;
}

void SettingsViewModel::setMicMode(const QString& mode) {
    const QString normalized = (mode == QLatin1String("toggle")) ? QStringLiteral("toggle") : QStringLiteral("hold");
    if (m_micMode != normalized) {
        m_micMode = normalized;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("input/micMode"), normalized);
        emit inputSettingsChanged();
    }
}

void SettingsViewModel::setIncludeCameraInScore(bool include) {
    if (m_includeCameraInScore != include) {
        m_includeCameraInScore = include;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("input/includeCameraInScore"), include);
        emit inputSettingsChanged();
    }
}

void SettingsViewModel::setMicTalkShortcut(const QString& shortcut) {
    const QString clean = normalizeShortcut(shortcut);
    if (!clean.isEmpty() && m_micTalkShortcut != clean) {
        m_micTalkShortcut = clean;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("shortcuts/micTalk"), clean);
        emit shortcutsChanged();
    }
}

void SettingsViewModel::setMicMuteShortcut(const QString& shortcut) {
    const QString clean = normalizeShortcut(shortcut);
    if (!clean.isEmpty() && m_micMuteShortcut != clean) {
        m_micMuteShortcut = clean;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("shortcuts/micMute"), clean);
        emit shortcutsChanged();
    }
}

void SettingsViewModel::setCameraToggleShortcut(const QString& shortcut) {
    const QString clean = normalizeShortcut(shortcut);
    if (!clean.isEmpty() && m_cameraToggleShortcut != clean) {
        m_cameraToggleShortcut = clean;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("shortcuts/cameraToggle"), clean);
        emit shortcutsChanged();
    }
}

void SettingsViewModel::setAiMuteShortcut(const QString& shortcut) {
    const QString clean = normalizeShortcut(shortcut);
    if (!clean.isEmpty() && m_aiMuteShortcut != clean) {
        m_aiMuteShortcut = clean;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("shortcuts/aiMute"), clean);
        emit shortcutsChanged();
    }
}

void SettingsViewModel::setRecordingShortcut(bool recording) {
    if (m_recordingShortcut != recording) {
        m_recordingShortcut = recording;
        emit recordingShortcutChanged();
    }
}

QString SettingsViewModel::formatKeyEvent(int key, int modifiers) const {
    if (key == 0 || key == Qt::Key_unknown || isModifierOnlyKey(key)) {
        return QString();
    }
    const Qt::KeyboardModifiers mods = cleanModifiers(modifiers);
    const QKeySequence seq(QKeyCombination(mods, static_cast<Qt::Key>(key)));
    return seq.toString(QKeySequence::PortableText);
}

void SettingsViewModel::resetShortcutsToDefaults() {
    m_micTalkShortcut = QLatin1String(DefaultMicTalkShortcut);
    m_micMuteShortcut = QLatin1String(DefaultMicMuteShortcut);
    m_cameraToggleShortcut = QLatin1String(DefaultCameraToggleShortcut);
    m_aiMuteShortcut = QLatin1String(DefaultAiMuteShortcut);

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("shortcuts/micTalk"), m_micTalkShortcut);
    settings.setValue(QStringLiteral("shortcuts/micMute"), m_micMuteShortcut);
    settings.setValue(QStringLiteral("shortcuts/cameraToggle"), m_cameraToggleShortcut);
    settings.setValue(QStringLiteral("shortcuts/aiMute"), m_aiMuteShortcut);
    emit shortcutsChanged();
}

void SettingsViewModel::installGlobalHotkeyFilter(QObject* appObject) {
    if (appObject) {
        appObject->installEventFilter(this);
    }
}

bool SettingsViewModel::matchesShortcut(const QString& binding, int key,
                                        Qt::KeyboardModifiers modifiers,
                                        bool& usesSafeModifierOrFn) {
    if (binding.isEmpty() || key == 0 || key == Qt::Key_unknown || isModifierOnlyKey(key)) {
        return false;
    }
    const QKeySequence target = QKeySequence::fromString(binding, QKeySequence::PortableText);
    if (target.isEmpty()) {
        return false;
    }
    const QKeyCombination pressed(cleanModifiers(static_cast<int>(modifiers)), static_cast<Qt::Key>(key));
    if (target[0] != pressed) {
        return false;
    }
    const Qt::KeyboardModifiers mods = pressed.keyboardModifiers();
    const bool hasCtrlAltMeta = (mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) != 0;
    const bool isFunctionKey = (key >= Qt::Key_F1 && key <= Qt::Key_F35);
    usesSafeModifierOrFn = hasCtrlAltMeta || isFunctionKey;
    return true;
}

bool SettingsViewModel::eventFilter(QObject* watched, QEvent* event) {
    if (!event || m_recordingShortcut) {
        return QObject::eventFilter(watched, event);
    }

    if (event->type() == QEvent::KeyRelease && m_micHoldActive) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (!ke->isAutoRepeat()) {
            const Qt::KeyboardModifiers mods = cleanModifiers(static_cast<int>(ke->modifiers()));
            if (ke->key() == m_micHoldKey ||
                (m_micHoldMods != Qt::NoModifier && (mods & m_micHoldMods) != m_micHoldMods)) {
                m_micHoldActive = false;
                m_micHoldKey = 0;
                m_micHoldMods = Qt::NoModifier;
                emit micTalkReleased();
            }
        }
        return QObject::eventFilter(watched, event);
    }

    if (event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(watched, event);
    }

    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->isAutoRepeat() || isModifierOnlyKey(ke->key())) {
        return QObject::eventFilter(watched, event);
    }

    const Qt::KeyboardModifiers mods = cleanModifiers(static_cast<int>(ke->modifiers()));
    if ((mods & Qt::ShiftModifier) && (mods & Qt::AltModifier) &&
        (ke->key() == Qt::Key_X || ke->nativeVirtualKey() == 0x58)) {
        emit systemMonitorToggleTriggered();
        return true;
    }

    QObject* focusObj = QGuiApplication::focusObject();
    const bool inTextInput = focusObj &&
        (focusObj->inherits("QQuickTextEdit") || focusObj->inherits("QQuickTextInput"));

    bool safeInEditor = false;
    if (matchesShortcut(m_micTalkShortcut, ke->key(), mods, safeInEditor)) {
        if (inTextInput && !safeInEditor) {
            return QObject::eventFilter(watched, event);
        }
        m_micHoldActive = true;
        m_micHoldKey = ke->key();
        m_micHoldMods = mods;
        emit micTalkPressed();
        return true;
    }

    if (matchesShortcut(m_micMuteShortcut, ke->key(), mods, safeInEditor)) {
        if (inTextInput && !safeInEditor) {
            return QObject::eventFilter(watched, event);
        }
        emit micMuteTriggered();
        return true;
    }

    if (matchesShortcut(m_cameraToggleShortcut, ke->key(), mods, safeInEditor)) {
        if (inTextInput && !safeInEditor) {
            return QObject::eventFilter(watched, event);
        }
        emit cameraToggleTriggered();
        return true;
    }

    if (matchesShortcut(m_aiMuteShortcut, ke->key(), mods, safeInEditor)) {
        if (inTextInput && !safeInEditor) {
            return QObject::eventFilter(watched, event);
        }
        emit aiMuteTriggered();
        return true;
    }

    return QObject::eventFilter(watched, event);
}

} // namespace devdialogue::viewmodels
