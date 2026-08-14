# OpenXgpro

Open-source port of **Xgpro** (the software for TL866II Plus / T48 / T56 USB
programmers, Minipro "Xgecu"). The original program is a 32-bit Windows PE32
(MFC application). This repository is a clean
rewrite: the Windows binary is reverse engineered for reference, and the code
is written from scratch as a Qt6 application.

> Status: very early exploration. Nothing usable yet.

## Layout

- `tools/` — Python utilities for extracting/analysing the reference PE
  (resource dumpers, algorithm file inspectors, etc.)
- `docs/` — analysis notes recovered from the reference binary
  (`00-reference-binary.md`): UI inventory, protocol findings, data formats,
  reference-data loading, and the project strategy (`05-strategy.md`)
- `src/` — the Qt6 application
- `udev/` — Linux udev rules so the programmer is usable without root
- `external/reference/` — extracted reference artifacts (dialogs, bitmaps,
  strings) used to guide the reimplementation

## Building

Requires Qt 6 (Widgets), CMake >= 3.16, a C++23 compiler and `libusb-1.0`.

```sh
cmake -S . -B build
cmake --build build
./build/OpenXgpro
```

### Linux

```sh
# Arch
sudo pacman -S cmake make gcc pkgconf qt6-base libusb
# Debian / Ubuntu
sudo apt install cmake make g++ pkg-config qt6-base-dev libusb-1.0-0-dev
# Fedora
sudo dnf install cmake make gcc-c++ pkgconf qt6-qtbase-devel libusbx-devel
```

### macOS

```sh
brew install cmake make qt libusb pkg-config
# Qt is keg-only; point CMake at it:
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
```
### Windows (MSYS2)

Build in a **MINGW64** (or UCRT64) MSYS2 shell:

```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make \
          mingw-w64-x86_64-pkgconf mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-libusb
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## USB access

### Linux

The programmer (VID `0xA466` PID `0x0A53`) has no kernel driver, so raw USB
access is root-only by default. Install the included udev rule to allow
unprivileged users (no `sudo` needed to read/erase/program):

```sh
sudo cp udev/99-openxgpro.rules /etc/udev/rules.d/
sudo udevadm control --reload
sudo udevadm trigger
```

then unplug/replug the programmer. Your user must be in the `plugdev` group
(`sudo usermod -aG plugdev $USER`, then log out/in).

### Windows

To talk to real hardware the programmer's USB driver must be WinUSB/libusbK
(e.g. via [Zadig](https://zadig.akeo.ie/)) — that is a runtime requirement,
not a build one.

## Reference data

See [docs](docs/) for how the app
finds the reference Xgpro distribution and loads the device database at
runtime.

## License

OpenXgpro is free software, released under the GNU General Public License. It
is free software; you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation version 3.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.

The full license text is available in the [LICENSE](LICENSE) file.
