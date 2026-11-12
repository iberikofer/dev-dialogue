cmake_minimum_required(VERSION 3.25)

project(DevDialogueDesktop
    VERSION 1.0.0
    DESCRIPTION "Dev Dialogue - Technical Interview Simulator C++ Desktop App"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# The QML files keep their `qrc:/DevDialogue/App/...` resource prefix, which
# the icons and the font loader rely on.
if(POLICY QTP0004)
    cmake_policy(SET QTP0004 OLD)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Svg
    Concurrent
    LinguistTools
)

find_package(nlohmann_json 3.11.3 REQUIRED)

option(DEVDIALOGUE_BUILD_TESTS "Build the desktop test suite" ON)

# ---------------------------------------------------------------------------
# DevDialogueCore: plain C++ contracts and the offline engine.
# Kept free of Qt so the unit tests can link it without a QCoreApplication.
# ---------------------------------------------------------------------------
add_library(DevDialogueCore STATIC
    # Models & Contracts
    include/models/ApiContracts.hpp

    # Shared QML/C++ helpers
    include/core/EnumUtils.hpp

    # Offline engine
    include/services/MockAiEngine.hpp
    src/services/MockAiEngine.cpp
)

target_include_directories(DevDialogueCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(DevDialogueCore PUBLIC
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------------------------
# DevDialogueApp: the Qt application layer (services, models, view models).
# ---------------------------------------------------------------------------
add_library(DevDialogueApp STATIC
    # Core Infrastructure
    src/core/AppConfig.cpp
    include/core/AppConfig.hpp
    src/core/I18nManager.cpp
    include/core/I18nManager.hpp
    include/core/Constants.hpp

    # Models
    include/models/MessageListModel.hpp
    src/models/MessageListModel.cpp
    include/models/InterviewListModel.hpp
    src/models/InterviewListModel.cpp
    include/models/QuestionListModel.hpp
    src/models/QuestionListModel.cpp

    # Network Layer
    include/network/ApiClient.hpp
    src/network/ApiClient.cpp

    # Services
    include/services/SecureStorage.hpp
    src/services/SecureStorage.cpp
    include/services/AuthService.hpp
    src/services/AuthService.cpp
    include/services/InterviewService.hpp
    src/services/InterviewService.cpp
    include/services/AdminService.hpp
    src/services/AdminService.cpp

    # ViewModels
    src/viewmodels/NavigationViewModel.cpp
    include/viewmodels/NavigationViewModel.hpp
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
)

target_include_directories(DevDialogueApp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(DevDialogueApp PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Svg
    Qt6::Concurrent
    DevDialogueCore
)

if(WIN32)
    target_link_libraries(DevDialogueApp PUBLIC Crypt32 Advapi32)
endif()

add_executable(DevDialogue
    src/main.cpp
)

target_link_libraries(DevDialogue PRIVATE
    DevDialogueApp
)

# Theme and design-token files are QML singletons; register them so their
# properties are available to every component in the module.
set(QML_SINGLETONS
    qml/styles/Theme.qml
    qml/styles/Typography.qml
    qml/styles/Icons.qml
    qml/styles/Labels.qml
)
set_source_files_properties(${QML_SINGLETONS}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

# The full list of QML modules, kept in a variable so the test suite can
# generate a disk based `qmldir` for the QML test runner.
set(QML_MODULE_FILES
    qml/Main.qml
    qml/styles/Theme.qml
    qml/styles/Typography.qml
    qml/styles/Icons.qml
    qml/styles/Labels.qml
    qml/components/TactileButton.qml
    qml/components/DarkCard.qml
    qml/components/Badge.qml
    qml/components/MessageBubble.qml
    qml/components/ScoreGauge.qml
    qml/components/ProgressRing.qml
    qml/components/KpiTile.qml
    qml/components/StatusBanner.qml
    qml/components/SegmentedControl.qml
    qml/components/AppModal.qml
    qml/components/EmptyState.qml
    qml/components/ErrorBanner.qml
    qml/components/Spinner.qml
    qml/components/DarkTextField.qml
    qml/components/DarkTextArea.qml
    qml/components/PillSwitch.qml
    qml/components/Navbar.qml
    qml/views/LandingView.qml
    qml/views/DashboardView.qml
    qml/views/SetupView.qml
    qml/views/ChatRoomView.qml
    qml/views/ReportView.qml
    qml/views/AdminView.qml
)

# Dev-build convenience: point Qt at its installation prefix so QML modules,
# platform plugins, and translations are found regardless of working directory.
if(WIN32)
    get_filename_component(_qt6_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    file(WRITE "${CMAKE_BINARY_DIR}/qt.conf"
        "[Paths]\n"
        "Prefix=${_qt6_prefix}\n"
        "Plugins=${_qt6_prefix}/share/qt6/plugins\n"
        "Qml2Imports=${_qt6_prefix}/share/qt6/qml\n"
        "QmlImports=${_qt6_prefix}/share/qt6/qml\n"
    )
endif()

# The QML layer is compiled into the executable. The test suite generates a
# disk based `qmldir` so the QML tests can import the very same sources.
qt_add_qml_module(DevDialogue
    URI "DevDialogue.App"
    VERSION 1.0
    QML_FILES
        ${QML_MODULE_FILES}
    RESOURCES
        resources/fonts/Geist-VF.ttf
        resources/fonts/GeistMono-VF.ttf
        resources/icons/frontend.svg
        resources/icons/backend.svg
        resources/icons/devops.svg
        resources/icons/qa.svg
        resources/icons/pm.svg
        resources/icons/bot.svg
        resources/icons/user.svg
        resources/icons/sparkles.svg
        resources/icons/shield.svg
        resources/icons/warning.svg
        resources/icons/refresh.svg
        resources/icons/dashboard.svg
        resources/icons/chart.svg
        resources/icons/plus.svg
        resources/icons/logout.svg
)

target_link_libraries(DevDialogue PRIVATE
    DevDialogueApp
)

# Compile the Qt Linguist catalogs and embed them as :/i18n/devdialogue_<code>.qm.
qt_add_translations(DevDialogue
    TS_FILES
        translations/devdialogue_uk.ts
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS -no-obsolete
)

if(WIN32)
    add_custom_command(TARGET DevDialogue POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml
            --compiler-runtime
            $<TARGET_FILE:DevDialogue>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt6_prefix}/bin/libstdc++-6.dll"
            "${_qt6_prefix}/bin/libgcc_s_seh-1.dll"
            "${_qt6_prefix}/bin/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:DevDialogue>"
        COMMENT "Deploying Qt runtime libraries with windeployqt..."
    )
endif()

if(DEVDIALOGUE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

