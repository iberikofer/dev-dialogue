#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QQuickImageProvider>
#include <QString>
#include <QVariantList>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::services {

struct CameraDeviceDescriptor {
    enum class Backend { MediaFoundation, DirectShow };
    Backend backend{Backend::MediaFoundation};
    QString label;
    QString id;
};

struct WebcamFrameBuffer {
    QMutex mutex;
    QImage frame;
};

class WebcamImageProvider final : public QQuickImageProvider {
public:
    explicit WebcamImageProvider(std::shared_ptr<WebcamFrameBuffer> buffer)
        : QQuickImageProvider(QQuickImageProvider::Image), m_buffer(std::move(buffer)) {}

    QImage requestImage(const QString&, QSize* size, const QSize& requestedSize) override;

private:
    std::shared_ptr<WebcamFrameBuffer> m_buffer;
};

// Lightweight native Windows Media Foundation webcam capture for the
// Zoom candidate tile. Falls back cleanly with a status message if absent.
class WebcamService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY frameReady)
    Q_PROPERTY(int frameRevision READ frameRevision NOTIFY frameReady)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QVariantList availableCameras READ availableCameras NOTIFY devicesChanged)
    Q_PROPERTY(int selectedCameraIndex READ selectedCameraIndex NOTIFY selectedCameraChanged)
    Q_PROPERTY(QString selectedCameraName READ selectedCameraName NOTIFY selectedCameraChanged)
    Q_PROPERTY(bool permissionGranted READ isPermissionGranted NOTIFY permissionChanged)
    Q_PROPERTY(bool osPermissionAllowed READ isOsPermissionAllowed NOTIFY permissionChanged)

    // Hybrid 30 FPS instant CV + 10-frame lfm3-omni:2b visual quality evaluation
    Q_PROPERTY(int visualScore READ visualScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int presenceScore READ presenceScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int handsScore READ handsScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int lightingScore READ lightingScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int appearanceScore READ appearanceScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int postureScore READ postureScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(int emotionScore READ emotionScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(QString detectedEmotion READ detectedEmotion NOTIFY visualMetricsChanged)
    Q_PROPERTY(QString detectedEmotionLabel READ detectedEmotionLabel NOTIFY visualMetricsChanged)
    Q_PROPERTY(bool includeInScore READ includeInScore WRITE setIncludeInScore NOTIFY visualMetricsChanged)
    Q_PROPERTY(QString visualSummary READ visualSummary NOTIFY visualMetricsChanged)
    Q_PROPERTY(bool visionAnalyzing READ isVisionAnalyzing NOTIFY visualMetricsChanged)
    Q_PROPERTY(bool personPresent READ isPersonPresent NOTIFY visualMetricsChanged)
    Q_PROPERTY(int visionFps READ visionFps NOTIFY visualMetricsChanged)
    Q_PROPERTY(int visionSamplesCount READ visionSamplesCount NOTIFY visualMetricsChanged)
    Q_PROPERTY(QString visionModelName READ visionModelName CONSTANT)

public:
    explicit WebcamService(QObject* parent = nullptr);
    ~WebcamService() override;

    void setApiBaseUrl(const QString& baseUrl);
    void setSessionContext(const QString& interviewId, const QString& language);
    std::optional<models::VisualAssessment> buildSessionVisualAssessment() const;
    std::optional<models::VisualContext> buildLiveVisualContext() const;

    std::shared_ptr<WebcamFrameBuffer> frameBuffer() const { return m_buffer; }

    bool isActive() const { return m_active.load(); }
    bool hasFrame() const { return m_hasFrame.load(); }
    int frameRevision() const { return m_frameRevision; }
    QString statusText() const { return m_statusText; }
    QVariantList availableCameras() const { return m_availableCameras; }
    int selectedCameraIndex() const { return m_selectedCameraIndex.load(); }
    QString selectedCameraName() const;
    bool isPermissionGranted() const { return m_permissionGranted; }
    bool isOsPermissionAllowed() const { return m_osPermissionAllowed; }

    int visualScore() const { return m_visualScore; }
    int presenceScore() const { return m_presenceScore; }
    int handsScore() const { return m_handsScore; }
    int lightingScore() const { return m_lightingScore; }
    int appearanceScore() const { return m_appearanceScore; }
    int postureScore() const { return m_postureScore; }
    int emotionScore() const { return m_emotionScore; }
    QString detectedEmotion() const { return m_detectedEmotion; }
    QString detectedEmotionLabel() const;
    bool includeInScore() const { return m_includeInScore; }
    void setIncludeInScore(bool include);
    QString visualSummary() const { return m_visualSummary; }
    bool isVisionAnalyzing() const { return m_vlmInFlight.load(); }
    bool isPersonPresent() const { return m_personPresent; }
    int visionFps() const { return m_visionFps; }
    int visionSamplesCount() const { return m_sessionSamples; }
    QString visionModelName() const { return QStringLiteral("gemma4:e4b"); }

    Q_INVOKABLE void setActive(bool enabled);
    Q_INVOKABLE void toggleCamera();
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void selectCamera(int index);
    Q_INVOKABLE void grantPermission();
    Q_INVOKABLE void setPermissionGranted(bool granted);
    Q_INVOKABLE void openPrivacySettings();
    Q_INVOKABLE void resetSessionVisualMetrics();

signals:
    void activeChanged();
    void frameReady();
    void statusChanged();
    void devicesChanged();
    void selectedCameraChanged();
    void permissionChanged();
    void permissionRequired();
    void visualMetricsChanged();

private:
    void startCapture();
    void stopCapture();
    void setStatus(const QString& text);
    void updateOsPermissionState();
    void pushFrame(const QImage& img);
    void analyzeFrameInstant(const QImage& img);
    void sendFrameToVisionModel(const QByteArray& jpegBytes, int instantOverall, int instantLight,
                                int instantPresence, int instantHands, int instantPosture,
                                int instantEmotionScore, const QString& instantEmotion);
    void recomputeBlendedVisualScore();
    void captureThreadMain(CameraDeviceDescriptor device);
    void captureMediaFoundation(const CameraDeviceDescriptor& device);
    void captureDirectShow(const CameraDeviceDescriptor& device);

    std::shared_ptr<WebcamFrameBuffer> m_buffer;
    std::vector<CameraDeviceDescriptor> m_devices;
    QVariantList m_availableCameras;
    std::atomic<int> m_selectedCameraIndex{0};
    bool m_permissionGranted{false};
    bool m_osPermissionAllowed{true};
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_hasFrame{false};
    int m_frameRevision{0};
    QString m_statusText;
    std::thread m_worker;

    QString m_apiBaseUrl{QStringLiteral("http://localhost:5238")};
    QString m_interviewId;
    QString m_sessionLanguage{QStringLiteral("uk")};
    QNetworkAccessManager m_nam;

    std::array<uchar, 64 * 48> m_prevLumaGrid{};
    bool m_hasPrevGrid{false};
    quint64 m_workerFrameCounter{0};
    QElapsedTimer m_fpsTimer;
    int m_fpsFrameAccum{0};
    std::atomic<bool> m_vlmInFlight{false};

    int m_instantOverall{0};
    int m_instantLighting{0};
    int m_instantPresence{0};
    int m_instantMotion{0};
    int m_instantPosture{0};
    int m_instantAppearance{0};
    int m_instantEmotionScore{0};
    QString m_instantEmotion{QStringLiteral("absent")};
    int m_vlmOverall{0};
    bool m_vlmPersonPresent{false};
    bool m_hasVlmResult{false};

    int m_visualScore{0};
    int m_presenceScore{0};
    int m_handsScore{0};
    int m_lightingScore{0};
    int m_appearanceScore{0};
    int m_postureScore{0};
    int m_emotionScore{0};
    QString m_detectedEmotion{QStringLiteral("absent")};
    bool m_includeInScore{true};
    QString m_visualSummary;
    bool m_personPresent{false};
    int m_visionFps{0};

    int m_sessionSamples{0};
    long long m_sumOverall{0};
    long long m_sumPresence{0};
    long long m_sumHands{0};
    long long m_sumLighting{0};
    long long m_sumAppearance{0};
    long long m_sumPosture{0};
    long long m_sumEmotion{0};
    QString m_lastVlmModel{QStringLiteral("gemma4:e4b")};
};

} // namespace devdialogue::services
