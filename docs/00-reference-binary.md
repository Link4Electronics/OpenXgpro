# 00 — The reference binary

Everything in this directory is recovered from the reference distribution
`../XgproV1316/` (sibling of this repository), whose executable is
`XgproV1316/Xgpro.exe`.

- PE32, i386, MSVC/MFC statically linked, no PDB
- ~50 dialog resources, 2 menus, toolbar bitmaps
- Default UI language resource: 2052 (Simplified Chinese); runtime language is
  switched through `Language*.INI` files in the install directory
- Talks to the programmer over USB using WINUSB (see `drv/` for the WinUSB .inf
  driver)
- Chip programming "algorithms" are stored as plugin files in `algorithm/*.alg`

This is a clean rewrite; the binary is only used as a reference for behaviour.
See `05-strategy.md` for how it is analysed.

## Reference data at runtime

The app loads the real device database (chip families, `Logic.lst` parts) from
the reference Xgpro distribution when it can find it. It looks for
`XgproV1316` in the current directory, the parent directories, or your home
directory, and you can point it somewhere explicit with the
`OPENXGPRO_REFERENCE` environment variable. Without it, a small built-in
sample chip list is used.

Formats of the reference data files are inventoried in
`03-buffer-and-hexview.md` (Data files section) and `04-algorithm-format.md`
(`.alg` plugins).
