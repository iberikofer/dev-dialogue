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
)

find_package(nlohmann_json 3.11.3 REQUIRED)

# Qt6Keychain: mandatory in Release (without QUIET), warning in Debug
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    find_package(Qt6Keychain REQUIRED)
else()
    find_package(Qt6Keychain QUIET)
    if(NOT Qt6Keychain_FOUND)
        message(WARNING "Qt6Keychain not found in Debug mode. Falling back to in-memory secure storage.")
    endif()
endif()

add_executable(DevDialogue
    src/main.cpp
    
    # Core Infrastructure
    src/core/AppConfig.cpp
    include/core/AppConfig.hpp
    include/core/Constants.hpp
    
    # Models & Contracts
    include/models/ApiContracts.hpp

    # Network
    src/network/ApiClient.cpp
    include/network/ApiClient.hpp
    include/network/NetworkEndpoints.hpp

    # Services & Secure Storage
    include/services/ISecureStorage.hpp
    src/services/InMemorySecureStorage.cpp
    include/services/InMemorySecureStorage.hpp
    src/services/QtKeychainSecureStorage.cpp
    include/services/QtKeychainSecureStorage.hpp
    src/services/AuthService.cpp
    include/services/AuthService.hpp
    src/services/InterviewService.cpp
    include/services/InterviewService.hpp
    src/services/AdminService.cpp
    include/services/AdminService.hpp
    
    # ViewModels
    src/viewmodels/AuthViewModel.cpp
    include/viewmodels/AuthViewModel.hpp
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

if(TARGET qt6keychain)
    target_link_libraries(DevDialogue PRIVATE qt6keychain)
    target_compile_definitions(DevDialogue PRIVATE HAS_QTKEYCHAIN=1)
elseif(TARGET Qt6Keychain::Qt6Keychain)
    target_link_libraries(DevDialogue PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(DevDialogue PRIVATE HAS_QTKEYCHAIN=1)
endif()

if(WIN32)
    target_link_libraries(DevDialogue PRIVATE Crypt32 Advapi32)
endif()

qt_add_qml_module(DevDialogue
    URI "DevDialogue.App"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/styles/Theme.qml
        qml/components/Navbar.qml
        qml/views/LandingView.qml
)

# ==========================================
# Testing (GoogleTest)
# ==========================================
find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()

    add_executable(DevDialogueTests
        tests/main.cpp
        tests/unit/TestApiContracts.cpp
        tests/unit/TestSecureStorage.cpp
        tests/unit/TestApiClient.cpp

        # Headers for AUTOMOC
        include/core/AppConfig.hpp
        include/core/Constants.hpp
        include/models/ApiContracts.hpp
        include/network/ApiClient.hpp
        include/network/NetworkEndpoints.hpp
        include/services/ISecureStorage.hpp
        include/services/InMemorySecureStorage.hpp
        include/services/QtKeychainSecureStorage.hpp
        include/services/AuthService.hpp
        include/services/InterviewService.hpp
        include/services/AdminService.hpp

        # Sources required for tests
        src/core/AppConfig.cpp
        src/network/ApiClient.cpp
        src/services/InMemorySecureStorage.cpp
        src/services/QtKeychainSecureStorage.cpp
        src/services/AuthService.cpp
        src/services/InterviewService.cpp
        src/services/AdminService.cpp
    )

    target_include_directories(DevDialogueTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(DevDialogueTests PRIVATE
        GTest::gtest
        Qt6::Core
        Qt6::Network
        nlohmann_json::nlohmann_json
    )

    if(TARGET qt6keychain)
        target_link_libraries(DevDialogueTests PRIVATE qt6keychain)
        target_compile_definitions(DevDialogueTests PRIVATE HAS_QTKEYCHAIN=1)
    elseif(TARGET Qt6Keychain::Qt6Keychain)
        target_link_libraries(DevDialogueTests PRIVATE Qt6Keychain::Qt6Keychain)
        target_compile_definitions(DevDialogueTests PRIVATE HAS_QTKEYCHAIN=1)
    endif()

    if(WIN32)
        target_link_libraries(DevDialogueTests PRIVATE Crypt32 Advapi32)
    endif()

    include(GoogleTest)
    gtest_discover_tests(DevDialogueTests
        DISCOVERY_MODE PRE_TEST
        PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin;$ENV{PATH}"
    )
endif()

find_program(WINDEPLOYQT_EXECUTABLE windeployqt
    HINTS "${Qt6_DIR}/../../../tools/Qt6/bin" "${Qt6_DIR}/../../../bin"
)

if(WIN32 AND WINDEPLOYQT_EXECUTABLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET DevDialogue POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml
            $<TARGET_FILE:DevDialogue>
        COMMENT "Deploying Qt runtime libraries with windeployqt..."
    )
endif()
