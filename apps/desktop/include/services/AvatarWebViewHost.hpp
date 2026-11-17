#pragma once

#include <QJsonArray>
#include <QObject>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QUrl>

class QQuickWindow;

namespace devdialogue::services {

// Hosts the local Three.js 3D Avatar (model.html?embed=1) inside a native
// Edge WebView2 child HWND synced to the QML ChatRoomView avatar slot.
class AvatarWebViewHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY urlChanged)

public:
    explicit AvatarWebViewHost(QObject* parent = nullptr);
    ~AvatarWebViewHost() override;

    bool isReady() const { return m_pageReady; }
    bool isAvailable() const { return m_available; }
    QString currentUrl() const { return m_targetUrl; }

    Q_INVOKABLE void attachToWindow(QQuickWindow* window, const QString& baseUrl);
    Q_INVOKABLE void setBaseUrl(const QString& baseUrl);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void focusHostWindow();
    Q_INVOKABLE void setViewport(qreal x, qreal y, qreal width, qreal height, bool visible);
    Q_INVOKABLE void runScript(const QString& script);

    // Typed bridge helpers invoking window.devDialogueAvatar safely via JSON.
    Q_INVOKABLE void speak(const QString& text, const QString& emotion,
                           const QStringList& actionsSequence, const QJsonArray& uiCommands,
                           const QString& voice = QString(), const QString& gender = QString());
    Q_INVOKABLE void react(const QString& emotion, const QStringList& actionsSequence,
                           const QJsonArray& uiCommands, const QString& gender = QString());
    Q_INVOKABLE void stopSpeech();
    Q_INVOKABLE void setVolume(qreal volume);
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void setOutputMode(const QString& mode);
    Q_INVOKABLE void setSoundEffect(const QString& soundFile);
    Q_INVOKABLE void setVoice(const QString& voiceId);
    Q_INVOKABLE void setGender(const QString& gender);
    Q_INVOKABLE void setAccessory(const QString& name, bool visible);
    Q_INVOKABLE void setPreset(const QString& presetId);

signals:
    void readyChanged();
    void availableChanged();
    void urlChanged();

private:
    static QString buildEmbedUrl(const QString& baseUrl);
    void ensureInitialized();
    void updateHostBounds();
    void flushPendingScripts();
    void destroyNative();

    struct Impl;
    Impl* m_impl{nullptr};
    QQuickWindow* m_window{nullptr};
    QString m_targetUrl;
    QRect m_logicalBounds;
    bool m_visible{false};
    bool m_available{false};
    bool m_pageReady{false};
    bool m_initializing{false};
    QStringList m_pendingScripts;
};

} // namespace devdialogue::services
