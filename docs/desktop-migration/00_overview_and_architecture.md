# Document 00: Architecture Strategy & Framework Evaluation

## 1. Executive Justification for C++ Desktop Migration

The transition of **Dev Dialogue** from a Next.js 14 web client to a native C++ desktop application addresses several key non-functional and operational requirements:

1. **Deterministic Performance & Resource Efficiency:**
   - Web browsers and Electron wrappers consume 300–600 MB of system RAM just to host a basic chat view.
   - A compiled C++ desktop binary operates within **20–35 MB of RAM** and starts in **<60 ms**.
2. **Local Air-Gapped Simulation (Offline Zero-Latency):**
   - Olympiad competitions, offline testing, or constrained networking environments require 100% offline functionality. While the web version has mock fallbacks, the desktop app can embed the entire mock inference and evaluation pipeline as a native C++ library with zero external dependencies.
3. **Hardware-Accelerated UI & Micro-interactions:**
   - Smooth 60/120 FPS transitions, spring physics, and typography rendering directly mapped to native graphics APIs (Direct3D 12 on Windows, Metal on macOS, Vulkan on Linux).
4. **Enhanced OS Integration & Security:**
   - Authentication tokens stored inside native hardware-backed keychains (Windows DPAPI / Credential Manager, macOS Keychain) instead of inspectable browser `localStorage`.
   - Low-latency native audio APIs ready for future voice interview modules.

---

## 2. Comprehensive C++ GUI Framework Evaluation

| Criteria | **Qt 6 (Qt Quick / QML)** | **Qt 6 (QtWidgets)** | **Slint (C++ Backend)** | **Dear ImGui** | **WinUI 3 / C++/WinRT** |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **UI Paradigm** | Declarative, GPU-rendered Scene Graph | Imperative, CPU rasterized | Declarative, modern DSL | Immediate Mode (gaming/tools) | Declarative XAML |
| **Styling Flexibility** | CSS-like states, glassmorphism, fluid animations | QSS (Qt Style Sheets) - fragile, limited radius/blur | Modern CSS-like properties | Low (requires custom draw lists) | Modern Fluent Design |
| **Data Binding** | Native reactive property bindings (like React/Zustand) | Manual signals & slot wiring | Reactive properties | Per-frame polling | XAML `{x:Bind}` |
| **Cross-Platform** | Windows, macOS, Linux, Android, iOS | Windows, macOS, Linux | Windows, macOS, Linux | Windows, macOS, Linux | **Windows Only** |
| **Ecosystem Maturity** | Extreme (30+ years, enterprise production) | Extreme | Emerging (rapidly maturing) | High (games/dev tools) | High (Windows ecosystem) |
| **Memory Footprint** | ~25–35 MB | ~15–20 MB | ~10–15 MB | ~8–12 MB | ~50–80 MB |
| **Recommendation** | **SELECTED (Tier 1)** | Unsuitable for modern dark glassmorphism | Viable alternative, but smaller ecosystem | Not suited for consumer web-like typography | Rejects cross-platform goal |

### Why Qt 6 with Qt Quick / QML is the Winning Choice

1. **Direct 1:1 Mapping to React / Next.js Mental Model:**
   - QML utilizes a declarative JSON/JSX-like syntax with property bindings that function identically to reactive state updates in React.
   - Transitioning from Tailwind CSS classes to QML properties (`color: "#09090b"`, `radius: 12`, `border.color: "#27272a"`) is intuitive and direct.
2. **Built-in Modern Infrastructure:**
   - `QNetworkAccessManager` handles asynchronous HTTP/REST and SSL.
   - `QTranslator` and `QLocale` replace `translations.ts` and `next-intl` with native i18n support.
   - `QAbstractListModel` provides high-performance virtualized scrolling for chat transcripts and question banks.
3. **C++20 Power Behind the Curtain:**
   - High-throughput business logic, JSON serialization (`nlohmann::json`), secure encryption, and background worker threads remain strictly in modern standard C++20.

---

## 3. High-Level Clean Architecture (MVVM)

The application follows the **Model-View-ViewModel (MVVM)** architectural pattern:

```
┌─────────────────────────────────────────────────────────────┐
│                        VIEW LAYER                           │
│     QML Files: Views, Components, Theme, Design Tokens      │
│  (ChatRoomView.qml, DashboardView.qml, AdminPanel.qml, etc) │
└──────────────────────────────▲──────────────────────────────┘
                               │ Property Bindings & Q_INVOKABLE
┌──────────────────────────────▼──────────────────────────────┐
│                     VIEWMODEL LAYER                         │
│                  C++ Classes (: public QObject)              │
│   AuthViewModel, DashboardViewModel, ChatViewModel, AdminVM │
└──────────────────────────────▲──────────────────────────────┘
                               │ Business Operations
┌──────────────────────────────▼──────────────────────────────┐
│                      SERVICE LAYER                          │
│     AuthService, InterviewService, AdminService, I18nMgr    │
└──────────────┬───────────────────────────────┬──────────────┘
               │                               │
┌──────────────▼──────────────┐ ┌──────────────▼──────────────┐
│      NETWORKING LAYER       │ │    LOCAL REPOSITORY LAYER   │
│ ApiClient (QNetworkAccess)  │ │ SecureStorage (DPAPI/Keych) │
│ DTO Serialization (nlohmann)│ │ LocalMockEngine (Offline AI)│
└─────────────────────────────┘ └─────────────────────────────┘
```

### Architectural Rules

1. **Zero Business Logic in QML:** QML is strictly responsible for rendering, animation states, user input events, and data binding.
2. **Thread Safety & Non-Blocking UI:** Network operations, heavy JSON parsing, and mock AI evaluations occur on background threads (`QThreadPool` / `QtConcurrent`). ViewModels dispatch updates to the main thread via Qt signals.
3. **Single Source of Truth:** Core states (User identity, Active session, Backend health) live in C++ singleton services, exposed to QML through read-only `Q_PROPERTY` bindings.

---

## 4. Monorepo Integration Strategy

The C++ desktop project will be integrated directly into the existing monorepo under `apps/desktop/`:

```text
dev-dialogue/
├── apps/
│   ├── web/                    # Existing Next.js 14 web app
│   ├── api/                    # Existing ASP.NET Core .NET 9 API
│   └── desktop/                # NEW: Native C++20 / Qt 6 Desktop Client
│       ├── CMakeLists.txt
│       ├── vcpkg.json
│       ├── include/
│       ├── src/
│       ├── qml/
│       └── resources/
│
├── packages/
│   └── api-contracts/          # Shared contracts (source for C++ DTO generation)
│
├── .agents/
├── docker-compose.yml
└── package.json                # Monorepo scripts (desktop build hooks)
```

In `package.json`, helper scripts can be added to invoke CMake configure and build targets:
```json
"scripts": {
  "desktop:configure": "cmake -B apps/desktop/build -S apps/desktop -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake",
  "desktop:build": "cmake --build apps/desktop/build --config Release",
  "desktop:dev": "cmake --build apps/desktop/build --target run"
}
```

