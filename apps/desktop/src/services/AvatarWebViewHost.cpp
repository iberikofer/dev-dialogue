#include "services/AvatarWebViewHost.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include "WebView2.h"
#include <atomic>
#endif

namespace devdialogue::services {

#ifdef Q_OS_WIN
namespace {

using CreateEnvWithOptsFn = HRESULT(STDAPICALLTYPE*)(
    PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

template <typename Interface, const IID* InterfaceIid, typename Callback>
class ComCallback final : public Interface {
public:
    explicit ComCallback(Callback cb) : m_cb(std::move(cb)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, *InterfaceIid)) {
            *ppv = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++m_ref;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = --m_ref;
        if (n == 0) delete this;
        return n;
    }

    template <typename Arg1, typename Arg2>
    HRESULT InvokeImpl(Arg1 a1, Arg2 a2) {
        return m_cb(a1, a2);
    }

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT a1, ICoreWebView2Environment* a2) {
        return InvokeImpl(a1, a2);
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT a1, ICoreWebView2Controller* a2) {
        return InvokeImpl(a1, a2);
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* a1, ICoreWebView2NavigationCompletedEventArgs* a2) {
        return InvokeImpl(a1, a2);
    }

private:
    std::atomic<ULONG> m_ref{1};
    Callback m_cb;
};

template <typename Interface, const IID* InterfaceIid, typename Callback>
Interface* makeComHandler(Callback&& cb) {
    return new ComCallback<Interface, InterfaceIid, std::decay_t<Callback>>(std::forward<Callback>(cb));
}

} // namespace

struct AvatarWebViewHost::Impl {
    HMODULE loaderDll{nullptr};
    CreateEnvWithOptsFn createEnv{nullptr};
    HWND parentHwnd{nullptr};
    HWND hostHwnd{nullptr};
    ICoreWebView2Environment* env{nullptr};
    ICoreWebView2Controller* controller{nullptr};
    ICoreWebView2* webview{nullptr};
    EventRegistrationToken navCompletedToken{};
};
#else
struct AvatarWebViewHost::Impl {};
#endif

AvatarWebViewHost::AvatarWebViewHost(QObject* parent)
    : QObject(parent), m_impl(new Impl()) {}

AvatarWebViewHost::~AvatarWebViewHost() {
    destroyNative();
    delete m_impl;
    m_impl = nullptr;
}

QString AvatarWebViewHost::buildEmbedUrl(const QString& baseUrl) {
    QString clean = baseUrl.trimmed();
    while (clean.endsWith(QLatin1Char('/'))) {
        clean.chop(1);
    }
    if (clean.isEmpty()) {
        clean = QStringLiteral("http://localhost:5238");
    }
    return clean + QStringLiteral("/avatar/model.html?embed=1");
}

void AvatarWebViewHost::attachToWindow(QQuickWindow* window, const QString& baseUrl) {
    m_window = window;
    if (!baseUrl.trimmed().isEmpty()) {
        setBaseUrl(baseUrl);
    } else if (m_targetUrl.isEmpty()) {
        setBaseUrl(QStringLiteral("http://localhost:5238"));
    }
    if (m_window) {
        connect(m_window, &QObject::destroyed, this, [this]() {
            m_window = nullptr;
            destroyNative();
        });
    }
    ensureInitialized();
}

void AvatarWebViewHost::setBaseUrl(const QString& baseUrl) {
    const QString nextUrl = buildEmbedUrl(baseUrl);
    if (m_targetUrl == nextUrl) {
        return;
    }
    m_targetUrl = nextUrl;
    emit urlChanged();
#ifdef Q_OS_WIN
    if (m_impl && m_impl->webview) {
        m_pageReady = false;
        emit readyChanged();
        const std::wstring wurl = m_targetUrl.toStdWString();
        m_impl->webview->Navigate(wurl.c_str());
    }
#endif
}

void AvatarWebViewHost::reload() {
    if (m_targetUrl.isEmpty()) {
        return;
    }
#ifdef Q_OS_WIN
    if (m_impl && m_impl->webview) {
        m_pageReady = false;
        emit readyChanged();
        const std::wstring wurl = m_targetUrl.toStdWString();
        m_impl->webview->Navigate(wurl.c_str());
    }
#endif
}

void AvatarWebViewHost::focusHostWindow() {
#ifdef Q_OS_WIN
    if (m_impl && m_impl->parentHwnd) {
        SetFocus(m_impl->parentHwnd);
    }
#endif
}

void AvatarWebViewHost::setViewport(qreal x, qreal y, qreal width, qreal height, bool visible) {
    const QRect nextBounds(qRound(x), qRound(y), std::max(0, qRound(width)), std::max(0, qRound(height)));
    const bool validRect = visible && nextBounds.width() > 20 && nextBounds.height() > 20;
    m_logicalBounds = nextBounds;
    m_visible = validRect;

    if (m_visible && m_window && !m_impl->controller && !m_initializing) {
        ensureInitialized();
    }
    updateHostBounds();
}

void AvatarWebViewHost::runScript(const QString& script) {
    if (script.isEmpty()) {
        return;
    }
#ifdef Q_OS_WIN
    if (m_pageReady && m_impl && m_impl->webview) {
        const std::wstring wscript = script.toStdWString();
        m_impl->webview->ExecuteScript(wscript.c_str(), nullptr);
        return;
    }
#endif
    if (m_pendingScripts.size() < 48) {
        m_pendingScripts.append(script);
    }
}

void AvatarWebViewHost::speak(const QString& text, const QString& emotion,
                              const QStringList& actionsSequence, const QJsonArray& uiCommands,
                              const QString& voice, const QString& gender) {
    QJsonObject payload;
    payload.insert(QStringLiteral("text"), text);
    if (!emotion.isEmpty()) payload.insert(QStringLiteral("emotion"), emotion);
    if (!actionsSequence.isEmpty()) payload.insert(QStringLiteral("actionsSequence"), QJsonArray::fromStringList(actionsSequence));
    if (!uiCommands.isEmpty()) payload.insert(QStringLiteral("uiCommands"), uiCommands);
    if (!voice.isEmpty()) payload.insert(QStringLiteral("voice"), voice);
    if (!gender.isEmpty()) payload.insert(QStringLiteral("gender"), gender);

    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.speak(%1);").arg(json));
}

void AvatarWebViewHost::react(const QString& emotion, const QStringList& actionsSequence,
                              const QJsonArray& uiCommands, const QString& gender) {
    QJsonObject payload;
    if (!emotion.isEmpty()) payload.insert(QStringLiteral("emotion"), emotion);
    if (!actionsSequence.isEmpty()) payload.insert(QStringLiteral("actionsSequence"), QJsonArray::fromStringList(actionsSequence));
    if (!uiCommands.isEmpty()) payload.insert(QStringLiteral("uiCommands"), uiCommands);
    if (!gender.isEmpty()) payload.insert(QStringLiteral("gender"), gender);

    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.react(%1);").arg(json));
}

void AvatarWebViewHost::stopSpeech() {
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.stopSpeech();"));
}

void AvatarWebViewHost::setVolume(qreal volume) {
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setVolume(%1);").arg(volume, 0, 'f', 3));
}

void AvatarWebViewHost::setMuted(bool muted) {
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setMuted(%1);")
                  .arg(muted ? QStringLiteral("true") : QStringLiteral("false")));
}

void AvatarWebViewHost::setOutputMode(const QString& mode) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{mode}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setOutputMode((%1)[0]);").arg(json));
}

void AvatarWebViewHost::setSoundEffect(const QString& soundFile) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{soundFile}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setSoundEffect((%1)[0]);").arg(json));
}

void AvatarWebViewHost::setVoice(const QString& voiceId) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{voiceId}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setVoice((%1)[0]);").arg(json));
}

void AvatarWebViewHost::setGender(const QString& gender) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{gender}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setGender((%1)[0]);").arg(json));
}

void AvatarWebViewHost::setAccessory(const QString& name, bool visible) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{name}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setAccessory((%1)[0], %2);")
                  .arg(json, visible ? QStringLiteral("true") : QStringLiteral("false")));
}

void AvatarWebViewHost::setPreset(const QString& presetId) {
    const QString json = QString::fromUtf8(QJsonDocument(QJsonArray{presetId}).toJson(QJsonDocument::Compact));
    runScript(QStringLiteral("window.devDialogueAvatar && window.devDialogueAvatar.setPreset((%1)[0]);").arg(json));
}

void AvatarWebViewHost::ensureInitialized() {
#ifdef Q_OS_WIN
    if (!m_window || !m_impl || m_impl->controller || m_initializing) {
        return;
    }

    HWND parent = reinterpret_cast<HWND>(m_window->winId());
    if (!parent) {
        return;
    }

    if (!m_impl->loaderDll) {
        const QString besideExe = QCoreApplication::applicationDirPath() + QStringLiteral("/WebView2Loader.dll");
        m_impl->loaderDll = LoadLibraryW(besideExe.toStdWString().c_str());
        if (!m_impl->loaderDll) {
            m_impl->loaderDll = LoadLibraryW(L"WebView2Loader.dll");
        }
        if (m_impl->loaderDll) {
            m_impl->createEnv = reinterpret_cast<CreateEnvWithOptsFn>(
                GetProcAddress(m_impl->loaderDll, "CreateCoreWebView2EnvironmentWithOptions"));
        }
    }
    if (!m_impl->createEnv) {
        return;
    }

    if (!m_impl->hostHwnd) {
        m_impl->parentHwnd = parent;
        m_impl->hostHwnd = CreateWindowExW(
            0, L"STATIC", L"",
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_DISABLED,
            0, 0, 10, 10,
            parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    }
    if (!m_impl->hostHwnd) {
        return;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_initializing = true;

    const QString userData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                           + QStringLiteral("/WebView2Avatar");
    QDir().mkpath(userData);
    const std::wstring wUserData = userData.toStdWString();

    auto* envHandler = makeComHandler<
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
        &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [this](HRESULT hr, ICoreWebView2Environment* createdEnv) -> HRESULT {
            if (FAILED(hr) || !createdEnv || !m_impl || !m_impl->hostHwnd) {
                m_initializing = false;
                return S_OK;
            }
            m_impl->env = createdEnv;
            m_impl->env->AddRef();

            auto* ctrlHandler = makeComHandler<
                ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
                &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT hrCtrl, ICoreWebView2Controller* createdCtrl) -> HRESULT {
                    m_initializing = false;
                    if (FAILED(hrCtrl) || !createdCtrl || !m_impl) {
                        return S_OK;
                    }
                    m_impl->controller = createdCtrl;
                    m_impl->controller->AddRef();
                    createdCtrl->get_CoreWebView2(&m_impl->webview);

                    ICoreWebView2Controller2* ctrl2 = nullptr;
                    if (SUCCEEDED(createdCtrl->QueryInterface(IID_ICoreWebView2Controller2,
                                                             reinterpret_cast<void**>(&ctrl2))) && ctrl2) {
                        COREWEBVIEW2_COLOR bg{255, 14, 14, 17};
                        ctrl2->put_DefaultBackgroundColor(bg);
                        ctrl2->Release();
                    }

                    if (m_impl->webview) {
                        ICoreWebView2Settings* settings = nullptr;
                        if (SUCCEEDED(m_impl->webview->get_Settings(&settings)) && settings) {
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_IsStatusBarEnabled(FALSE);
                            settings->put_IsZoomControlEnabled(FALSE);
                            settings->Release();
                        }

                        auto* navHandler = makeComHandler<
                            ICoreWebView2NavigationCompletedEventHandler,
                            &IID_ICoreWebView2NavigationCompletedEventHandler>(
                            [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                BOOL ok = FALSE;
                                if (args) args->get_IsSuccess(&ok);
                                m_pageReady = (ok == TRUE);
                                emit readyChanged();
                                if (m_pageReady) {
                                    QTimer::singleShot(150, this, &AvatarWebViewHost::flushPendingScripts);
                                }
                                focusHostWindow();
                                return S_OK;
                            });
                        m_impl->webview->add_NavigationCompleted(navHandler, &m_impl->navCompletedToken);
                        navHandler->Release();

                        if (!m_targetUrl.isEmpty()) {
                            const std::wstring wurl = m_targetUrl.toStdWString();
                            m_impl->webview->Navigate(wurl.c_str());
                        }
                    }

                    m_available = true;
                    emit availableChanged();
                    updateHostBounds();
                    focusHostWindow();
                    return S_OK;
                });

            m_impl->env->CreateCoreWebView2Controller(m_impl->hostHwnd, ctrlHandler);
            ctrlHandler->Release();
            return S_OK;
        });

    HRESULT hr = m_impl->createEnv(nullptr, wUserData.c_str(), nullptr, envHandler);
    envHandler->Release();
    if (FAILED(hr)) {
        m_initializing = false;
    }
#endif
}

void AvatarWebViewHost::updateHostBounds() {
#ifdef Q_OS_WIN
    if (!m_impl || !m_impl->hostHwnd) {
        return;
    }
    if (!m_visible) {
        if (m_impl->controller) {
            m_impl->controller->put_IsVisible(FALSE);
        }
        ShowWindow(m_impl->hostHwnd, SW_HIDE);
        return;
    }

    const qreal dpr = m_window ? m_window->devicePixelRatio() : 1.0;
    const int pxX = qRound(m_logicalBounds.x() * dpr);
    const int pxY = qRound(m_logicalBounds.y() * dpr);
    const int pxW = std::max(1, qRound(m_logicalBounds.width() * dpr));
    const int pxH = std::max(1, qRound(m_logicalBounds.height() * dpr));

    SetWindowPos(m_impl->hostHwnd, HWND_TOP, pxX, pxY, pxW, pxH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    if (m_impl->controller) {
        RECT bounds{0, 0, pxW, pxH};
        m_impl->controller->put_Bounds(bounds);
        m_impl->controller->put_IsVisible(TRUE);
    }
#endif
}

void AvatarWebViewHost::flushPendingScripts() {
    if (!m_pageReady || m_pendingScripts.isEmpty()) {
        return;
    }
    const QStringList batch = m_pendingScripts;
    m_pendingScripts.clear();
    for (const QString& s : batch) {
        runScript(s);
    }
}

void AvatarWebViewHost::destroyNative() {
#ifdef Q_OS_WIN
    if (!m_impl) return;
    if (m_impl->webview) {
        if (m_impl->navCompletedToken.value != 0) {
            m_impl->webview->remove_NavigationCompleted(m_impl->navCompletedToken);
            m_impl->navCompletedToken.value = 0;
        }
        m_impl->webview->Release();
        m_impl->webview = nullptr;
    }
    if (m_impl->controller) {
        m_impl->controller->Close();
        m_impl->controller->Release();
        m_impl->controller = nullptr;
    }
    if (m_impl->env) {
        m_impl->env->Release();
        m_impl->env = nullptr;
    }
    if (m_impl->hostHwnd) {
        DestroyWindow(m_impl->hostHwnd);
        m_impl->hostHwnd = nullptr;
    }
    if (m_impl->loaderDll) {
        FreeLibrary(m_impl->loaderDll);
        m_impl->loaderDll = nullptr;
    }
    m_impl->createEnv = nullptr;
    m_pageReady = false;
    m_available = false;
#endif
}

} // namespace devdialogue::services
