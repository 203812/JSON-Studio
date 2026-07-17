cmake_minimum_required(VERSION 3.21)
project(JsonStudio VERSION 0.2.0 LANGUAGES CXX)

# The .rc carries the executable's Windows icon (Explorer, taskbar, shortcut).
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()
enable_testing()

find_package(yaml-cpp REQUIRED)

find_package(nlohmann_json 3.10 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found locally - fetching")
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

qt_add_executable(JsonStudio
    src/main.cpp
    src/ui/Theme.cpp
    src/ui/MainWindow.cpp
    src/ui/CommandPalette.cpp
    src/editor/CodeEditor.cpp
    src/editor/JsonHighlighter.cpp
    src/editor/YamlHighlighter.cpp
    src/editor/Minimap.cpp
    src/editor/EditorPane.cpp
    src/editor/Syntax.cpp
    src/core/JsonTools.cpp
    src/core/YamlTools.cpp
    src/core/Version.cpp
    src/core/Updater.cpp
)

# In-app window icon (title bar, Alt-Tab): embedded so it needs no external file.
# BASE strips the dir so the resource path is :/app/app-icon.png.
qt_add_resources(JsonStudio "appicon"
    PREFIX "/app"
    BASE assets/logo
    FILES assets/logo/app-icon.png)

# Executable icon for Explorer/taskbar/shortcut. windres resolves icon.ico
# relative to the .rc file's own directory.
if(WIN32)
    target_sources(JsonStudio PRIVATE assets/logo/app.rc)
endif()

target_include_directories(JsonStudio PRIVATE src)
target_link_libraries(JsonStudio PRIVATE Qt6::Widgets Qt6::Network nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)
target_compile_definitions(JsonStudio PRIVATE JSONSTUDIO_VERSION="${PROJECT_VERSION}")

# Console window in Debug (qDebug output), windowed app otherwise.
set_target_properties(JsonStudio PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

# Tests cover core/ only - no Widgets, no display needed.
add_executable(CoreTests
    tests/test_jsontools.cpp
    tests/test_yamltools.cpp
    tests/test_version.cpp
    tests/main.cpp
    src/core/JsonTools.cpp
    src/core/YamlTools.cpp
    src/core/Version.cpp
)
target_include_directories(CoreTests PRIVATE src tests)
target_link_libraries(CoreTests PRIVATE Qt6::Core nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)
target_compile_definitions(CoreTests PRIVATE JSONSTUDIO_VERSION="${PROJECT_VERSION}")
add_test(NAME Core COMMAND CoreTests)
