# OpenXgpro

Native Linux port of **Xgpro** (the software for TL866II Plus / T48 / T56 USB
programmers, Minipro "Xgecu"). The original program is a 32-bit Windows PE32
(MFC application) located in `../XgproV1316/`. This repository is a clean
rewrite: the Windows binary is reverse engineered for reference, and the code
here is written from scratch as a Qt6 application.

> Status: very early exploration. Nothing usable yet.

## Layout

- `tools/` — Python utilities for extracting/analysing the reference PE
  (resource dumpers, algorithm file inspectors, etc.)
- `docs/reverse-engineering/` — notes produced while analysing the binary
  (UI inventory, protocol findings, data formats)
- `src/` — the Qt6 application
- `external/reference/` — extracted reference artifacts (dialogs, bitmaps,
  strings) used to guide the reimplementation

## The reference binary

`../XgproV1316/Xgpro.exe`

- PE32, i386, MSVC/MFC statically linked, no PDB
- ~50 dialog resources, 2 menus, toolbar bitmaps
- Default UI language resource: 2052 (Simplified Chinese); runtime language is
  switched through `Language*.INI` files in the install directory
- Talks to the programmer over USB using WINUSB (see `drv/` for the WinUSB .inf
  driver)
- Chip programming "algorithms" are stored as plugin files in `algorithm/*.alg`

## Building

Requires Qt 6 (Widgets), CMake >= 3.16 and a C++23 compiler.

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

## Strategy

1. Reconstruct the interface from the embedded dialog/menu resources (this is
   the current milestone). The layout of every dialog can be dumped with
   `tools/dump_resources.py`.
2. Reproduce dialogs as native Qt widgets.
3. Reverse the USB transport / WINUSB protocol to drive real hardware.
4. Implement the `.alg` algorithm plugin loader so chip support can be reused
   from the original distribution.
