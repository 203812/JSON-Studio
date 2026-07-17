cmake_minimum_required(VERSION 3.21)
project(JsonStudio VERSION 0.2.1 LANGUAGES CXX)

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

include(FetchContent)

# Both dependencies fall back to FetchContent when not installed, so CI on any
# platform only needs Qt + a compiler - no system package juggling.
find_package(nlohmann_json 3.10 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found locally - fetching")
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    message(STATUS "yaml-cpp not found locally - fetching")
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
    # yaml-cpp 0.8.0 declares cmake_minimum_required < 3.5, which CMake 4.x
    # rejects. Tell CMake to apply 3.5-era policies to it.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0)
    FetchContent_MakeAvailable(yaml-cpp)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
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

# macOS: bundle the .icns so Finder and the Dock show the logo.
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE JsonStudio.icns)
    set(app_icns ${CMAKE_CURRENT_SOURCE_DIR}/assets/logo/JsonStudio.icns)
    set_source_files_properties(${app_icns} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(JsonStudio PRIVATE ${app_icns})
endif()

target_include_directories(JsonStudio PRIVATE src)
target_link_libraries(JsonStudio PRIVATE Qt6::Widgets Qt6::Network nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)
target_compile_definitions(JsonStudio PRIVATE JSONSTUDIO_VERSION="${PROJECT_VERSION}")

# Windowed app on Windows (no console) except in Debug; a real .app on macOS.
set_target_properties(JsonStudio PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "JSON Studio"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.203812.jsonstudio"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
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
