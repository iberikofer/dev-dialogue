#pragma once

#include <QEvent>
#include <QObject>
#include <QString>
#include <memory>

#include "services/AuthService.hpp"

namespace devdialogue::viewmodels {

// Profile summary plus persisted voice/camera interaction mode and customizable
// hotkeys (Push-to-Talk vs Toggle mic, mic mute, camera toggle, AI voice mute).
class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(QString email READ email NOTIFY profileChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY profileChanged)
    Q_PROPERTY(QString role READ role NOTIFY profileChanged)
    Q_PROPERTY(bool isDemoAccount READ isDemoAccount NOTIFY profileChanged)

    Q_PROPERTY(QString micMode READ micMode WRITE setMicMode NOTIFY inputSettingsChanged)
    Q_PROPERTY(bool includeCameraInScore READ includeCameraInScore WRITE setIncludeCameraInScore NOTIFY inputSettingsChanged)
    Q_PROPERTY(QString micTalkShortcut READ micTalkShortcut WRITE setMicTalkShortcut NOTIFY shortcutsChanged)
    Q_PROPERTY(QString micMuteShortcut READ micMuteShortcut WRITE setMicMuteShortcut NOTIFY shortcutsChanged)
    Q_PROPERTY(QString cameraToggleShortcut READ cameraToggleShortcut WRITE setCameraToggleShortcut NOTIFY shortcutsChanged)
    Q_PROPERTY(QString aiMuteShortcut READ aiMuteShortcut WRITE setAiMuteShortcut NOTIFY shortcutsChanged)
    Q_PROPERTY(bool recordingShortcut READ isRecordingShortcut WRITE setRecordingShortcut NOTIFY recordingShortcutChanged)

public:
    explicit SettingsViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent = nullptr);

    QString displayName() const;
    QString email() const;
    QString avatarUrl() const;
    QString role() const;
    bool isDemoAccount() const;

    QString micMode() const { return m_micMode; }
    void setMicMode(const QString& mode);

    bool includeCameraInScore() const { return m_includeCameraInScore; }
    void setIncludeCameraInScore(bool include);

    QString micTalkShortcut() const { return m_micTalkShortcut; }
    void setMicTalkShortcut(const QString& shortcut);

    QString micMuteShortcut() const { return m_micMuteShortcut; }
    void setMicMuteShortcut(const QString& shortcut);

    QString cameraToggleShortcut() const { return m_cameraToggleShortcut; }
    void setCameraToggleShortcut(const QString& shortcut);

    QString aiMuteShortcut() const { return m_aiMuteShortcut; }
    void setAiMuteShortcut(const QString& shortcut);

    bool isRecordingShortcut() const { return m_recordingShortcut; }
    void setRecordingShortcut(bool recording);

    Q_INVOKABLE QString formatKeyEvent(int key, int modifiers) const;
    Q_INVOKABLE void resetShortcutsToDefaults();

    void installGlobalHotkeyFilter(QObject* appObject);

signals:
    void profileChanged();
    void inputSettingsChanged();
    void shortcutsChanged();
    void recordingShortcutChanged();

    void micTalkPressed();
    void micTalkReleased();
    void micMuteTriggered();
    void cameraToggleTriggered();
    void aiMuteTriggered();
    void systemMonitorToggleTriggered();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    static bool matchesShortcut(const QString& binding, int key, Qt::KeyboardModifiers modifiers,
                                bool& usesSafeModifierOrFn);

    std::shared_ptr<services::AuthService> m_authService;
    QString m_micMode{QStringLiteral("hold")};
    bool m_includeCameraInScore{true};
    QString m_micTalkShortcut{QStringLiteral("Alt+M")};
    QString m_micMuteShortcut{QStringLiteral("Ctrl+M")};
    QString m_cameraToggleShortcut{QStringLiteral("Alt+C")};
    QString m_aiMuteShortcut{QStringLiteral("Alt+S")};
    bool m_recordingShortcut{false};
    bool m_micHoldActive{false};
    int m_micHoldKey{0};
    Qt::KeyboardModifiers m_micHoldMods{Qt::NoModifier};
};

} // namespace devdialogue::viewmodels
