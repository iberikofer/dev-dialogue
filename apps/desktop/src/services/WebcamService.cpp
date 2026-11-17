#include "services/WebcamService.hpp"

#include <QBuffer>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <qedit.h>
#endif

namespace devdialogue::services {

namespace {

bool readWindowsWebcamConsent() {
#ifdef Q_OS_WIN
    const QSettings hkcu(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam"),
        QSettings::NativeFormat);
    if (hkcu.value(QStringLiteral("Value"), QStringLiteral("Allow")).toString()
            .compare(QStringLiteral("Deny"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    const QSettings hklm(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam"),
        QSettings::NativeFormat);
    return hklm.value(QStringLiteral("Value"), QStringLiteral("Allow")).toString()
               .compare(QStringLiteral("Deny"), Qt::CaseInsensitive) != 0;
#else
    return true;
#endif
}

#ifdef Q_OS_WIN
constexpr CLSID kClsidSampleGrabber = {
    0xc1f400a0, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}
};
constexpr CLSID kClsidNullRenderer = {
    0xc1f400a4, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}
};
constexpr GUID kSubtypeI420 = {
    0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
};

inline uchar clampByte(int v) {
    return static_cast<uchar>(std::clamp(v, 0, 255));
}

inline QRgb yuvToRgb(int y, int u, int v) {
    const int c = y - 16;
    const int d = u - 128;
    const int e = v - 128;
    const int r = (298 * c + 409 * e + 128) >> 8;
    const int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
    const int b = (298 * c + 516 * d + 128) >> 8;
    return qRgb(clampByte(r), clampByte(g), clampByte(b));
}

QImage decodeVideoFrameToQImage(const BYTE* data, DWORD curLen,
                                UINT32 width, UINT32 height,
                                LONG stride, const GUID& subtype,
                                bool bottomUpRgb) {
    if (!data || width == 0 || height == 0 || curLen == 0) {
        return {};
    }

    if (IsEqualGUID(subtype, MFVideoFormat_RGB32) ||
        IsEqualGUID(subtype, MEDIASUBTYPE_RGB32) ||
        IsEqualGUID(subtype, MFVideoFormat_ARGB32) ||
        IsEqualGUID(subtype, MEDIASUBTYPE_ARGB32)) {
        QImage img(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
        const LONG absStride = stride != 0 ? std::abs(stride) : static_cast<LONG>(width * 4);
        const bool flipVertical = (stride < 0) || (stride > 0 && bottomUpRgb);
        const int rowBytes = static_cast<int>(width * 4);
        for (UINT32 y = 0; y < height; ++y) {
            const UINT32 srcY = flipVertical ? (height - 1 - y) : y;
            const SIZE_T offset = static_cast<SIZE_T>(srcY) * static_cast<SIZE_T>(absStride);
            if (offset + static_cast<SIZE_T>(rowBytes) <= curLen) {
                std::memcpy(img.scanLine(static_cast<int>(y)), data + offset, static_cast<size_t>(rowBytes));
            }
        }
        return img;
    }

    if (IsEqualGUID(subtype, MFVideoFormat_RGB24) ||
        IsEqualGUID(subtype, MEDIASUBTYPE_RGB24)) {
        QImage img(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
        const LONG minStride = static_cast<LONG>(((width * 3 + 3) / 4) * 4);
        const LONG absStride = stride != 0 ? std::abs(stride) : minStride;
        const bool flipVertical = (stride < 0) || (stride > 0 && bottomUpRgb);
        for (UINT32 y = 0; y < height; ++y) {
            const UINT32 srcY = flipVertical ? (height - 1 - y) : y;
            const SIZE_T offset = static_cast<SIZE_T>(srcY) * static_cast<SIZE_T>(absStride);
            if (offset + static_cast<SIZE_T>(width * 3) <= curLen) {
                const BYTE* src = data + offset;
                auto* dst = reinterpret_cast<QRgb*>(img.scanLine(static_cast<int>(y)));
                for (UINT32 x = 0; x < width; ++x) {
                    dst[x] = qRgb(src[x * 3 + 2], src[x * 3 + 1], src[x * 3 + 0]);
                }
            }
        }
        return img;
    }

    if (IsEqualGUID(subtype, MFVideoFormat_NV12)) {
        const SIZE_T yStride = stride != 0 ? static_cast<SIZE_T>(std::abs(stride)) : static_cast<SIZE_T>(width);
        const SIZE_T yPlaneBytes = yStride * height;
        const SIZE_T uvPlaneBytes = yStride * (height / 2);
        if (curLen < yPlaneBytes + uvPlaneBytes) {
            return {};
        }
        const UINT32 step = (width >= 1920 && height >= 1080) ? 2U : 1U;
        const UINT32 outW = width / step;
        const UINT32 outH = height / step;
        QImage img(static_cast<int>(outW), static_cast<int>(outH), QImage::Format_RGB32);
        const BYTE* yPlane = data;
        const BYTE* uvPlane = data + yPlaneBytes;
        for (UINT32 oy = 0; oy < outH; ++oy) {
            const UINT32 y = oy * step;
            const BYTE* yRow = yPlane + static_cast<SIZE_T>(y) * yStride;
            const BYTE* uvRow = uvPlane + static_cast<SIZE_T>(y / 2) * yStride;
            auto* dst = reinterpret_cast<QRgb*>(img.scanLine(static_cast<int>(oy)));
            for (UINT32 ox = 0; ox < outW; ++ox) {
                const UINT32 x = ox * step;
                const SIZE_T uvIdx = static_cast<SIZE_T>(x & ~1U);
                dst[ox] = yuvToRgb(yRow[x], uvRow[uvIdx], uvRow[uvIdx + 1]);
            }
        }
        return img;
    }

    if (IsEqualGUID(subtype, MFVideoFormat_YUY2)) {
        const SIZE_T rowStride = stride != 0 ? static_cast<SIZE_T>(std::abs(stride)) : static_cast<SIZE_T>(width * 2);
        if (curLen < rowStride * height) {
            return {};
        }
        QImage img(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
        for (UINT32 y = 0; y < height; ++y) {
            const BYTE* src = data + static_cast<SIZE_T>(y) * rowStride;
            auto* dst = reinterpret_cast<QRgb*>(img.scanLine(static_cast<int>(y)));
            for (UINT32 x = 0; x + 1 < width; x += 2) {
                const SIZE_T idx = static_cast<SIZE_T>(x) * 2;
                const int y0 = src[idx + 0];
                const int u  = src[idx + 1];
                const int y1 = src[idx + 2];
                const int v  = src[idx + 3];
                dst[x] = yuvToRgb(y0, u, v);
                dst[x + 1] = yuvToRgb(y1, u, v);
            }
        }
        return img;
    }

    if (IsEqualGUID(subtype, MFVideoFormat_I420) ||
        IsEqualGUID(subtype, MFVideoFormat_IYUV) ||
        IsEqualGUID(subtype, kSubtypeI420)) {
        const SIZE_T yStride = stride != 0 ? static_cast<SIZE_T>(std::abs(stride)) : static_cast<SIZE_T>(width);
        const SIZE_T uvStride = yStride / 2;
        const SIZE_T yBytes = yStride * height;
        const SIZE_T uvBytes = uvStride * (height / 2);
        if (curLen < yBytes + uvBytes * 2) {
            return {};
        }
        QImage img(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
        const BYTE* yPlane = data;
        const BYTE* uPlane = data + yBytes;
        const BYTE* vPlane = uPlane + uvBytes;
        for (UINT32 y = 0; y < height; ++y) {
            const BYTE* yRow = yPlane + static_cast<SIZE_T>(y) * yStride;
            const BYTE* uRow = uPlane + static_cast<SIZE_T>(y / 2) * uvStride;
            const BYTE* vRow = vPlane + static_cast<SIZE_T>(y / 2) * uvStride;
            auto* dst = reinterpret_cast<QRgb*>(img.scanLine(static_cast<int>(y)));
            for (UINT32 x = 0; x < width; ++x) {
                dst[x] = yuvToRgb(yRow[x], uRow[x / 2], vRow[x / 2]);
            }
        }
        return img;
    }

    return {};
}

QString readMfAttrString(IMFActivate* dev, const GUID& key) {
    WCHAR* raw = nullptr;
    UINT32 len = 0;
    if (SUCCEEDED(dev->GetAllocatedString(key, &raw, &len)) && raw) {
        const QString s = QString::fromWCharArray(raw, static_cast<int>(len)).trimmed();
        CoTaskMemFree(raw);
        return s;
    }
    return {};
}

class SampleGrabberCallback final : public ISampleGrabberCB {
public:
    using FrameHandler = std::function<void(const BYTE*, LONG)>;

    explicit SampleGrabberCallback(FrameHandler handler)
        : m_handler(std::move(handler)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
            *ppv = static_cast<ISampleGrabberCB*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(++m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const LONG ref = --m_refCount;
        if (ref == 0) {
            delete this;
            return 0;
        }
        return static_cast<ULONG>(ref);
    }

    HRESULT STDMETHODCALLTYPE SampleCB(double, IMediaSample*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE BufferCB(double, BYTE* pBuffer, LONG bufferLen) override {
        if (m_handler && pBuffer && bufferLen > 0) {
            m_handler(pBuffer, bufferLen);
        }
        return S_OK;
    }

private:
    std::atomic<LONG> m_refCount{1};
    FrameHandler m_handler;
};

void freeMediaType(AM_MEDIA_TYPE& mt) {
    if (mt.cbFormat != 0 && mt.pbFormat) {
        CoTaskMemFree(mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = nullptr;
    }
    if (mt.pUnk) {
        mt.pUnk->Release();
        mt.pUnk = nullptr;
    }
}

bool extractMediaTypeInfo(const AM_MEDIA_TYPE& mt, UINT32& width, UINT32& height,
                          LONG& stride, bool& bottomUpRgb, GUID& subtype) {
    subtype = mt.subtype;
    const BITMAPINFOHEADER* bmi = nullptr;
    if (IsEqualGUID(mt.formattype, FORMAT_VideoInfo) && mt.pbFormat &&
        mt.cbFormat >= sizeof(VIDEOINFOHEADER)) {
        bmi = &reinterpret_cast<const VIDEOINFOHEADER*>(mt.pbFormat)->bmiHeader;
    } else if (IsEqualGUID(mt.formattype, FORMAT_VideoInfo2) && mt.pbFormat &&
               mt.cbFormat >= sizeof(VIDEOINFOHEADER2)) {
        bmi = &reinterpret_cast<const VIDEOINFOHEADER2*>(mt.pbFormat)->bmiHeader;
    }
    if (!bmi || bmi->biWidth <= 0 || bmi->biHeight == 0) {
        return false;
    }
    width = static_cast<UINT32>(bmi->biWidth);
    height = static_cast<UINT32>(std::abs(bmi->biHeight));
    bottomUpRgb = (bmi->biHeight > 0);
    stride = 0;
    return true;
}
#endif

} // namespace

QImage WebcamImageProvider::requestImage(const QString&, QSize* size, const QSize& requestedSize) {
    QImage copy;
    if (m_buffer) {
        QMutexLocker locker(&m_buffer->mutex);
        copy = m_buffer->frame;
    }
    if (copy.isNull()) {
        copy = QImage(2, 2, QImage::Format_RGBA8888);
        copy.fill(Qt::transparent);
    } else if (requestedSize.isValid() && !requestedSize.isEmpty()) {
        copy = copy.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (size) {
        *size = copy.size();
    }
    return copy;
}

WebcamService::WebcamService(QObject* parent)
    : QObject(parent), m_buffer(std::make_shared<WebcamFrameBuffer>()) {
    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    m_permissionGranted = settings.value(QStringLiteral("permissions/webcamGranted"), false).toBool();
    m_includeInScore = settings.value(QStringLiteral("input/includeCameraInScore"), true).toBool();
    refreshDevices();
}

WebcamService::~WebcamService() {
    stopCapture();
}

QString WebcamService::selectedCameraName() const {
    const int idx = m_selectedCameraIndex.load();
    if (idx >= 0 && idx < m_availableCameras.size()) {
        return m_availableCameras[idx].toMap().value(QStringLiteral("label")).toString();
    }
    return QString();
}

void WebcamService::updateOsPermissionState() {
    const bool osAllowed = readWindowsWebcamConsent();
    if (m_osPermissionAllowed != osAllowed) {
        m_osPermissionAllowed = osAllowed;
        emit permissionChanged();
    }
}

void WebcamService::refreshDevices() {
    updateOsPermissionState();
    std::vector<CameraDeviceDescriptor> discovered;
    QVariantList list;

#ifdef Q_OS_WIN
    const HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool coInit = (coHr == S_OK || coHr == S_FALSE);

    auto containsLabel = [&discovered](const QString& label) {
        for (const auto& existing : discovered) {
            if (existing.label.compare(label, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

    // 1. Media Foundation capture sources (physical UVC / hardware webcams)
    if (SUCCEEDED(MFStartup(MF_VERSION))) {
        IMFAttributes* attrs = nullptr;
        IMFActivate** devices = nullptr;
        UINT32 count = 0;
        if (SUCCEEDED(MFCreateAttributes(&attrs, 1)) && attrs &&
            SUCCEEDED(attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                     MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) &&
            SUCCEEDED(MFEnumDeviceSources(attrs, &devices, &count)) &&
            devices) {
            for (UINT32 i = 0; i < count; ++i) {
                QString label = readMfAttrString(devices[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
                if (label.isEmpty()) {
                    label = tr("Camera %1").arg(discovered.size() + 1);
                }
                const QString symLink = readMfAttrString(
                    devices[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
                discovered.push_back(CameraDeviceDescriptor{
                    CameraDeviceDescriptor::Backend::MediaFoundation,
                    label,
                    symLink
                });
                devices[i]->Release();
            }
            CoTaskMemFree(devices);
        }
        if (attrs) {
            attrs->Release();
        }
        MFShutdown();
    }

    // 2. DirectShow capture sources (catches OBS Virtual Camera, DroidCam, etc.)
    ICreateDevEnum* devEnum = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&devEnum))) && devEnum) {
        IEnumMoniker* enumMoniker = nullptr;
        if (devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMoniker, 0) == S_OK && enumMoniker) {
            IMoniker* moniker = nullptr;
            while (enumMoniker->Next(1, &moniker, nullptr) == S_OK && moniker) {
                QString friendlyName;
                IPropertyBag* propBag = nullptr;
                if (SUCCEEDED(moniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&propBag))) && propBag) {
                    VARIANT varName;
                    VariantInit(&varName);
                    if (SUCCEEDED(propBag->Read(L"FriendlyName", &varName, nullptr)) &&
                        varName.vt == VT_BSTR && varName.bstrVal) {
                        friendlyName = QString::fromWCharArray(varName.bstrVal).trimmed();
                    }
                    VariantClear(&varName);
                    propBag->Release();
                }

                QString displayName;
                LPOLESTR rawDisp = nullptr;
                if (SUCCEEDED(moniker->GetDisplayName(nullptr, nullptr, &rawDisp)) && rawDisp) {
                    displayName = QString::fromWCharArray(rawDisp).trimmed();
                    CoTaskMemFree(rawDisp);
                }

                if (!friendlyName.isEmpty() && !containsLabel(friendlyName)) {
                    discovered.push_back(CameraDeviceDescriptor{
                        CameraDeviceDescriptor::Backend::DirectShow,
                        friendlyName,
                        displayName
                    });
                }
                moniker->Release();
            }
            enumMoniker->Release();
        }
        devEnum->Release();
    }

    if (coInit) {
        CoUninitialize();
    }
#endif

    for (size_t i = 0; i < discovered.size(); ++i) {
        QVariantMap entry;
        entry.insert(QStringLiteral("value"), static_cast<int>(i));
        entry.insert(QStringLiteral("label"), discovered[i].label);
        list.push_back(entry);
    }

    const bool changed = (m_availableCameras != list);
    m_devices = std::move(discovered);
    m_availableCameras = list;

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    const QString savedName = settings.value(QStringLiteral("devices/webcamName")).toString();
    int resolvedIndex = 0;
    if (!savedName.isEmpty()) {
        for (int i = 0; i < m_availableCameras.size(); ++i) {
            if (m_availableCameras[i].toMap().value(QStringLiteral("label")).toString() == savedName) {
                resolvedIndex = i;
                break;
            }
        }
    }
    if (resolvedIndex >= m_availableCameras.size()) {
        resolvedIndex = 0;
    }

    const int prevIdx = m_selectedCameraIndex.exchange(resolvedIndex);
    if (changed) {
        emit devicesChanged();
    }
    if (prevIdx != resolvedIndex || changed) {
        emit selectedCameraChanged();
    }
}

void WebcamService::selectCamera(int index) {
    if (m_availableCameras.isEmpty()) {
        return;
    }
    const int clamped = std::clamp(index, 0, static_cast<int>(m_availableCameras.size()) - 1);
    const int prev = m_selectedCameraIndex.exchange(clamped);

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("devices/webcamName"),
                      m_availableCameras[clamped].toMap().value(QStringLiteral("label")).toString());

    if (prev != clamped) {
        emit selectedCameraChanged();
        if (m_active.load()) {
            startCapture();
        }
    }
}

void WebcamService::grantPermission() {
    setPermissionGranted(true);
}

void WebcamService::setPermissionGranted(bool granted) {
    updateOsPermissionState();
    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("permissions/webcamGranted"), granted);
    if (m_permissionGranted != granted) {
        m_permissionGranted = granted;
        emit permissionChanged();
    }
    if (!granted && m_active.load()) {
        stopCapture();
    } else if (granted) {
        refreshDevices();
    }
}

void WebcamService::openPrivacySettings() {
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:privacy-webcam")));
#endif
}

void WebcamService::setActive(bool enabled) {
    if (enabled == m_active.load()) {
        return;
    }
    if (enabled) {
        startCapture();
    } else {
        stopCapture();
    }
}

void WebcamService::toggleCamera() {
    setActive(!isActive());
}

void WebcamService::setApiBaseUrl(const QString& baseUrl) {
    QString clean = baseUrl.trimmed();
    while (clean.endsWith(QLatin1Char('/'))) {
        clean.chop(1);
    }
    if (!clean.isEmpty()) {
        m_apiBaseUrl = clean;
    }
}

void WebcamService::setSessionContext(const QString& interviewId, const QString& language) {
    m_interviewId = interviewId.trimmed();
    m_sessionLanguage = (language == QLatin1String("en")) ? QStringLiteral("en") : QStringLiteral("uk");
    resetSessionVisualMetrics();
}

void WebcamService::setIncludeInScore(bool include) {
    if (m_includeInScore != include) {
        m_includeInScore = include;
        QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
        settings.setValue(QStringLiteral("input/includeCameraInScore"), include);
        emit visualMetricsChanged();
    }
}

QString WebcamService::detectedEmotionLabel() const {
    const bool en = (m_sessionLanguage == QLatin1String("en"));
    const QString e = m_detectedEmotion.toLower().trimmed();
    if (!m_personPresent || e == QLatin1String("absent")) {
        return en ? QStringLiteral("Not in frame") : QStringLiteral("Відсутній у кадрі");
    }
    if (e == QLatin1String("smiling") || e == QLatin1String("smile") || e == QLatin1String("happy")) {
        return en ? QStringLiteral("Smiling") : QStringLiteral("Посмішка");
    }
    if (e == QLatin1String("confident")) {
        return en ? QStringLiteral("Confident") : QStringLiteral("Впевненість");
    }
    if (e == QLatin1String("focused") || e == QLatin1String("attentive")) {
        return en ? QStringLiteral("Focused") : QStringLiteral("Зосередженість");
    }
    if (e == QLatin1String("tense") || e == QLatin1String("nervous") || e == QLatin1String("stressed")) {
        return en ? QStringLiteral("Tense") : QStringLiteral("Напруженість");
    }
    if (e == QLatin1String("distracted") || e == QLatin1String("away")) {
        return en ? QStringLiteral("Distracted") : QStringLiteral("Відволікання");
    }
    return en ? QStringLiteral("Calm") : QStringLiteral("Спокій");
}

void WebcamService::resetSessionVisualMetrics() {
    m_instantOverall = 0;
    m_instantLighting = 0;
    m_instantPresence = 0;
    m_instantMotion = 0;
    m_instantPosture = 0;
    m_instantAppearance = 0;
    m_instantEmotionScore = 0;
    m_instantEmotion = QStringLiteral("absent");
    m_vlmOverall = 0;
    m_vlmPersonPresent = false;
    m_hasVlmResult = false;
    m_visualScore = 0;
    m_presenceScore = 0;
    m_handsScore = 0;
    m_lightingScore = 0;
    m_appearanceScore = 0;
    m_postureScore = 0;
    m_emotionScore = 0;
    m_detectedEmotion = QStringLiteral("absent");
    m_visualSummary.clear();
    m_personPresent = false;
    m_visionFps = 0;
    m_sessionSamples = 0;
    m_sumOverall = 0;
    m_sumPresence = 0;
    m_sumHands = 0;
    m_sumLighting = 0;
    m_sumAppearance = 0;
    m_sumPosture = 0;
    m_sumEmotion = 0;
    emit visualMetricsChanged();
}

std::optional<models::VisualAssessment> WebcamService::buildSessionVisualAssessment() const {
    if (m_sessionSamples <= 0 && m_visualScore <= 0 && !isActive()) {
        return std::nullopt;
    }
    models::VisualAssessment v;
    if (m_sessionSamples > 0) {
        const double c = static_cast<double>(m_sessionSamples);
        v.overallScore = static_cast<int>(std::llround(m_sumOverall / c));
        v.presenceScore = static_cast<int>(std::llround(m_sumPresence / c));
        v.handsScore = static_cast<int>(std::llround(m_sumHands / c));
        v.lightingScore = static_cast<int>(std::llround(m_sumLighting / c));
        v.appearanceScore = static_cast<int>(std::llround(m_sumAppearance / c));
        v.postureScore = static_cast<int>(std::llround(m_sumPosture / c));
        v.emotionScore = static_cast<int>(std::llround(m_sumEmotion / c));
        v.samplesAnalyzed = m_sessionSamples;
        v.model = (m_lastVlmModel + QStringLiteral(" + 30 FPS CV")).toStdString();
    } else {
        v.overallScore = m_visualScore;
        v.presenceScore = m_presenceScore;
        v.handsScore = m_handsScore;
        v.lightingScore = m_lightingScore;
        v.appearanceScore = m_appearanceScore;
        v.postureScore = m_postureScore;
        v.emotionScore = m_emotionScore;
        v.samplesAnalyzed = 1;
        v.model = "30 FPS Local CV";
    }
    v.dominantEmotion = m_detectedEmotion.toStdString();
    v.includedInScore = m_includeInScore;
    v.summary = m_visualSummary.toStdString();
    return v;
}

std::optional<models::VisualContext> WebcamService::buildLiveVisualContext() const {
    if (!isActive()) {
        return std::nullopt;
    }
    models::VisualContext ctx;
    ctx.cameraActive = true;
    ctx.includeInScore = m_includeInScore;
    ctx.personPresent = m_personPresent;
    ctx.visualScore = m_visualScore;
    ctx.presenceScore = m_presenceScore;
    ctx.handsScore = m_handsScore;
    ctx.lightingScore = m_lightingScore;
    ctx.appearanceScore = m_appearanceScore;
    ctx.postureScore = m_postureScore;
    ctx.emotionScore = m_emotionScore;
    ctx.emotion = m_detectedEmotion.toStdString();
    ctx.summary = m_visualSummary.toStdString();
    return ctx;
}

void WebcamService::setStatus(const QString& text) {
    if (m_statusText != text) {
        m_statusText = text;
        emit statusChanged();
    }
}

void WebcamService::analyzeFrameInstant(const QImage& img) {
    const int w = img.width();
    const int h = img.height();
    if (w < 64 || h < 48) {
        return;
    }

    constexpr int cols = 64;
    constexpr int rows = 48;
    std::array<uchar, cols * rows> lumaGrid{};
    std::array<uchar, cols * rows> skinMask{};

    double centerSum = 0.0;
    int centerCount = 0;
    int centerMin = 255;
    int centerMax = 0;

    double borderSum = 0.0;
    int borderCount = 0;
    int darkPixels = 0;
    int clippedPixels = 0;

    int skinCount = 0;
    int skinMinX = cols;
    int skinMaxX = 0;
    int skinMinY = rows;
    int skinMaxY = 0;
    double skinSumX = 0.0;
    double skinSumY = 0.0;
    double skinLumaSum = 0.0;
    double leftFaceLumaSum = 0.0;
    double rightFaceLumaSum = 0.0;
    int leftFaceCount = 0;
    int rightFaceCount = 0;
    int lowerHandSkinCount = 0;

    for (int gy = 0; gy < rows; ++gy) {
        const int py = (gy * (h - 1)) / (rows - 1);
        const auto* line = reinterpret_cast<const QRgb*>(img.constScanLine(py));
        for (int gx = 0; gx < cols; ++gx) {
            const int px = (gx * (w - 1)) / (cols - 1);
            const QRgb rgb = line[px];
            const int r = qRed(rgb);
            const int g = qGreen(rgb);
            const int b = qBlue(rgb);
            const int y = (77 * r + 150 * g + 29 * b) >> 8;
            const int cb = 128 + ((-43 * r - 85 * g + 128 * b) >> 8);
            const int cr = 128 + ((128 * r - 107 * g - 21 * b) >> 8);

            const int idx = gy * cols + gx;
            lumaGrid[static_cast<size_t>(idx)] = static_cast<uchar>(y);

            if (y < 36) ++darkPixels;
            if (y > 240) ++clippedPixels;

            const bool isBorder = (gx < 8 || gx > 55 || gy < 4);
            if (isBorder) {
                borderSum += y;
                ++borderCount;
            }

            const bool inSubjectZone = (gx >= 12 && gx <= 51 && gy >= 4 && gy <= 41);
            const bool isSkin = (y >= 36 && y <= 238 &&
                                 cb >= 76 && cb <= 129 &&
                                 cr >= 131 && cr <= 178 &&
                                 r > g && r > b && (r - b) >= 14 && std::abs(r - g) >= 6);
            skinMask[static_cast<size_t>(idx)] = isSkin ? 1 : 0;

            if (inSubjectZone) {
                centerSum += y;
                ++centerCount;
                centerMin = std::min(centerMin, y);
                centerMax = std::max(centerMax, y);

                if (isSkin && gy <= 33) {
                    ++skinCount;
                    skinMinX = std::min(skinMinX, gx);
                    skinMaxX = std::max(skinMaxX, gx);
                    skinMinY = std::min(skinMinY, gy);
                    skinMaxY = std::max(skinMaxY, gy);
                    skinSumX += gx;
                    skinSumY += gy;
                    skinLumaSum += y;
                    if (gx < 32) {
                        leftFaceLumaSum += y;
                        ++leftFaceCount;
                    } else {
                        rightFaceLumaSum += y;
                        ++rightFaceCount;
                    }
                } else if (isSkin && gy >= 31) {
                    ++lowerHandSkinCount;
                }
            }
        }
    }

    const double meanLuma = centerCount > 0 ? (centerSum / centerCount) : 0.0;
    const double borderLuma = borderCount > 0 ? (borderSum / borderCount) : meanLuma;
    const int contrast = std::max(0, centerMax - centerMin);
    const double totalGrid = static_cast<double>(cols * rows);
    const double darkRatio = darkPixels / totalGrid;
    const double clipRatio = clippedPixels / totalGrid;

    // Determine candidate head region (either from detected skin cluster or central head zone)
    const int headLeft = (skinCount >= 24) ? std::clamp(skinMinX, 10, 44) : 20;
    const int headRight = (skinCount >= 24) ? std::clamp(skinMaxX, headLeft + 6, 53) : 43;
    const int headTop = (skinCount >= 24) ? std::clamp(skinMinY, 3, 30) : 7;
    const int headBottom = (skinCount >= 24) ? std::clamp(skinMaxY, headTop + 6, 38) : 30;
    const int headW = std::max(1, headRight - headLeft + 1);
    const int headH = std::max(1, headBottom - headTop + 1);

    // Interior facial gradient analysis: horizontal features (brows, eyes, nose, mouth) + bilateral symmetry
    double faceHorizEdgeSum = 0.0;
    double faceVertEdgeSum = 0.0;
    int facePixels = 0;
    double eyeBandEdgeSum = 0.0;
    int eyeBandPixels = 0;
    double mouthBandEdgeSum = 0.0;
    int mouthBandPixels = 0;
    double symDiffSum = 0.0;
    int symPairs = 0;

    for (int gy = headTop + 1; gy <= headBottom; ++gy) {
        const double relY = static_cast<double>(gy - headTop) / headH;
        for (int gx = headLeft + 1; gx <= headRight; ++gx) {
            const int idx = gy * cols + gx;
            const int y = lumaGrid[static_cast<size_t>(idx)];
            const int dy = std::abs(y - static_cast<int>(lumaGrid[static_cast<size_t>(idx - cols)]));
            const int dx = std::abs(y - static_cast<int>(lumaGrid[static_cast<size_t>(idx - 1)]));
            faceHorizEdgeSum += dy;
            faceVertEdgeSum += dx;
            ++facePixels;

            if (relY >= 0.20 && relY <= 0.48) {
                eyeBandEdgeSum += (dy * 1.3 + dx * 0.7);
                ++eyeBandPixels;
            } else if (relY >= 0.62 && relY <= 0.88) {
                mouthBandEdgeSum += (dy * 1.4 + dx * 0.6);
                ++mouthBandPixels;
            }

            const int mirrorX = headLeft + headRight - gx;
            if (gx < mirrorX && mirrorX < cols) {
                const int mirrorY = lumaGrid[static_cast<size_t>(gy * cols + mirrorX)];
                symDiffSum += std::abs(y - mirrorY);
                ++symPairs;
            }
        }
    }

    const double faceHorizEdge = facePixels > 0 ? (faceHorizEdgeSum / facePixels) : 0.0;
    const double faceVertEdge = facePixels > 0 ? (faceVertEdgeSum / facePixels) : 0.0;
    const double eyeBandEdge = eyeBandPixels > 0 ? (eyeBandEdgeSum / eyeBandPixels) : 0.0;
    const double mouthBandEdge = mouthBandPixels > 0 ? (mouthBandEdgeSum / mouthBandPixels) : 0.0;
    const double symmetryScore = symPairs > 0
        ? std::clamp(1.0 - (symDiffSum / (symPairs * 95.0)), 0.0, 1.0)
        : 0.0;

    // Check torso/shoulder silhouette in lower center (gy 31..46) vs upper background corners
    double torsoEdgeSum = 0.0;
    int torsoPixels = 0;
    for (int gy = 31; gy < 46; ++gy) {
        for (int gx = 14; gx <= 49; ++gx) {
            const int idx = gy * cols + gx;
            torsoEdgeSum += std::abs(static_cast<int>(lumaGrid[static_cast<size_t>(idx)]) -
                                     static_cast<int>(lumaGrid[static_cast<size_t>(idx - 1)]));
            ++torsoPixels;
        }
    }
    const double torsoEdge = torsoPixels > 0 ? (torsoEdgeSum / torsoPixels) : 0.0;

    // 1. Deep Lighting Score (0..100): exposure, dynamic range, clipping, backlighting, key-light balance
    int lightScore = 0;
    if (meanLuma >= 82.0 && meanLuma <= 185.0) {
        lightScore = 90;
    } else if (meanLuma < 82.0) {
        lightScore = static_cast<int>(std::clamp(meanLuma * 1.08, 4.0, 88.0));
    } else {
        lightScore = static_cast<int>(std::clamp(92.0 - (meanLuma - 185.0) * 1.05, 18.0, 88.0));
    }
    if (contrast >= 55 && contrast <= 185) {
        lightScore = std::min(98, lightScore + 8);
    } else if (contrast < 24) {
        lightScore = std::max(5, lightScore - 30);
    }
    if (darkRatio > 0.45) {
        lightScore = std::max(5, lightScore - static_cast<int>((darkRatio - 0.45) * 55.0));
    }
    if (clipRatio > 0.25) {
        lightScore = std::max(12, lightScore - static_cast<int>((clipRatio - 0.25) * 60.0));
    }
    const double faceLuma = skinCount > 0 ? (skinLumaSum / skinCount) : meanLuma;
    if (borderLuma > faceLuma + 42.0) {
        // Strong backlighting silhouette
        lightScore = std::max(15, lightScore - static_cast<int>(std::min(26.0, (borderLuma - faceLuma - 42.0) * 0.45)));
    }
    if (leftFaceCount > 8 && rightFaceCount > 8) {
        const double asym = std::abs((leftFaceLumaSum / leftFaceCount) - (rightFaceLumaSum / rightFaceCount));
        if (asym > 45.0) {
            lightScore = std::max(20, lightScore - 10);
        }
    }

    // 2. Strict yet lighting-adaptive Person Presence & Face Structure Verification (0..100)
    // Supports dim/cool monitor light and glasses while still rejecting empty rooms or covered lenses
    const bool validHeadShape = (headW >= 6 && headW <= 48 && headH >= 6 && headH <= 42);
    const bool hasFacialFeatures = (faceHorizEdge >= 3.0 && eyeBandEdge >= 3.5 && (faceHorizEdge + faceVertEdge) >= 5.8);
    const bool hasHumanSkinFace = (skinCount >= 26 && validHeadShape && hasFacialFeatures);
    const bool hasCoolLitFace = (validHeadShape && faceHorizEdge >= 3.4 &&
                                 eyeBandEdge >= 4.0 && symmetryScore >= 0.46 &&
                                 (torsoEdge >= 2.2 || contrast >= 28));
    const bool personDetected = (meanLuma >= 12.0 && contrast >= 16 && (hasHumanSkinFace || hasCoolLitFace));

    int presenceInst = 10;
    if (!personDetected) {
        if (meanLuma < 11.0 || contrast < 12) {
            presenceInst = 5;
        } else {
            presenceInst = std::clamp(static_cast<int>(6.0 + std::min(skinCount, 20) * 0.25 + std::min(faceHorizEdge, 4.0) * 0.8), 5, 18);
        }
    } else {
        const double skinCoverageBonus = std::clamp(skinCount * 0.22, 6.0, 26.0);
        const double featureSharpnessBonus = std::clamp((eyeBandEdge + mouthBandEdge) * 0.85, 6.0, 22.0);
        const double symBonus = symmetryScore * 12.0;
        presenceInst = static_cast<int>(std::clamp(44.0 + skinCoverageBonus + featureSharpnessBonus + symBonus, 54.0, 97.0));
    }

    // 3. Temporal Motion / Liveness & Hands Score (0..100)
    double mad = 0.0;
    double lowerMad = 0.0;
    if (m_hasPrevGrid) {
        int lowerCells = 0;
        for (int gy = 0; gy < rows; ++gy) {
            for (int gx = 0; gx < cols; ++gx) {
                const size_t i = static_cast<size_t>(gy * cols + gx);
                const int diff = std::abs(static_cast<int>(lumaGrid[i]) - static_cast<int>(m_prevLumaGrid[i]));
                mad += diff;
                if (gy >= 30 && gx >= 10 && gx <= 53) {
                    lowerMad += diff;
                    ++lowerCells;
                }
            }
        }
        mad /= totalGrid;
        if (lowerCells > 0) {
            lowerMad /= lowerCells;
        }
    }
    m_prevLumaGrid = lumaGrid;
    m_hasPrevGrid = true;

    int handsInst = 15;
    if (personDetected) {
        handsInst = 78;
        if (lowerHandSkinCount >= 10 && lowerHandSkinCount <= 140) {
            handsInst += 8; // visible natural hand gestures in lower frame
        }
        if (lowerMad >= 0.6 && lowerMad <= 14.0) {
            handsInst += 6;
        } else if (mad > 18.0) {
            handsInst -= static_cast<int>(std::min(25.0, (mad - 18.0) * 1.5));
        } else if (mad < 0.15) {
            handsInst -= 10; // frozen frame
        }
        handsInst = std::clamp(handsInst, 40, 96);
    }

    // 4. Posture & Framing Score (0..100)
    int postureInst = 12;
    double normCx = 0.5;
    double normCy = 0.36;
    if (personDetected) {
        normCx = (skinCount > 0 ? (skinSumX / skinCount) : 31.5) / (cols - 1.0);
        normCy = (skinCount > 0 ? (skinSumY / skinCount) : 18.0) / (rows - 1.0);
        const double dxCenter = std::abs(normCx - 0.50);
        const double dyIdeal = std::abs(normCy - 0.36);
        const double framingPenalty = std::max(0.0, (dxCenter - 0.08) * 145.0) +
                                      std::max(0.0, (dyIdeal - 0.10) * 125.0);
        postureInst = static_cast<int>(std::clamp(91.0 - framingPenalty + symmetryScore * 6.0, 38.0, 96.0));
    }

    // 5. Appearance Score (0..100)
    int appearanceInst = 12;
    if (personDetected) {
        appearanceInst = static_cast<int>(std::clamp(
            68.0 + std::min(torsoEdge, 14.0) * 1.2 + (lightScore >= 65 ? 8.0 : 0.0),
            45.0, 94.0));
    }

    // 6. Emotion & Facial Expression Analysis (0..100 + label)
    int emotionInstScore = 0;
    QString emotionInstLabel = QStringLiteral("absent");
    if (personDetected) {
        const double mouthToEyeRatio = (eyeBandEdge > 1.0) ? (mouthBandEdge / eyeBandEdge) : 1.0;
        const bool wellFramed = (std::abs(normCx - 0.50) <= 0.18 && normCy <= 0.56);
        if (!wellFramed) {
            emotionInstLabel = QStringLiteral("distracted");
            emotionInstScore = 48;
        } else if (mad > 15.5) {
            emotionInstLabel = QStringLiteral("tense");
            emotionInstScore = 56;
        } else if (mouthBandEdge >= 9.2 && mouthToEyeRatio >= 1.12 && symmetryScore >= 0.55) {
            emotionInstLabel = QStringLiteral("smiling");
            emotionInstScore = std::clamp(static_cast<int>(86.0 + std::min(mouthBandEdge - 9.0, 8.0)), 86, 95);
        } else if (postureInst >= 82 && eyeBandEdge >= 7.5 && mad <= 6.5) {
            emotionInstLabel = QStringLiteral("confident");
            emotionInstScore = std::clamp(static_cast<int>(82.0 + symmetryScore * 8.0), 82, 92);
        } else if (eyeBandEdge >= 6.0 && mad <= 4.5) {
            emotionInstLabel = QStringLiteral("focused");
            emotionInstScore = 81;
        } else {
            emotionInstLabel = QStringLiteral("calm");
            emotionInstScore = 76;
        }
    }

    // Strict overall visual score: if person is NOT detected, cap at <= 22 (Red)!
    int instantOverall = 0;
    if (!personDetected) {
        instantOverall = std::min(22, static_cast<int>(std::llround(presenceInst * 0.65 + lightScore * 0.35)));
    } else {
        instantOverall = std::clamp(static_cast<int>(std::llround(
            presenceInst * 0.26 +
            lightScore * 0.20 +
            postureInst * 0.18 +
            emotionInstScore * 0.14 +
            appearanceInst * 0.12 +
            handsInst * 0.10)), 0, 100);
    }

    // Measure live capture FPS
    if (!m_fpsTimer.isValid()) {
        m_fpsTimer.start();
    }
    ++m_fpsFrameAccum;
    int reportedFps = -1;
    if (m_fpsTimer.elapsed() >= 1000) {
        reportedFps = static_cast<int>(std::llround((m_fpsFrameAccum * 1000.0) / m_fpsTimer.elapsed()));
        m_fpsFrameAccum = 0;
        m_fpsTimer.restart();
    }

    // Trigger deep complex evaluation by lfm3-omni:2b every 10 frames (single-flight async)
    ++m_workerFrameCounter;
    QByteArray jpegForVlm;
    if ((m_workerFrameCounter == 1 || (m_workerFrameCounter % 10 == 0)) &&
        !m_apiBaseUrl.isEmpty() && !m_vlmInFlight.exchange(true)) {
        const QImage scaled = (w > 512 || h > 288)
            ? img.scaled(512, 288, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : img;
        QBuffer buf(&jpegForVlm);
        buf.open(QIODevice::WriteOnly);
        if (!scaled.save(&buf, "JPEG", 72)) {
            jpegForVlm.clear();
            m_vlmInFlight.store(false);
        }
    }

    QMetaObject::invokeMethod(this, [this, instantOverall, lightScore, presenceInst, handsInst,
                                     postureInst, appearanceInst, emotionInstScore, emotionInstLabel,
                                     reportedFps, jpegForVlm]() {
        m_instantOverall = instantOverall;
        m_instantLighting = lightScore;
        m_instantPresence = presenceInst;
        m_instantMotion = handsInst;
        m_instantPosture = postureInst;
        m_instantAppearance = appearanceInst;
        m_instantEmotionScore = emotionInstScore;
        m_instantEmotion = emotionInstLabel;
        if (reportedFps >= 0) {
            m_visionFps = reportedFps;
        }
        recomputeBlendedVisualScore();
        if (!jpegForVlm.isEmpty()) {
            sendFrameToVisionModel(jpegForVlm, instantOverall, lightScore, presenceInst,
                                   handsInst, postureInst, emotionInstScore, emotionInstLabel);
        }
    }, Qt::QueuedConnection);
}

void WebcamService::recomputeBlendedVisualScore() {
    // Immediate absence drop only when pitch-black/covered OR both CV and VLM see nobody
    if (m_instantPresence < 20 && (!m_hasVlmResult || !m_vlmPersonPresent || m_instantLighting <= 10)) {
        m_personPresent = false;
        m_presenceScore = m_instantPresence;
        m_lightingScore = m_instantLighting;
        m_handsScore = m_instantMotion;
        m_postureScore = m_instantPosture;
        m_appearanceScore = m_instantAppearance;
        m_emotionScore = 0;
        m_detectedEmotion = QStringLiteral("absent");
        m_visualScore = std::min(22, m_instantOverall);
        if (m_visualSummary.isEmpty()) {
            m_visualSummary = (m_sessionLanguage == QLatin1String("en"))
                ? QStringLiteral("Candidate is not visible in the camera frame.")
                : QStringLiteral("Кандидат відсутній у кадрі камери.");
        }
        emit visualMetricsChanged();
        return;
    }

    if (!m_hasVlmResult) {
        m_personPresent = (m_instantPresence >= 40);
        m_presenceScore = m_instantPresence;
        m_lightingScore = m_instantLighting;
        m_handsScore = m_instantMotion;
        m_appearanceScore = m_instantAppearance;
        m_postureScore = m_instantPosture;
        m_emotionScore = m_instantEmotionScore;
        m_detectedEmotion = m_instantEmotion;
        m_visualScore = m_instantOverall;
    } else {
        m_visualScore = std::clamp(
            static_cast<int>(std::llround(m_vlmOverall * 0.62 + m_instantOverall * 0.38)),
            0, 100);
        m_lightingScore = std::clamp(
            static_cast<int>(std::llround(m_lightingScore * 0.65 + m_instantLighting * 0.35)),
            0, 100);
        m_postureScore = std::clamp(
            static_cast<int>(std::llround(m_postureScore * 0.68 + m_instantPosture * 0.32)),
            0, 100);
        m_handsScore = std::clamp(
            static_cast<int>(std::llround(m_handsScore * 0.68 + m_instantMotion * 0.32)),
            0, 100);
        m_emotionScore = std::clamp(
            static_cast<int>(std::llround(m_emotionScore * 0.70 + m_instantEmotionScore * 0.30)),
            0, 100);
        if (!m_personPresent) {
            m_visualScore = std::min(22, m_visualScore);
        }
    }
    emit visualMetricsChanged();
}

void WebcamService::sendFrameToVisionModel(const QByteArray& jpegBytes, int instantOverall,
                                           int instantLight, int instantPresence,
                                           int instantHands, int instantPosture,
                                           int instantEmotionScore, const QString& instantEmotion) {
    QJsonObject payload;
    payload.insert(QStringLiteral("image_base64"), QString::fromLatin1(jpegBytes.toBase64()));
    if (!m_interviewId.isEmpty()) {
        payload.insert(QStringLiteral("interview_id"), m_interviewId);
    }
    payload.insert(QStringLiteral("language"), m_sessionLanguage);
    payload.insert(QStringLiteral("instant_score"), instantOverall);
    payload.insert(QStringLiteral("instant_lighting"), instantLight);
    payload.insert(QStringLiteral("instant_presence"), instantPresence);
    payload.insert(QStringLiteral("instant_hands"), instantHands);
    payload.insert(QStringLiteral("instant_posture"), instantPosture);
    payload.insert(QStringLiteral("instant_emotion_score"), instantEmotionScore);
    payload.insert(QStringLiteral("instant_emotion"), instantEmotion);
    payload.insert(QStringLiteral("include_in_score"), m_includeInScore);

    QNetworkRequest req(QUrl(m_apiBaseUrl + QStringLiteral("/api/vision/analyze")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setTransferTimeout(m_hasVlmResult ? 25000 : 65000);

    emit visualMetricsChanged();

    QNetworkReply* reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_vlmInFlight.store(false);

        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || code < 200 || code >= 300) {
            emit visualMetricsChanged();
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(body).object();
        if (obj.isEmpty()) {
            emit visualMetricsChanged();
            return;
        }

        m_vlmOverall = std::clamp(obj.value(QStringLiteral("overall_score")).toInt(m_instantOverall), 0, 100);
        m_presenceScore = std::clamp(obj.value(QStringLiteral("presence_score")).toInt(m_instantPresence), 0, 100);
        m_handsScore = std::clamp(obj.value(QStringLiteral("hands_score")).toInt(m_instantMotion), 0, 100);
        m_lightingScore = std::clamp(obj.value(QStringLiteral("lighting_score")).toInt(m_instantLighting), 0, 100);
        m_appearanceScore = std::clamp(obj.value(QStringLiteral("appearance_score")).toInt(m_instantAppearance), 0, 100);
        m_postureScore = std::clamp(obj.value(QStringLiteral("posture_score")).toInt(m_instantPosture), 0, 100);
        m_emotionScore = std::clamp(obj.value(QStringLiteral("emotion_score")).toInt(m_instantEmotionScore), 0, 100);
        const QString emo = obj.value(QStringLiteral("emotion")).toString().trimmed();
        if (!emo.isEmpty()) {
            m_detectedEmotion = emo;
        }
        m_personPresent = obj.value(QStringLiteral("person_present")).toBool(m_presenceScore >= 40);
        m_vlmPersonPresent = m_personPresent;
        if (!m_personPresent) {
            m_vlmOverall = std::min(22, m_vlmOverall);
            m_presenceScore = std::min(18, m_presenceScore);
            m_emotionScore = 0;
            m_detectedEmotion = QStringLiteral("absent");
        }
        const QString sum = obj.value(QStringLiteral("summary")).toString().trimmed();
        if (!sum.isEmpty()) {
            m_visualSummary = sum;
        }
        const QString mdl = obj.value(QStringLiteral("model")).toString().trimmed();
        if (!mdl.isEmpty()) {
            m_lastVlmModel = mdl;
        }
        m_hasVlmResult = true;

        ++m_sessionSamples;
        m_sumOverall += m_vlmOverall;
        m_sumPresence += m_presenceScore;
        m_sumHands += m_handsScore;
        m_sumLighting += m_lightingScore;
        m_sumAppearance += m_appearanceScore;
        m_sumPosture += m_postureScore;
        m_sumEmotion += m_emotionScore;

        recomputeBlendedVisualScore();
    });
}

void WebcamService::pushFrame(const QImage& img) {
    if (img.isNull() || m_stopRequested.load()) {
        return;
    }
    if (m_buffer) {
        QMutexLocker locker(&m_buffer->mutex);
        m_buffer->frame = img;
    }
    analyzeFrameInstant(img);
    const bool firstFrame = !m_hasFrame.exchange(true);
    QMetaObject::invokeMethod(this, [this, firstFrame]() {
        if (firstFrame) {
            setStatus(tr("Camera live"));
        }
        ++m_frameRevision;
        emit frameReady();
    }, Qt::QueuedConnection);
}

void WebcamService::startCapture() {
    updateOsPermissionState();
    if (!m_osPermissionAllowed) {
        setStatus(tr("Camera blocked in Windows Privacy Settings"));
        emit permissionRequired();
        return;
    }
    if (!m_permissionGranted) {
        setStatus(tr("Camera permission required"));
        emit permissionRequired();
        return;
    }

    refreshDevices();
    if (m_devices.empty()) {
        setStatus(tr("No webcam detected"));
        return;
    }

    const int idx = std::clamp(m_selectedCameraIndex.load(), 0, static_cast<int>(m_devices.size()) - 1);
    const CameraDeviceDescriptor target = m_devices[static_cast<size_t>(idx)];

    stopCapture();
    m_stopRequested.store(false);
    m_hasFrame.store(false);
    m_hasPrevGrid = false;
    m_workerFrameCounter = 0;
    m_fpsFrameAccum = 0;
    m_fpsTimer.invalidate();
    setStatus(tr("Starting %1...").arg(target.label));
    m_active.store(true);
    emit activeChanged();
    m_worker = std::thread(&WebcamService::captureThreadMain, this, target);
}

void WebcamService::stopCapture() {
    m_stopRequested.store(true);
    if (m_worker.joinable()) {
        m_worker.join();
    }
    if (m_active.exchange(false)) {
        emit activeChanged();
    }
    m_hasFrame.store(false);
    m_hasPrevGrid = false;
    if (m_visionFps != 0) {
        m_visionFps = 0;
        emit visualMetricsChanged();
    }
    if (m_buffer) {
        QMutexLocker locker(&m_buffer->mutex);
        m_buffer->frame = QImage();
    }
    setStatus(QString());
}

void WebcamService::captureThreadMain(CameraDeviceDescriptor device) {
#ifdef Q_OS_WIN
    if (device.backend == CameraDeviceDescriptor::Backend::DirectShow) {
        captureDirectShow(device);
    } else {
        captureMediaFoundation(device);
    }
#else
    Q_UNUSED(device)
    QMetaObject::invokeMethod(this, [this]() {
        setStatus(tr("Camera not supported on this platform"));
        if (m_active.exchange(false)) emit activeChanged();
    }, Qt::QueuedConnection);
#endif
}

void WebcamService::captureMediaFoundation(const CameraDeviceDescriptor& device) {
#ifdef Q_OS_WIN
    const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool coInit = (coHr == S_OK || coHr == S_FALSE);
    auto postError = [this](const QString& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            setStatus(msg);
            if (m_active.exchange(false)) {
                emit activeChanged();
            }
        }, Qt::QueuedConnection);
    };

    if (FAILED(MFStartup(MF_VERSION))) {
        postError(tr("Media Foundation unavailable"));
        if (coInit) CoUninitialize();
        return;
    }

    IMFAttributes* attrs = nullptr;
    IMFActivate** devices = nullptr;
    UINT32 count = 0;
    IMFMediaSource* source = nullptr;
    IMFSourceReader* reader = nullptr;

    if (FAILED(MFCreateAttributes(&attrs, 1)) || !attrs ||
        FAILED(attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                              MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) ||
        FAILED(MFEnumDeviceSources(attrs, &devices, &count)) ||
        count == 0 || !devices) {
        if (attrs) attrs->Release();
        if (devices) CoTaskMemFree(devices);
        MFShutdown();
        postError(tr("No webcam detected"));
        if (coInit) CoUninitialize();
        return;
    }

    UINT32 chosenIdx = 0;
    for (UINT32 i = 0; i < count; ++i) {
        const QString sym = readMfAttrString(devices[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
        const QString name = readMfAttrString(devices[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
        if ((!device.id.isEmpty() && sym == device.id) ||
            (!device.label.isEmpty() && name == device.label)) {
            chosenIdx = i;
            break;
        }
    }

    HRESULT hr = devices[chosenIdx]->ActivateObject(IID_PPV_ARGS(&source));
    for (UINT32 i = 0; i < count; ++i) {
        devices[i]->Release();
    }
    CoTaskMemFree(devices);
    attrs->Release();

    IMFAttributes* readerAttrs = nullptr;
    if (SUCCEEDED(hr) && source) {
        // Enable Windows Media Foundation Video Processor MFT so NV12/YUY2/MJPG
        // streams convert cleanly to RGB32 in hardware/software.
        if (SUCCEEDED(MFCreateAttributes(&readerAttrs, 2)) && readerAttrs) {
            readerAttrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
            readerAttrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        }
        hr = MFCreateSourceReaderFromMediaSource(source, readerAttrs, &reader);
    }
    if (readerAttrs) {
        readerAttrs->Release();
    }

    if (FAILED(hr) || !source || !reader) {
        if (source) {
            source->Shutdown();
            source->Release();
        }
        MFShutdown();
        postError(tr("Cannot access webcam"));
        if (coInit) CoUninitialize();
        return;
    }

    const GUID preferredSubtypes[] = {
        MFVideoFormat_RGB32,
        MFVideoFormat_RGB24,
        MFVideoFormat_NV12,
        MFVideoFormat_YUY2
    };
    for (const GUID& sub : preferredSubtypes) {
        IMFMediaType* reqType = nullptr;
        if (SUCCEEDED(MFCreateMediaType(&reqType)) && reqType) {
            reqType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            reqType->SetGUID(MF_MT_SUBTYPE, sub);
            const HRESULT typeHr = reader->SetCurrentMediaType(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, reqType);
            reqType->Release();
            if (SUCCEEDED(typeHr)) {
                break;
            }
        }
    }

    UINT32 frameWidth = 640;
    UINT32 frameHeight = 480;
    LONG defaultStride = 0;
    GUID activeSubtype = MFVideoFormat_RGB32;

    auto refreshActiveMediaType = [&]() {
        IMFMediaType* curType = nullptr;
        if (SUCCEEDED(reader->GetCurrentMediaType(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &curType)) && curType) {
            MFGetAttributeSize(curType, MF_MT_FRAME_SIZE, &frameWidth, &frameHeight);
            curType->GetGUID(MF_MT_SUBTYPE, &activeSubtype);
            UINT32 rawStride = 0;
            if (SUCCEEDED(curType->GetUINT32(MF_MT_DEFAULT_STRIDE, &rawStride))) {
                defaultStride = static_cast<LONG>(static_cast<INT32>(rawStride));
            } else {
                defaultStride = 0;
            }
            curType->Release();
        }
    };
    refreshActiveMediaType();

    bool streamFailed = false;
    while (!m_stopRequested.load()) {
        DWORD streamIdx = 0;
        DWORD flags = 0;
        LONGLONG ts = 0;
        IMFSample* sample = nullptr;
        hr = reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                0, &streamIdx, &flags, &ts, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            if (sample) sample->Release();
            streamFailed = FAILED(hr);
            break;
        }
        if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) {
            refreshActiveMediaType();
        }
        if (sample) {
            IMFMediaBuffer* buf = nullptr;
            if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buf)) && buf) {
                BYTE* data = nullptr;
                DWORD maxLen = 0;
                DWORD curLen = 0;
                if (SUCCEEDED(buf->Lock(&data, &maxLen, &curLen)) && data) {
                    const QImage img = decodeVideoFrameToQImage(
                        data, curLen, frameWidth, frameHeight,
                        defaultStride, activeSubtype, false);
                    buf->Unlock();
                    pushFrame(img);
                }
                buf->Release();
            }
            sample->Release();
        }
    }

    reader->Release();
    source->Shutdown();
    source->Release();
    MFShutdown();
    if (streamFailed && !m_stopRequested.load()) {
        postError(tr("Camera busy (close OBS or other app using it)"));
    }
    if (coInit) {
        CoUninitialize();
    }
#else
    Q_UNUSED(device)
#endif
}

void WebcamService::captureDirectShow(const CameraDeviceDescriptor& device) {
#ifdef Q_OS_WIN
    const HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool coInit = (coHr == S_OK || coHr == S_FALSE);
    auto postError = [this](const QString& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            setStatus(msg);
            if (m_active.exchange(false)) {
                emit activeChanged();
            }
        }, Qt::QueuedConnection);
    };

    IBaseFilter* srcFilter = nullptr;
    ICreateDevEnum* devEnum = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&devEnum))) && devEnum) {
        IEnumMoniker* enumMoniker = nullptr;
        if (devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMoniker, 0) == S_OK && enumMoniker) {
            IMoniker* moniker = nullptr;
            while (enumMoniker->Next(1, &moniker, nullptr) == S_OK && moniker) {
                bool match = false;
                LPOLESTR rawDisp = nullptr;
                if (SUCCEEDED(moniker->GetDisplayName(nullptr, nullptr, &rawDisp)) && rawDisp) {
                    const QString disp = QString::fromWCharArray(rawDisp).trimmed();
                    CoTaskMemFree(rawDisp);
                    if (!device.id.isEmpty() && disp == device.id) {
                        match = true;
                    }
                }
                if (!match) {
                    IPropertyBag* propBag = nullptr;
                    if (SUCCEEDED(moniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&propBag))) && propBag) {
                        VARIANT varName;
                        VariantInit(&varName);
                        if (SUCCEEDED(propBag->Read(L"FriendlyName", &varName, nullptr)) &&
                            varName.vt == VT_BSTR && varName.bstrVal) {
                            match = (QString::fromWCharArray(varName.bstrVal).trimmed() == device.label);
                        }
                        VariantClear(&varName);
                        propBag->Release();
                    }
                }
                if (match) {
                    moniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&srcFilter));
                    moniker->Release();
                    break;
                }
                moniker->Release();
            }
            enumMoniker->Release();
        }
        devEnum->Release();
    }

    if (!srcFilter) {
        postError(tr("Cannot access %1").arg(device.label));
        if (coInit) CoUninitialize();
        return;
    }

    IGraphBuilder* graph = nullptr;
    ICaptureGraphBuilder2* capture = nullptr;
    IBaseFilter* grabberFilter = nullptr;
    ISampleGrabber* grabber = nullptr;
    IBaseFilter* nullFilter = nullptr;
    IMediaControl* mediaControl = nullptr;

    auto cleanup = [&]() {
        if (grabber) {
            grabber->SetCallback(nullptr, 1);
            grabber->Release();
        }
        if (mediaControl) {
            mediaControl->Stop();
            mediaControl->Release();
        }
        if (nullFilter) nullFilter->Release();
        if (grabberFilter) grabberFilter->Release();
        if (capture) capture->Release();
        if (graph) graph->Release();
        if (srcFilter) srcFilter->Release();
    };

    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&graph));
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&capture));
    }
    if (SUCCEEDED(hr)) {
        hr = capture->SetFiltergraph(graph);
    }
    if (SUCCEEDED(hr)) {
        hr = graph->AddFilter(srcFilter, L"Webcam Source");
    }
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(kClsidSampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&grabberFilter));
    }
    if (SUCCEEDED(hr)) {
        hr = grabberFilter->QueryInterface(IID_PPV_ARGS(&grabber));
    }
    if (SUCCEEDED(hr)) {
        hr = graph->AddFilter(grabberFilter, L"Sample Grabber");
    }
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(kClsidNullRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&nullFilter));
    }
    if (SUCCEEDED(hr)) {
        hr = graph->AddFilter(nullFilter, L"Null Renderer");
    }
    if (SUCCEEDED(hr)) {
        hr = graph->QueryInterface(IID_PPV_ARGS(&mediaControl));
    }

    if (FAILED(hr) || !grabber || !mediaControl) {
        cleanup();
        postError(tr("Cannot initialize virtual camera pipeline"));
        if (coInit) CoUninitialize();
        return;
    }

    IMediaFilter* mediaFilter = nullptr;
    if (SUCCEEDED(graph->QueryInterface(IID_PPV_ARGS(&mediaFilter))) && mediaFilter) {
        mediaFilter->SetSyncSource(nullptr);
        mediaFilter->Release();
    }

    // Try native pin format (GUID_NULL, e.g. NV12/YUY2/I420) FIRST so DirectShow
    // doesn't stall inserting a legacy Color Space Converter; fall back to RGB32/24.
    const GUID dsSubtypes[] = {
        GUID_NULL,
        MFVideoFormat_NV12,
        MEDIASUBTYPE_RGB32,
        MEDIASUBTYPE_RGB24
    };
    bool rendered = false;
    for (const GUID& sub : dsSubtypes) {
        AM_MEDIA_TYPE req{};
        req.majortype = MEDIATYPE_Video;
        req.subtype = sub;
        grabber->SetMediaType(&req);
        if (SUCCEEDED(capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                            srcFilter, grabberFilter, nullFilter))) {
            rendered = true;
            break;
        }
    }

    if (!rendered) {
        cleanup();
        postError(tr("Cannot connect stream for %1").arg(device.label));
        if (coInit) CoUninitialize();
        return;
    }

    AM_MEDIA_TYPE connectedMt{};
    UINT32 frameWidth = 640;
    UINT32 frameHeight = 480;
    LONG stride = 0;
    bool bottomUpRgb = true;
    GUID activeSubtype = MEDIASUBTYPE_RGB32;

    if (SUCCEEDED(grabber->GetConnectedMediaType(&connectedMt))) {
        extractMediaTypeInfo(connectedMt, frameWidth, frameHeight, stride, bottomUpRgb, activeSubtype);
        freeMediaType(connectedMt);
    }

    auto* cb = new SampleGrabberCallback([this, frameWidth, frameHeight, stride, bottomUpRgb, activeSubtype](
                                             const BYTE* buf, LONG len) {
        if (m_stopRequested.load() || len <= 0) {
            return;
        }
        const QImage img = decodeVideoFrameToQImage(
            buf, static_cast<DWORD>(len), frameWidth, frameHeight,
            stride, activeSubtype, bottomUpRgb);
        pushFrame(img);
    });

    grabber->SetOneShot(FALSE);
    grabber->SetBufferSamples(TRUE);
    grabber->SetCallback(cb, 1);
    cb->Release(); // grabber holds the ref

    hr = mediaControl->Run();
    if (FAILED(hr)) {
        cleanup();
        postError(tr("Failed to start %1").arg(device.label));
        if (coInit) CoUninitialize();
        return;
    }

    while (!m_stopRequested.load()) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(15);
    }

    cleanup();
    if (coInit) {
        CoUninitialize();
    }
#else
    Q_UNUSED(device)
#endif
}

} // namespace devdialogue::services
