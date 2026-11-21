#pragma once

#include <QByteArray>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <atomic>

namespace devdialogue::services {

// Native Windows PCM16 16kHz mono microphone recorder + Parakeet TDT STT client.
class MicrophoneService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(bool transcribing READ isTranscribing NOTIFY transcribingChanged)
    Q_PROPERTY(bool testingMic READ isTestingMic NOTIFY testingMicChanged)
    Q_PROPERTY(qreal inputLevel READ inputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QVariantList availableMicrophones READ availableMicrophones NOTIFY devicesChanged)
    Q_PROPERTY(int selectedMicrophoneIndex READ selectedMicrophoneIndex NOTIFY selectedMicrophoneChanged)
    Q_PROPERTY(QString selectedMicrophoneName READ selectedMicrophoneName NOTIFY selectedMicrophoneChanged)
    Q_PROPERTY(bool permissionGranted READ isPermissionGranted NOTIFY permissionChanged)
    Q_PROPERTY(bool osPermissionAllowed READ isOsPermissionAllowed NOTIFY permissionChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    explicit MicrophoneService(QObject* parent = nullptr);
    ~MicrophoneService() override;

    void setApiBaseUrl(const QString& baseUrl);

    bool isRecording() const { return m_recording.load(); }
    bool isTranscribing() const { return m_transcribing; }
    bool isTestingMic() const { return m_testingMic.load(); }
    qreal inputLevel() const { return m_inputLevel; }
    QString statusText() const { return m_statusText; }
    QVariantList availableMicrophones() const { return m_availableMicrophones; }
    int selectedMicrophoneIndex() const { return m_selectedMicrophoneIndex.load(); }
    QString selectedMicrophoneName() const;
    bool isPermissionGranted() const { return m_permissionGranted; }
    bool isOsPermissionAllowed() const { return m_osPermissionAllowed; }
    bool isMuted() const { return m_muted; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopAndTranscribe();
    Q_INVOKABLE void toggleRecording();
    Q_INVOKABLE void cancelRecording();
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void toggleMuted();
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void selectMicrophone(int index);
    Q_INVOKABLE void grantPermission();
    Q_INVOKABLE void setPermissionGranted(bool granted);
    Q_INVOKABLE void openPrivacySettings();
    Q_INVOKABLE void startMicTest();
    Q_INVOKABLE void stopMicTest();
    Q_INVOKABLE void toggleMicTest();

signals:
    void recordingChanged();
    void transcribingChanged();
    void testingMicChanged();
    void inputLevelChanged();
    void statusChanged();
    void devicesChanged();
    void selectedMicrophoneChanged();
    void permissionChanged();
    void mutedChanged();
    void permissionRequired();
    void transcriptionReady(const QString& text);
    void noSpeechDetected();
    void transcriptionError(const QString& error);

private:
    void setTranscribing(bool busy);
    void setStatus(const QString& status);
    void updateInputLevel(qreal level);
    void updateOsPermissionState();
    bool openWaveDevice();
    void handleCapturedBlock(const char* data, int size, int bufferIndex);
    void stopHardwareCapture();
    void sendToParakeet(const QByteArray& pcmBytes);
    static QByteArray buildWavPayload(const QByteArray& pcmBytes, int sampleRate = 16000);

    struct Impl;
    Impl* m_impl{nullptr};
    QMutex m_pcmMutex;
    QByteArray m_pcmBuffer;
    QVariantList m_availableMicrophones;
    std::atomic<int> m_selectedMicrophoneIndex{0};
    bool m_permissionGranted{false};
    bool m_osPermissionAllowed{true};
    bool m_muted{false};
    QString m_apiBaseUrl{QStringLiteral("http://localhost:5238")};
    QString m_statusText;
    QNetworkAccessManager m_nam;
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_testingMic{false};
    std::atomic<bool> m_flushingTail{false};
    bool m_transcribing{false};
    qreal m_inputLevel{0.0};
};

} // namespace devdialogue::services
