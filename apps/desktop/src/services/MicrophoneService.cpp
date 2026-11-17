#include "services/MicrophoneService.hpp"

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
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace devdialogue::services {

namespace {

bool readWindowsMicConsent() {
#ifdef Q_OS_WIN
    const QSettings hkcu(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone"),
        QSettings::NativeFormat);
    if (hkcu.value(QStringLiteral("Value"), QStringLiteral("Allow")).toString()
            .compare(QStringLiteral("Deny"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    const QSettings hklm(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone"),
        QSettings::NativeFormat);
    return hklm.value(QStringLiteral("Value"), QStringLiteral("Allow")).toString()
               .compare(QStringLiteral("Deny"), Qt::CaseInsensitive) != 0;
#else
    return true;
#endif
}

} // namespace

#ifdef Q_OS_WIN
namespace {

constexpr int SampleRate = 16000;
constexpr int Channels = 1;
constexpr int BitsPerSample = 16;
constexpr int BufferCount = 4;
constexpr int BufferBytes = SampleRate * (BitsPerSample / 8) / 20; // 50ms chunks (1600 bytes)

} // namespace

struct MicrophoneService::Impl {
    HWAVEIN hWaveIn{nullptr};
    std::array<WAVEHDR, BufferCount> headers{};
    std::array<std::vector<char>, BufferCount> storage;

    static void CALLBACK waveInCallback(HWAVEIN, UINT uMsg, DWORD_PTR dwInstance,
                                        DWORD_PTR dwParam1, DWORD_PTR) {
        if (uMsg != WIM_DATA || !dwInstance || !dwParam1) {
            return;
        }
        auto* self = reinterpret_cast<MicrophoneService*>(dwInstance);
        auto* hdr = reinterpret_cast<WAVEHDR*>(dwParam1);
        const int idx = static_cast<int>(hdr->dwUser);
        if (hdr->dwBytesRecorded > 0 && hdr->lpData) {
            self->handleCapturedBlock(hdr->lpData, static_cast<int>(hdr->dwBytesRecorded), idx);
        } else {
            QMetaObject::invokeMethod(self, [self, idx]() {
                if ((self->m_recording.load() || self->m_testingMic.load()) &&
                    self->m_impl && self->m_impl->hWaveIn) {
                    waveInAddBuffer(self->m_impl->hWaveIn, &self->m_impl->headers[idx], sizeof(WAVEHDR));
                }
            }, Qt::QueuedConnection);
        }
    }
};
#else
struct MicrophoneService::Impl {};
#endif

MicrophoneService::MicrophoneService(QObject* parent)
    : QObject(parent), m_impl(new Impl()) {
    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    m_permissionGranted = settings.value(QStringLiteral("permissions/microphoneGranted"), false).toBool();
    m_muted = settings.value(QStringLiteral("audio/micMuted"), false).toBool();
    refreshDevices();
}

MicrophoneService::~MicrophoneService() {
    stopHardwareCapture();
    delete m_impl;
    m_impl = nullptr;
}

QString MicrophoneService::selectedMicrophoneName() const {
    const int idx = m_selectedMicrophoneIndex.load();
    if (idx >= 0 && idx < m_availableMicrophones.size()) {
        return m_availableMicrophones[idx].toMap().value(QStringLiteral("label")).toString();
    }
    return QString();
}

void MicrophoneService::updateOsPermissionState() {
    const bool osAllowed = readWindowsMicConsent();
    if (m_osPermissionAllowed != osAllowed) {
        m_osPermissionAllowed = osAllowed;
        emit permissionChanged();
    }
}

void MicrophoneService::refreshDevices() {
    updateOsPermissionState();
    QVariantList list;

#ifdef Q_OS_WIN
    const UINT numDevs = waveInGetNumDevs();
    for (UINT i = 0; i < numDevs; ++i) {
        WAVEINCAPSW caps{};
        QString label = tr("Microphone %1").arg(i + 1);
        if (waveInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            const QString decoded = QString::fromWCharArray(caps.szPname).trimmed();
            if (!decoded.isEmpty()) {
                label = decoded;
            }
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("value"), static_cast<int>(i));
        entry.insert(QStringLiteral("label"), label);
        list.push_back(entry);
    }
#endif

    const bool changed = (m_availableMicrophones != list);
    m_availableMicrophones = list;

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    const QString savedName = settings.value(QStringLiteral("devices/microphoneName")).toString();
    int resolvedIndex = 0;
    if (!savedName.isEmpty()) {
        for (int i = 0; i < m_availableMicrophones.size(); ++i) {
            if (m_availableMicrophones[i].toMap().value(QStringLiteral("label")).toString() == savedName) {
                resolvedIndex = i;
                break;
            }
        }
    }
    if (resolvedIndex >= m_availableMicrophones.size()) {
        resolvedIndex = 0;
    }

    const int prevIdx = m_selectedMicrophoneIndex.exchange(resolvedIndex);
    if (changed) {
        emit devicesChanged();
    }
    if (prevIdx != resolvedIndex || changed) {
        emit selectedMicrophoneChanged();
    }
}

void MicrophoneService::selectMicrophone(int index) {
    if (m_availableMicrophones.isEmpty()) {
        return;
    }
    const int clamped = std::clamp(index, 0, static_cast<int>(m_availableMicrophones.size()) - 1);
    const int prev = m_selectedMicrophoneIndex.exchange(clamped);

    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("devices/microphoneName"),
                      m_availableMicrophones[clamped].toMap().value(QStringLiteral("label")).toString());

    if (prev != clamped) {
        emit selectedMicrophoneChanged();
        if (m_testingMic.load()) {
            stopMicTest();
            startMicTest();
        }
    }
}

void MicrophoneService::grantPermission() {
    setPermissionGranted(true);
}

void MicrophoneService::setPermissionGranted(bool granted) {
    updateOsPermissionState();
    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("permissions/microphoneGranted"), granted);
    if (m_permissionGranted != granted) {
        m_permissionGranted = granted;
        emit permissionChanged();
    }
    if (!granted) {
        cancelRecording();
        stopMicTest();
    } else {
        refreshDevices();
    }
}

void MicrophoneService::openPrivacySettings() {
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:privacy-microphone")));
#endif
}

void MicrophoneService::setMuted(bool muted) {
    QSettings settings(QStringLiteral("DevDialogue"), QStringLiteral("DevDialogue"));
    settings.setValue(QStringLiteral("audio/micMuted"), muted);
    if (m_muted != muted) {
        m_muted = muted;
        emit mutedChanged();
    }
    if (muted) {
        cancelRecording();
        stopMicTest();
        setStatus(tr("Microphone muted"));
    } else if (m_statusText == tr("Microphone muted")) {
        setStatus(QString());
    }
}

void MicrophoneService::toggleMuted() {
    setMuted(!m_muted);
}

void MicrophoneService::setApiBaseUrl(const QString& baseUrl) {
    QString clean = baseUrl.trimmed();
    while (clean.endsWith(QLatin1Char('/'))) {
        clean.chop(1);
    }
    if (!clean.isEmpty()) {
        m_apiBaseUrl = clean;
    }
}

void MicrophoneService::setTranscribing(bool busy) {
    if (m_transcribing != busy) {
        m_transcribing = busy;
        emit transcribingChanged();
    }
}

void MicrophoneService::setStatus(const QString& status) {
    if (m_statusText != status) {
        m_statusText = status;
        emit statusChanged();
    }
}

void MicrophoneService::updateInputLevel(qreal level) {
    const qreal clamped = std::clamp(level, 0.0, 1.0);
    if (!qFuzzyCompare(1.0 + m_inputLevel, 1.0 + clamped)) {
        m_inputLevel = clamped;
        emit inputLevelChanged();
    }
}

bool MicrophoneService::openWaveDevice() {
#ifdef Q_OS_WIN
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = Channels;
    wfx.nSamplesPerSec = SampleRate;
    wfx.wBitsPerSample = BitsPerSample;
    wfx.nBlockAlign = (Channels * BitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    const int chosenIdx = m_selectedMicrophoneIndex.load();
    const UINT deviceId = (!m_availableMicrophones.isEmpty() &&
                           chosenIdx >= 0 && chosenIdx < m_availableMicrophones.size())
        ? static_cast<UINT>(chosenIdx)
        : WAVE_MAPPER;

    HWAVEIN hWave = nullptr;
    MMRESULT res = waveInOpen(
        &hWave, deviceId, &wfx,
        reinterpret_cast<DWORD_PTR>(&Impl::waveInCallback),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION);

    if (res != MMSYSERR_NOERROR || !hWave) {
        setStatus(tr("Microphone unavailable"));
        emit transcriptionError(tr("Could not open selected microphone (code %1).").arg(static_cast<int>(res)));
        return false;
    }

    m_impl->hWaveIn = hWave;
    for (int i = 0; i < BufferCount; ++i) {
        m_impl->storage[i].assign(BufferBytes, 0);
        auto& hdr = m_impl->headers[i];
        hdr = WAVEHDR{};
        hdr.lpData = m_impl->storage[i].data();
        hdr.dwBufferLength = static_cast<DWORD>(BufferBytes);
        hdr.dwUser = static_cast<DWORD_PTR>(i);
        waveInPrepareHeader(hWave, &hdr, sizeof(WAVEHDR));
        waveInAddBuffer(hWave, &hdr, sizeof(WAVEHDR));
    }
    return true;
#else
    emit transcriptionError(tr("Microphone capture is only supported on Windows."));
    return false;
#endif
}

void MicrophoneService::startRecording() {
    if (m_recording.load() || m_transcribing) {
        return;
    }
    if (m_muted) {
        setStatus(tr("Microphone muted"));
        return;
    }
    updateOsPermissionState();
    if (!m_osPermissionAllowed) {
        setStatus(tr("Microphone blocked in Windows Privacy Settings"));
        emit permissionRequired();
        return;
    }
    if (!m_permissionGranted) {
        setStatus(tr("Microphone permission required"));
        emit permissionRequired();
        return;
    }

    if (m_testingMic.load()) {
        stopMicTest();
    }

#ifdef Q_OS_WIN
    {
        QMutexLocker locker(&m_pcmMutex);
        m_pcmBuffer.clear();
        m_pcmBuffer.reserve(SampleRate * 2 * 30);
    }

    if (!openWaveDevice()) {
        return;
    }

    m_recording.store(true);
    updateInputLevel(0.0);
    setStatus(tr("Listening..."));
    emit recordingChanged();
    waveInStart(m_impl->hWaveIn);
#else
    emit transcriptionError(tr("Microphone capture is only supported on Windows."));
#endif
}

void MicrophoneService::startMicTest() {
    if (m_recording.load() || m_transcribing || m_testingMic.load()) {
        return;
    }
    updateOsPermissionState();
    if (!m_osPermissionAllowed || !m_permissionGranted) {
        emit permissionRequired();
        return;
    }
#ifdef Q_OS_WIN
    if (!openWaveDevice()) {
        return;
    }
    m_testingMic.store(true);
    updateInputLevel(0.0);
    emit testingMicChanged();
    waveInStart(m_impl->hWaveIn);
#endif
}

void MicrophoneService::stopMicTest() {
    if (!m_testingMic.load()) {
        return;
    }
    stopHardwareCapture();
}

void MicrophoneService::toggleMicTest() {
    if (m_testingMic.load()) {
        stopMicTest();
    } else {
        startMicTest();
    }
}

void MicrophoneService::stopAndTranscribe() {
    if (!m_recording.load()) {
        return;
    }
    stopHardwareCapture();

    QByteArray pcm;
    {
        QMutexLocker locker(&m_pcmMutex);
        pcm = m_pcmBuffer;
        m_pcmBuffer.clear();
    }

    if (pcm.size() < 2400) {
        setStatus(tr("Recording too short"));
        emit noSpeechDetected();
        return;
    }

    sendToParakeet(pcm);
}

void MicrophoneService::toggleRecording() {
    if (m_transcribing) {
        return;
    }
    if (m_recording.load()) {
        stopAndTranscribe();
    } else {
        startRecording();
    }
}

void MicrophoneService::cancelRecording() {
    if (m_recording.load()) {
        stopHardwareCapture();
    }
    QMutexLocker locker(&m_pcmMutex);
    m_pcmBuffer.clear();
    setStatus(QString());
}

void MicrophoneService::handleCapturedBlock(const char* data, int size, int bufferIndex) {
    const bool recording = m_recording.load();
    const bool testing = m_testingMic.load();
    const bool flushing = m_flushingTail.load();
    if (!recording && !testing && !flushing) {
        return;
    }

    if (recording || flushing) {
        QMutexLocker locker(&m_pcmMutex);
        if (m_pcmBuffer.size() < SampleRate * 2 * 120) {
            m_pcmBuffer.append(data, size);
        }
    }

    const int sampleCount = size / 2;
    const auto* samples = reinterpret_cast<const qint16*>(data);
    double sumSq = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        const double norm = samples[i] / 32768.0;
        sumSq += norm * norm;
    }
    const double rms = sampleCount > 0 ? std::sqrt(sumSq / sampleCount) : 0.0;
    const qreal level = std::min(1.0, rms * 5.0);

    if (flushing) {
        return;
    }

    QMetaObject::invokeMethod(this, [this, bufferIndex, level]() {
        updateInputLevel(level);
#ifdef Q_OS_WIN
        if ((m_recording.load() || m_testingMic.load()) && m_impl && m_impl->hWaveIn &&
            bufferIndex >= 0 && bufferIndex < BufferCount) {
            waveInAddBuffer(m_impl->hWaveIn, &m_impl->headers[bufferIndex], sizeof(WAVEHDR));
        }
#else
        Q_UNUSED(bufferIndex)
#endif
    }, Qt::QueuedConnection);
}

void MicrophoneService::stopHardwareCapture() {
    const bool wasRecording = m_recording.exchange(false);
    const bool wasTesting = m_testingMic.exchange(false);
    updateInputLevel(0.0);

#ifdef Q_OS_WIN
    if (m_impl && m_impl->hWaveIn) {
        HWAVEIN hWave = m_impl->hWaveIn;
        m_impl->hWaveIn = nullptr;
        m_flushingTail.store(wasRecording);
        waveInReset(hWave);
        m_flushingTail.store(false);
        for (int i = 0; i < BufferCount; ++i) {
            waveInUnprepareHeader(hWave, &m_impl->headers[i], sizeof(WAVEHDR));
        }
        waveInClose(hWave);
    }
#endif

    if (wasRecording) {
        emit recordingChanged();
    }
    if (wasTesting) {
        emit testingMicChanged();
    }
}

QByteArray MicrophoneService::buildWavPayload(const QByteArray& pcmBytes, int sampleRate) {
    QByteArray wav;
    wav.resize(44);
    char* h = wav.data();

    const quint32 dataSize = static_cast<quint32>(pcmBytes.size());
    const quint32 riffSize = 36 + dataSize;
    const quint16 channels = 1;
    const quint16 bitsPerSample = 16;
    const quint32 byteRate = static_cast<quint32>(sampleRate) * channels * (bitsPerSample / 8);
    const quint16 blockAlign = channels * (bitsPerSample / 8);

    std::memcpy(h + 0, "RIFF", 4);
    qToLittleEndian<quint32>(riffSize, h + 4);
    std::memcpy(h + 8, "WAVE", 4);
    std::memcpy(h + 12, "fmt ", 4);
    qToLittleEndian<quint32>(16, h + 16);
    qToLittleEndian<quint16>(1, h + 20);
    qToLittleEndian<quint16>(channels, h + 22);
    qToLittleEndian<quint32>(static_cast<quint32>(sampleRate), h + 24);
    qToLittleEndian<quint32>(byteRate, h + 28);
    qToLittleEndian<quint16>(blockAlign, h + 32);
    qToLittleEndian<quint16>(bitsPerSample, h + 34);
    std::memcpy(h + 36, "data", 4);
    qToLittleEndian<quint32>(dataSize, h + 40);

    wav.append(pcmBytes);
    return wav;
}

void MicrophoneService::sendToParakeet(const QByteArray& pcmBytes) {
    setTranscribing(true);
    setStatus(tr("Transcribing (Parakeet)..."));

    const QByteArray wav = buildWavPayload(pcmBytes);
    QNetworkRequest req(QUrl(m_apiBaseUrl + QStringLiteral("/api/stt")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("audio/wav"));
    req.setTransferTimeout(45000);

    QNetworkReply* reply = m_nam.post(req, wav);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setTranscribing(false);

        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || code < 200 || code >= 300) {
            QString errMsg = reply->errorString();
            const QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject() && doc.object().contains(QLatin1String("error"))) {
                errMsg = doc.object().value(QLatin1String("error")).toString();
            }
            setStatus(tr("STT error"));
            emit transcriptionError(errMsg);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QString text = doc.object().value(QLatin1String("text")).toString().trimmed();
        if (text.isEmpty()) {
            setStatus(tr("No speech detected"));
            emit noSpeechDetected();
            return;
        }

        setStatus(QString());
        emit transcriptionReady(text);
    });
}

} // namespace devdialogue::services
