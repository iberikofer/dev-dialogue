# Dev Dialogue: Web (Next.js) to C++ Desktop App Migration Plan

## 📌 Executive Summary

This documentation suite provides a comprehensive, production-grade architectural and implementation plan to migrate the **Dev Dialogue** interview simulator frontend from **Next.js 14 (TypeScript)** to a high-performance **native C++ desktop application**.

The native desktop application will preserve 100% of the functional capabilities, design polish, dual-language capabilities (English & Ukrainian), and offline demo robustness of the current web platform while gaining the low-latency responsiveness, minimal memory footprint (<30 MB RAM vs 300+ MB), OS-level security (credential vaults), and distribution autonomy of native C++.

---

## 🗺 Documentation Map

The migration plan is partitioned into specialized, deep-dive implementation guides:

| File | Focus Area | Description |
| :--- | :--- | :--- |
| [**00_overview_and_architecture.md**](./00_overview_and_architecture.md) | **Architecture & Tech Stack** | Technology selection (Qt 6 QML vs Widgets vs Slint), Clean Architecture/MVVM design, communication protocols, memory and concurrency models. |
| [**01_project_structure_and_cmake.md**](./01_project_structure_and_cmake.md) | **Task 1: Basic Project Structure & CMake** | Root folder layout, modern CMake 3.25+ configuration, dependency management via `vcpkg`, asset bundling, cross-platform build pipelines (Windows, Linux, macOS). |
| [**02_data_models_and_networking.md**](./02_data_models_and_networking.md) | **Contracts, Serialization & API Client** | C++20 port of `@dev-dialogue/api-contracts`, `nlohmann::json` serialization, asynchronous `QNetworkAccessManager` client, bearer token lifecycle, retry policies. |
| [**03_state_management_and_auth.md**](./03_state_management_and_auth.md) | **State Management & Credential Vault** | Porting Zustand stores to C++ ViewModels, OS-level secure storage (Windows DPAPI / Credential Manager, macOS Keychain, Linux SecretService), Admin/Candidate view switcher. |
| [**04_ui_design_and_screens.md**](./04_ui_design_and_screens.md) | **UI Design System & Screen Porting** | Recreating Tailwind CSS dark-mode glassmorphism, Geist typography, interactive Chat Room, Scorecard/Report gauges, Dashboard KPI grid, and Admin Question Bank in QML. |
| [**05_offline_engine_and_i18n.md**](./05_offline_engine_and_i18n.md) | **Offline AI Engine & Internationalization** | Embedded native C++ port of `MockAiService.cs`, question banks for all tracks and levels, dynamic real-time runtime language switching (UK / EN) with Qt Linguist. |
| [**06_testing_packaging_and_milestones.md**](./06_testing_packaging_and_milestones.md) | **Testing, Packaging & Milestones** | GoogleTest and Qt Test suites, `windeployqt` installer packaging, GitHub Actions CI/CD matrix, and phased 9-step implementation roadmap. |

---

## 🎯 Architecture Decision Matrix

```
┌─────────────────────────────────────────────────────────────┐
│                       Dev Dialogue GUI                       │
│      QML (Qt Quick 6.7+) Hardware-Accelerated Scene Graph    │
│   Dark Zinc Glassmorphism · Spring Physics · Geist Fonts   │
└──────────────────────────────┬──────────────────────────────┘
                               │ Q_PROPERTY bindings / Signals
┌──────────────────────────────▼──────────────────────────────┐
│                    C++20 ViewModels (MVVM)                  │
│ AuthViewModel · DashboardViewModel · ChatViewModel · Admin  │
└──────┬───────────────────────┬───────────────────────┬──────┘
       │                       │                       │
┌──────▼────────┐       ┌──────▼────────┐       ┌──────▼──────┐
│  State/Stores │       │   ApiClient   │       │ MockEngine  │
│ Auth / Config │       │  HTTP / REST  │       │ Offline AI  │
│  Secure Vault │       │  (QtNetwork)  │       │ (Embedded)  │
└───────────────┘       └───────┬───────┘       └─────────────┘
                                │ Bearer JWT
                        ┌───────▼──────────────┐
                        │ ASP.NET Core Backend │
                        │   (.NET 9 Web API)   │
                        └──────────────────────┘
```

---

## 🚀 Key Advantages Over the Web Client

1. **Zero Cold-Boot Overhead:** The application starts in under 60 milliseconds with no Node.js/Chromium overhead.
2. **Deterministic Memory Footprint:** Operates strictly within 20–35 MB of RAM under active interview streaming.
3. **Hardware-Accelerated 120 FPS Animations:** Rendered directly via Direct3D 11/12 (Windows), Metal (macOS), or Vulkan (Linux).
4. **Hardware Secure Storage:** Sensitive authentication tokens are stored in encrypted OS credential vaults rather than plain browser `localStorage`.
5. **Air-Gapped Autonomous Operation:** Seamless failover to native embedded C++ question generation when the backend network connection is absent.

