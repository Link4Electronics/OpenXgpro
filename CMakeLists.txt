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

# Release packaging metadata. Single source of truth for the executable name,
# .desktop entry, AppStream metainfo and AppImage labels. Override any of
# these with -D... at configure time. Consumed by packaging/make-appimage.sh
# via the generated build/packaging/appimage_env.sh.
set(OXGPRO_EXECUTABLE_NAME     "OpenXgpro"   CACHE STRING "Name of the built executable")
set(OXGPRO_PRODUCT_NAME        "OpenXgpro"   CACHE STRING "Display name (.desktop Name, AppImage label)")
set(OXGPRO_PRODUCT_DESCRIPTION "Open-source port of Xgpro (TL866II Plus / T48 / T56 USB programmers)"
                                                              CACHE STRING "One-line description (.desktop Comment, AppStream summary)")
set(OXGPRO_DESKTOP_ID          "openxgpro"   CACHE STRING "Desktop file basename and icon ID")
set(OXGPRO_BUNDLE_IDENTIFIER   "org.openxgpro.OpenXgpro"
                                                              CACHE STRING "Reverse-DNS app identifier (AppStream component ID)")

# Plain-text version for the release pipeline (make-appimage.sh reads this to
# name the artifact).
file(WRITE "${CMAKE_BINARY_DIR}/VERSION.txt" "${APP_VERSION}\n")

# Configure the packaging templates into ${CMAKE_BINARY_DIR}/packaging/ where
# make-appimage.sh picks them up. The metainfo filename must be the component
# ID; make-appimage.sh installs it to usr/share/metainfo/ inside the AppDir.
configure_file(packaging/openxgpro.desktop.in
               ${CMAKE_BINARY_DIR}/packaging/${OXGPRO_DESKTOP_ID}.desktop @ONLY)
configure_file(packaging/openxgpro.metainfo.xml.in
               ${CMAKE_BINARY_DIR}/packaging/${OXGPRO_BUNDLE_IDENTIFIER}.metainfo.xml @ONLY)
configure_file(packaging/appimage_env.sh.in
               ${CMAKE_BINARY_DIR}/packaging/appimage_env.sh @ONLY)

# Offscreen smoke test for the buffer hex view (QPA offscreen, exit code only).
# Gated behind an option so release packaging (AppImage) doesn't need the test
# source. Enable with -DOXGPRO_BUILD_TESTS=ON.
option(OXGPRO_BUILD_TESTS "Build the hexview-smoke test" OFF)
if(OXGPRO_BUILD_TESTS)
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
endif()
