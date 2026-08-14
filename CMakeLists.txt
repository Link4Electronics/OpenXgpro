cmake_minimum_required(VERSION 3.16)
project(OpenXgpro VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Application identity shared by every target.
set(APP_NAME "OpenXgpro")
set(APP_VERSION "${PROJECT_VERSION}")

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

# Generate src/version.h into the build tree so every target can use the
# shared application name/version.
configure_file(src/version.h.in generated/version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

qt_add_executable(OpenXgpro
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/blockfilldialog.cpp
    src/blockfilldialog.h
    src/chipdialog.cpp
    src/chipdialog.h
    src/chips.cpp
    src/chips.h
    src/device.cpp
    src/device.h
    src/hexview.cpp
    src/hexview.h
    src/operations.cpp
    src/operations.h
    src/programmer.cpp
    src/programmer.h
    src/programmerdialog.cpp
    src/programmerdialog.h
    src/theme.cpp
    src/theme.h
)

target_link_libraries(OpenXgpro PRIVATE Qt6::Widgets PkgConfig::LIBUSB)

set_target_properties(OpenXgpro PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Offscreen smoke test for the buffer hex view (QPA offscreen, exit code only).
qt_add_executable(hexview-smoke
    tests/hexview_smoke.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/blockfilldialog.cpp
    src/blockfilldialog.h
    src/chipdialog.cpp
    src/chipdialog.h
    src/chips.cpp
    src/chips.h
    src/device.cpp
    src/device.h
    src/hexview.cpp
    src/hexview.h
    src/operations.cpp
    src/operations.h
    src/programmer.cpp
    src/programmer.h
    src/programmerdialog.cpp
    src/programmerdialog.h
    src/theme.cpp
    src/theme.h
)

target_link_libraries(hexview-smoke PRIVATE Qt6::Widgets PkgConfig::LIBUSB)
target_include_directories(hexview-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
