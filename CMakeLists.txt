# =========================================================================
# Dante CLI — C++20 / Qt 6 / CMake build configuration
# =========================================================================
# Target: Windows 10/11 (MSVC), with macOS dev build supported.
# Stack mandated by `.claude/skills/low-latency-trading-windows`:
#   - C++20+, MSVC, CMake + Ninja
#   - Qt 6 Community (LGPL — dynamic linking only)
#   - SQLite for local persistence
#   - spdlog for structured logging
#   - simdjson / nlohmann::json for JSON
#   - WiX or Inno Setup for installer (we use Inno here)
# =========================================================================

cmake_minimum_required(VERSION 3.21)
project(dante-cli
    VERSION 0.7.0
    DESCRIPTION "Dante CLI — Terminal premium com IA, sessões e split workspace"
    HOMEPAGE_URL "https://dantetesta.com.br/dante-cli"
    LANGUAGES CXX
)

# ---- Language standard ----------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Qt --------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt modules used. Quick = QML runtime; Network = HTTP; Sql = SQLite;
# Concurrent = threaded persistence; Widgets only for menu bar/tray.
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Network Sql Concurrent Widgets
)

qt_standard_project_setup(REQUIRES 6.5)

# ---- Compiler flags --------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus /EHsc)
    add_compile_definitions(_WIN32_WINNT=0x0A00 NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- Subdirectories --------------------------------------------------------
add_subdirectory(core)
add_subdirectory(infra)
add_subdirectory(apps/desktop-qt)

# ---- Tests -----------------------------------------------------------------
# enable_testing()
# add_subdirectory(tests)   # enable when first test arrives
