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

add_executable(DevDialogue
    src/main.cpp
    
    # Core Infrastructure
    src/core/AppConfig.cpp
    include/core/AppConfig.hpp
    include/core/Constants.hpp
    
    # Models & Contracts
    include/models/ApiContracts.hpp
    
    # ViewModels
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

if(WIN32)
    add_custom_command(TARGET DevDialogue POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml
            $<TARGET_FILE:DevDialogue>
        COMMENT "Deploying Qt runtime libraries with windeployqt..."
    )
endif()

