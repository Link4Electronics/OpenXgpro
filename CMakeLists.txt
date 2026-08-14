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

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets LinguistTools)

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
    src/algfile.cpp
    src/algfile.h
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
    src/logiclist.cpp
    src/logiclist.h
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

# Translations (Qt Linguist). The committed .ts files are compiled to .qm at
# build time (lrelease) and embedded into the executable as resources under
# ":/i18n". Run the "OpenXgpro_lupdate" target to refresh the .ts files after
# adding or changing tr() strings.
set(TS_FILES
    translations/OpenXgpro_pl.ts
    translations/OpenXgpro_ru.ts
    translations/OpenXgpro_cs.ts
    translations/OpenXgpro_es.ts
    translations/OpenXgpro_de.ts
    translations/OpenXgpro_pt_BR.ts
    translations/OpenXgpro_fr.ts
    translations/OpenXgpro_hu.ts
    translations/OpenXgpro_it.ts
    translations/OpenXgpro_tr.ts
)
qt_add_translations(OpenXgpro SOURCE_TARGETS OpenXgpro TS_FILES ${TS_FILES})

# Offscreen smoke test for the buffer hex view (QPA offscreen, exit code only).
qt_add_executable(hexview-smoke
    tests/hexview_smoke.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/algfile.cpp
    src/algfile.h
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
    src/logiclist.cpp
    src/logiclist.h
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
