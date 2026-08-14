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

# Generate src/version.h into the build tree so every target can use the
# shared application name/version.
configure_file(src/version.h.in generated/version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

qt_add_executable(OpenXgpro
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/chipdialog.cpp
    src/chipdialog.h
    src/chips.cpp
    src/chips.h
    src/theme.cpp
    src/theme.h
)

target_link_libraries(OpenXgpro PRIVATE Qt6::Widgets)

set_target_properties(OpenXgpro PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
