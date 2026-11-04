# Document 03: State Management, Auth & Secure Storage

This document details how the client-side state architecture managed by **Zustand** in `apps/web/src/store/` is mapped to C++ ViewModels, thread-safe services, and native OS credential vaults.

---

## 1. Mapping Zustand Stores to C++

| Next.js / Zustand Store | C++ Desktop Equivalent | Storage / Lifetime |
| :--- | :--- | :--- |
| `useAuthStore` | `AuthService` (Singleton) + `AuthViewModel` (QML Interface) | Encrypted via OS Credential Vault (DPAPI / Keychain) |
| `useBackendStatusStore` | `ApiClient` (Q_PROPERTY `isOnline`, `lastError`) | In-memory session state + 30s heartbeat timer |
| `useLanguageStore` | `I18nManager` (`QTranslator`, `QLocale`) | `QSettings` (`~/.config/DevDialogue` or Registry) |
| Active View / Routing | `NavigationViewModel` (`currentViewIndex`) | In-memory UI view stack |

---

## 2. Hardware-Backed Secure Storage (`SecureStorage.hpp` & `.cpp`)

In the Next.js web application, JWT tokens and user objects are stored in browser `localStorage` as plaintext strings (`STORAGE_KEY = 'dev_dialogue_auth'`). In a native C++ desktop application, tokens must be stored in hardware-encrypted OS keychains:

- **Windows:** Data Protection API (DPAPI) via `CryptProtectData` or Windows Credential Manager (`CredWriteW`).
- **macOS:** Apple Keychain Services (`SecItemAdd` / `SecItemCopyMatching`).
- **Linux:** FreeDesktop Secret Service via `libsecret`.

### C++ Header: `include/services/SecureStorage.hpp`

```cpp
#pragma once

#include <string>
#include <optional>

namespace devdialogue::services {

class SecureStorage {
public:
    SecureStorage() = default;
    ~SecureStorage() = default;

    bool writeCredential(const std::string& key, const std::string& secret);
    std::optional<std::string> readCredential(const std::string& key);
    bool deleteCredential(const std::string& key);

private:
#if defined(_WIN32)
    std::wstring toWide(const std::string& str);
    std::string toNarrow(const std::wstring& wstr);
#endif
};

} // namespace devdialogue::services
```

### Windows Implementation: `src/services/SecureStorage.cpp`

```cpp
#include "services/SecureStorage.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <wincred.h>

namespace devdialogue::services {

std::wstring SecureStorage::toWide(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
    return wstr;
}

std::string SecureStorage::toNarrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], sizeNeeded, NULL, NULL);
    return str;
}

bool SecureStorage::writeCredential(const std::string& key, const std::string& secret) {
    std::wstring targetName = L"DevDialogue_" + toWide(key);
    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(targetName.c_str());
    cred.CredentialBlobSize = static_cast<DWORD>(secret.size());
    cred.CredentialBlob = (LPBYTE)secret.data();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    return CredWriteW(&cred, 0) == TRUE;
}

std::optional<std::string> SecureStorage::readCredential(const std::string& key) {
    std::wstring targetName = L"DevDialogue_" + toWide(key);
    PCREDENTIALW pCred = NULL;

    if (CredReadW(targetName.c_str(), CRED_TYPE_GENERIC, 0, &pCred)) {
        std::string secret(reinterpret_cast<char*>(pCred->CredentialBlob), pCred->CredentialBlobSize);
        CredFree(pCred);
        return secret;
    }
    return std::nullopt;
}

bool SecureStorage::deleteCredential(const std::string& key) {
    std::wstring targetName = L"DevDialogue_" + toWide(key);
    return CredDeleteW(targetName.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
}

} // namespace devdialogue::services
#endif
```

---

## 3. AuthViewModel: Exposing Auth & Role Toggle to QML

The `AuthViewModel` replicates `useAuthStore.ts`, including the instantaneous **Admin View / Candidate View** toggle:

### Header: `include/viewmodels/AuthViewModel.hpp`

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "services/AuthService.hpp"

namespace devdialogue::viewmodels {

class AuthViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStateChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY authStateChanged)
    Q_PROPERTY(bool isAdminViewActive READ isAdminViewActive WRITE setAdminViewActive NOTIFY adminViewChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY authStateChanged)
    Q_PROPERTY(QString userFullName READ userFullName NOTIFY authStateChanged)
    Q_PROPERTY(bool isLoginModalOpen READ isLoginModalOpen WRITE setLoginModalOpen NOTIFY loginModalChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorOccurred)

public:
    explicit AuthViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent = nullptr);

    bool isAuthenticated() const;
    bool isAdmin() const;
    bool isAdminViewActive() const;
    void setAdminViewActive(bool active);

    QString userEmail() const;
    QString userFullName() const;

    bool isLoginModalOpen() const { return m_isLoginModalOpen; }
    void setLoginModalOpen(bool open);

    bool isBusy() const { return m_isBusy; }
    QString errorMessage() const { return m_errorMessage; }

    // QML-invokable actions
    Q_INVOKABLE void loginWithEmail(const QString& email, const QString& password);
    Q_INVOKABLE void registerUser(const QString& email, const QString& password, const QString& fullName);
    Q_INVOKABLE void mockLogin(const QString& roleStr = "candidate");
    Q_INVOKABLE void toggleAdminView();
    Q_INVOKABLE void logout();

signals:
    void authStateChanged();
    void adminViewChanged();
    void loginModalChanged();
    void busyStateChanged();
    void errorOccurred(const QString& error);

private:
    std::shared_ptr<services::AuthService> m_authService;
    bool m_isAdminViewActive{true};
    bool m_isLoginModalOpen{false};
    bool m_isBusy{false};
    QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
```

### Implementation Highlights: `src/viewmodels/AuthViewModel.cpp`

```cpp
#include "viewmodels/AuthViewModel.hpp"

namespace devdialogue::viewmodels {

AuthViewModel::AuthViewModel(std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_authService(std::move(authService)) {
    connect(m_authService.get(), &services::AuthService::userChanged, this, &AuthViewModel::authStateChanged);
}

bool AuthViewModel::isAuthenticated() const {
    return m_authService->currentUser().has_value();
}

bool AuthViewModel::isAdmin() const {
    auto user = m_authService->currentUser();
    return user && user->role == models::UserRole::Admin;
}

bool AuthViewModel::isAdminViewActive() const {
    return m_isAdminViewActive;
}

void AuthViewModel::setAdminViewActive(bool active) {
    if (m_isAdminViewActive != active) {
        m_isAdminViewActive = active;
        emit adminViewChanged();
    }
}

void AuthViewModel::toggleAdminView() {
    setAdminViewActive(!m_isAdminViewActive);
}

void AuthViewModel::mockLogin(const QString& roleStr) {
    m_authService->mockLogin(roleStr == "admin" ? models::UserRole::Admin : models::UserRole::User);
    setLoginModalOpen(false);
}

void AuthViewModel::logout() {
    m_authService->logout();
}

} // namespace devdialogue::viewmodels
```

---

## 4. Navigation & View Routing (`NavigationViewModel`)

In Next.js, view navigation occurs via URLs (`/`, `/dashboard`, `/interview/setup`, `/interview/:id`, `/admin`). In the C++ desktop application, navigation is managed by the `NavigationViewModel` controlling the QML `StackLayout` index:

```cpp
enum class AppScreen {
    Landing = 0,
    Dashboard = 1,
    InterviewSetup = 2,
    ChatRoom = 3,
    Report = 4,
    AdminPanel = 5
};
```

QML calls `navVM.navigateToChat(interviewId)` or `navVM.navigateToReport(interviewId)`, triggering an instantaneous GPU transition with no page reload.

