# Document 01: Project Structure, CMake & Dependency Setup (Task 1)

This document specifies **Task 1** of the migration roadmap: establishing the baseline C++ desktop application structure, CMake build system, dependency management, entry point, and QML engine initialization.

---

## 1. Directory Structure (`apps/desktop/`)

The C++ desktop client is organized into modular directories adhering to modern C++ conventions:

```text
apps/desktop/
├── CMakeLists.txt                  # Root CMake configuration file
├── CMakePresets.json               # Cross-platform CMake presets (VS2022, Ninja, Clang)
├── vcpkg.json                      # Dependency manifest (Qt6, nlohmann-json, gtest)
├── vcpkg-configuration.json        # vcpkg registry configuration
│
├── include/                        # Public C++ Header Files
│   ├── core/
│   │   ├── AppConfig.hpp           # Runtime settings, base URLs, flags
│   │   ├── Constants.hpp           # Design constants, timeouts, storage keys
│   │   └── Logger.hpp              # Structured logging utility
│   ├── models/
│   │   ├── ApiContracts.hpp        # C++20 DTOs matching @dev-dialogue/api-contracts
│   │   ├── UserSession.hpp         # Current logged-in user state
│   │   ├── InterviewTranscript.hpp # Chat message data model
│   │   └── QuestionBankItem.hpp    # Admin question entity
│   ├── network/
│   │   ├── ApiClient.hpp           # Async HTTP/REST client using QNetworkAccessManager
│   │   └── NetworkEndpoints.hpp    # Centralized REST route definitions
│   ├── services/
│   │   ├── AuthService.hpp         # Authentication, token lifecycle & profile
│   │   ├── InterviewService.hpp    # Interview CRUD, messaging, and evaluation
│   │   ├── AdminService.hpp        # Question bank administration
│   │   ├── MockAiEngine.hpp        # Native C++ offline AI simulation engine
│   │   └── SecureStorage.hpp       # OS Credential Vault (DPAPI / Keychain)
│   └── viewmodels/
│       ├── AuthViewModel.hpp       # Login/Register state & validation for QML
│       ├── DashboardViewModel.hpp  # User KPI statistics and past sessions
│       ├── SetupViewModel.hpp      # Interview config (direction, level, language)
│       ├── ChatViewModel.hpp       # Real-time interview chat controller & timers
│       ├── ReportViewModel.hpp     # Performance report data presentation
│       ├── AdminViewModel.hpp      # Admin question bank table & filters
│       └── NavigationViewModel.hpp # Active view routing and view-mode toggle
│
├── src/                            # C++ Implementation Files
│   ├── main.cpp                    # Application entry point & QML engine setup
│   ├── core/
│   │   └── AppConfig.cpp
│   ├── network/
│   │   └── ApiClient.cpp
│   ├── services/
│   │   ├── AuthService.cpp
│   │   ├── InterviewService.cpp
│   │   ├── AdminService.cpp
│   │   ├── MockAiEngine.cpp
│   │   └── SecureStorage.cpp
│   └── viewmodels/
│       ├── AuthViewModel.cpp
│       ├── DashboardViewModel.cpp
│       ├── SetupViewModel.cpp
│       ├── ChatViewModel.cpp
│       ├── ReportViewModel.cpp
│       ├── AdminViewModel.cpp
│       └── NavigationViewModel.cpp
│
├── qml/                            # QML User Interface Modules
│   ├── Main.qml                    # Root window shell & navigation orchestrator
│   ├── styles/
│   │   ├── Theme.qml               # Design tokens, palette, radii, borders
│   │   ├── Typography.qml          # Font metrics, font sizes, weights
│   │   └── Icons.qml               # SVG vector icon references
│   ├── components/
│   │   ├── TactileButton.qml       # Custom button with spring press states
│   │   ├── DarkCard.qml            # Card container with 1px border & glass effect
│   │   ├── ScoreGauge.qml          # Circular/bar score indicator
│   │   ├── MessageBubble.qml       # Chat message bubble (AI vs Candidate)
│   │   ├── KpiTile.qml             # Dashboard statistics card
│   │   ├── StatusBanner.qml        # Backend health warning banner
│   │   └── Navbar.qml              # Top app bar with mode switcher & profile
│   └── views/
│       ├── LandingView.qml         # Hero section & interactive role showcase
│       ├── LoginView.qml           # Authentication & sign-in modal/view
│       ├── DashboardView.qml       # Candidate dashboard & past sessions
│       ├── SetupView.qml           # Track, level, and language selector
│       ├── ChatRoomView.qml        # Core interview dialogue room
│       ├── ReportView.qml          # Performance scorecard & review
│       └── AdminView.qml           # Question bank management panel
│
├── resources/                      # Embedded Static Assets
│   ├── fonts/
│   │   ├── GeistVF.woff2           # Primary Sans font
│   │   └── GeistMonoVF.woff2       # Monospace font for badges & timestamps
│   ├── icons/                      # Lucide/Feather SVG icons
│   │   ├── bot.svg
│   │   ├── user.svg
│   │   ├── sparkler.svg
│   │   ├── shield.svg
│   │   └── ...
│   └── i18n/
│       ├── devdialogue_en.ts       # English string catalog
│       └── devdialogue_uk.ts       # Ukrainian string catalog
│
└── tests/                          # Automated Tests
    ├── unit/
    │   ├── TestApiContracts.cpp    # DTO JSON roundtrip serialization tests
    │   └── TestMockAiEngine.cpp    # Offline engine question generation tests
    └── integration/
        └── TestApiClient.cpp       # Network client tests with mock HTTP server
```

---

## 2. Dependency Management: `vcpkg.json`

Using `vcpkg` in manifest mode guarantees deterministic, hermetic builds across development environments:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "name": "dev-dialogue-desktop",
  "version-string": "1.0.0",
  "description": "Dev Dialogue C++ native desktop client",
  "homepage": "https://github.com/vntu-dev-dialogue/dev-dialogue",
  "dependencies": [
    {
      "name": "qtbase",
      "features": ["network", "gui", "widgets", "concurrent", "ssl"]
    },
    {
      "name": "qtdeclarative"
    },
    {
      "name": "qtquickcontrols2"
    },
    {
      "name": "qtsvg"
    },
    {
      "name": "nlohmann-json",
      "version>=": "3.11.3"
    },
    {
      "name": "gtest",
      "version>=": "1.14.0"
    }
  ]
}
```

---

## 3. Production `CMakeLists.txt`

The build system utilizes **CMake 3.25+** with the official Qt 6 QML module integration (`qt_add_qml_module`) to enable ahead-of-time (AOT) QML compilation and resource bundling:

```cmake
cmake_minimum_required(VERSION 3.25)

project(DevDialogueDesktop
    VERSION 1.0.0
    DESCRIPTION "Dev Dialogue - Technical Interview Simulator C++ Desktop App"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Compiler Warnings & Hardening
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

# Locate Required Qt6 Packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Svg
    Concurrent
)

# Locate Third-Party Dependencies
find_package(nlohmann_json 3.11.3 REQUIRED)

# Define Core Application Executable
add_executable(DevDialogue
    src/main.cpp
    
    # Core Infrastructure
    src/core/AppConfig.cpp
    include/core/AppConfig.hpp
    include/core/Constants.hpp
    include/core/Logger.hpp
    
    # Models & Contracts
    include/models/ApiContracts.hpp
    include/models/UserSession.hpp
    include/models/InterviewTranscript.hpp
    
    # Network Layer
    src/network/ApiClient.cpp
    include/network/ApiClient.hpp
    include/network/NetworkEndpoints.hpp
    
    # Services & Engine
    src/services/AuthService.cpp
    include/services/AuthService.hpp
    src/services/InterviewService.cpp
    include/services/InterviewService.hpp
    src/services/AdminService.cpp
    include/services/AdminService.hpp
    src/services/MockAiEngine.cpp
    include/services/MockAiEngine.hpp
    src/services/SecureStorage.cpp
    include/services/SecureStorage.hpp
    
    # ViewModels (MVVM)
    src/viewmodels/AuthViewModel.cpp
    include/viewmodels/AuthViewModel.hpp
    src/viewmodels/DashboardViewModel.cpp
    include/viewmodels/DashboardViewModel.hpp
    src/viewmodels/SetupViewModel.cpp
    include/viewmodels/SetupViewModel.hpp
    src/viewmodels/ChatViewModel.cpp
    include/viewmodels/ChatViewModel.hpp
    src/viewmodels/ReportViewModel.cpp
    include/viewmodels/ReportViewModel.hpp
    src/viewmodels/AdminViewModel.cpp
    include/viewmodels/AdminViewModel.hpp
    src/viewmodels/NavigationViewModel.cpp
    include/viewmodels/NavigationViewModel.hpp
)

target_include_directories(DevDialogue PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(DevDialogue PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Svg
    Qt6::Concurrent
    nlohmann_json::nlohmann_json
)

# Windows Specific Libraries (DPAPI / Credential Manager)
if(WIN32)
    target_link_libraries(DevDialogue PRIVATE Crypt32 Advapi32)
endif()

# Register QML Module with Ahead-of-Time Compilation
qt_add_qml_module(DevDialogue
    URI "DevDialogue.App"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/styles/Theme.qml
        qml/styles/Typography.qml
        qml/styles/Icons.qml
        qml/components/TactileButton.qml
        qml/components/DarkCard.qml
        qml/components/ScoreGauge.qml
        qml/components/MessageBubble.qml
        qml/components/KpiTile.qml
        qml/components/StatusBanner.qml
        qml/components/Navbar.qml
        qml/views/LandingView.qml
        qml/views/LoginView.qml
        qml/views/DashboardView.qml
        qml/views/SetupView.qml
        qml/views/ChatRoomView.qml
        qml/views/ReportView.qml
        qml/views/AdminView.qml
    RESOURCES
        resources/fonts/GeistVF.woff2
        resources/fonts/GeistMonoVF.woff2
        resources/icons/bot.svg
        resources/icons/user.svg
        resources/icons/sparkler.svg
        resources/icons/shield.svg
)

# Deployment target for Windows (bundles Qt runtime DLLs)
if(WIN32)
    add_custom_command(TARGET DevDialogue POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml
            $<TARGET_FILE:DevDialogue>
        COMMENT "Deploying Qt runtime libraries with windeployqt..."
    )
endif()
```

---

## 4. Application Entry Point (`src/main.cpp`)

The C++ entry point configures high-DPI scaling, registers fonts, instantiates ViewModels, and launches the QML engine:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFontDatabase>
#include <QIcon>
#include <QDir>
#include <memory>

#include "core/AppConfig.hpp"
#include "services/SecureStorage.hpp"
#include "services/AuthService.hpp"
#include "services/InterviewService.hpp"
#include "services/AdminService.hpp"
#include "network/ApiClient.hpp"

#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/AuthViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/AdminViewModel.hpp"

int main(int argc, char *argv[]) {
    // 1. Configure High-DPI Scaling & Render Engine
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );
    
    QGuiApplication app(argc, argv);
    app.setOrganizationName("VNTU");
    app.setOrganizationDomain("vntu.edu.ua");
    app.setApplicationName("DevDialogue");
    app.setApplicationVersion("1.0.0");

    // 2. Load Embedded Design Fonts (Geist Sans & Mono)
    QFontDatabase::addApplicationFont(":/resources/fonts/GeistVF.woff2");
    QFontDatabase::addApplicationFont(":/resources/fonts/GeistMonoVF.woff2");
    app.setFont(QFont("Geist", 10));

    // 3. Set Base Quick Controls Style
    QQuickStyle::setStyle("Basic");

    // 4. Initialize Core Singletons & Services
    auto appConfig = std::make_shared<devdialogue::core::AppConfig>();
    auto secureStorage = std::make_shared<devdialogue::services::SecureStorage>();
    auto apiClient = std::make_shared<devdialogue::network::ApiClient>(appConfig, secureStorage);

    auto authService = std::make_shared<devdialogue::services::AuthService>(apiClient, secureStorage);
    auto interviewService = std::make_shared<devdialogue::services::InterviewService>(apiClient);
    auto adminService = std::make_shared<devdialogue::services::AdminService>(apiClient);

    // 5. Instantiate ViewModels
    auto navVM = std::make_unique<devdialogue::viewmodels::NavigationViewModel>();
    auto authVM = std::make_unique<devdialogue::viewmodels::AuthViewModel>(authService);
    auto dashboardVM = std::make_unique<devdialogue::viewmodels::DashboardViewModel>(interviewService);
    auto setupVM = std::make_unique<devdialogue::viewmodels::SetupViewModel>(interviewService, navVM.get());
    auto chatVM = std::make_unique<devdialogue::viewmodels::ChatViewModel>(interviewService, navVM.get());
    auto reportVM = std::make_unique<devdialogue::viewmodels::ReportViewModel>(interviewService);
    auto adminVM = std::make_unique<devdialogue::viewmodels::AdminViewModel>(adminService, authService);

    // 6. Bootstrap QML Engine & Register Context Properties
    QQmlApplicationEngine engine;
    QQmlContext* rootContext = engine.rootContext();

    rootContext->setContextProperty("navVM", navVM.get());
    rootContext->setContextProperty("authVM", authVM.get());
    rootContext->setContextProperty("dashboardVM", dashboardVM.get());
    rootContext->setContextProperty("setupVM", setupVM.get());
    rootContext->setContextProperty("chatVM", chatVM.get());
    rootContext->setContextProperty("reportVM", reportVM.get());
    rootContext->setContextProperty("adminVM", adminVM.get());
    rootContext->setContextProperty("apiClient", apiClient.get());

    const QUrl url(QStringLiteral("qrc:/DevDialogue/App/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    // Check saved session on launch
    authService->restoreSession();

    return app.exec();
}
```

---

## 5. Root Window Shell (`qml/Main.qml`)

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DevDialogue.App

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1200
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    title: qsTr("Dev Dialogue — AI Interview Simulator")
    color: Theme.backgroundPrimary

    // Top Header / Navbar
    Navbar {
        id: navbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        z: 10
    }

    // Backend Connectivity Warning Banner (Equivalent of BackendStatusBanner.tsx)
    StatusBanner {
        id: statusBanner
        anchors.top: navbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !apiClient.isOnline
        z: 9
    }

    // Dynamic View Host (Stacked Layout routing between screens)
    StackLayout {
        id: viewStack
        anchors.top: statusBanner.visible ? statusBanner.bottom : navbar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: navVM.currentViewIndex

        LandingView {}     // Index 0: Hero / Feature Overview
        DashboardView {}   // Index 1: Candidate Dashboard
        SetupView {}       // Index 2: Interview Config
        ChatRoomView {}    // Index 3: Active Interview Chat
        ReportView {}      // Index 4: Scorecard & Report
        AdminView {}       // Index 5: Question Bank Management
    }

    // Global Login / Auth Modal Overlay
    LoginView {
        id: loginModal
        anchors.fill: parent
        visible: authVM.isLoginModalOpen
        z: 100
    }
}
```

---

## 6. Build & Execution Workflow

### Step 1: Install Dependencies via vcpkg
```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat    # Windows
./vcpkg/bootstrap-vcpkg.sh     # Linux / macOS
```

### Step 2: Configure & Generate Build Files
```bash
cmake -B apps/desktop/build -S apps/desktop \
  -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

### Step 3: Compile and Run
```bash
# Build
cmake --build apps/desktop/build --config Release --parallel

# Execute
./apps/desktop/build/Release/DevDialogue.exe   # Windows
./apps/desktop/build/DevDialogue               # Linux / macOS
```

