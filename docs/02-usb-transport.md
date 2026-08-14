# USB Transport Layer (WinUSB)

Disassembled from `Xgpro.exe` (i386, MFC). Addresses are RVAs into the image
base `0x400000`. The transport is generic WinUSB; the same code drives
TL866II, T48 and T56, branching on a device-type byte. For a Linux port this
maps 1:1 onto `libusb` (bulk endpoints 0x01/0x02/0x03 OUT, 0x81/0x82/0x83 IN).

## Device identity

- Interface GUID: **`{E7E8BA13-2A81-446E-A11E-72398FBDA82F}`** (stored LE at
  `Xgpro.exe` rdata `0x684b94`; matches `drv/Xgprowinusb.inf`
  `[Dev_AddReg] DeviceInterfaceGUIDs`).
- VID/PID **VID_A466&PID_0A53** (`Xgprowinusb.inf` `[MyDevice_WinUSB.*]`).
  For Linux/libusb: `open(0xA466, 0x0A53)`, match on interface class/interface.
- Devices are found by interface GUID via SetupDi (the app never matches by
  name); model (TL866II/T48/T56) is detected by protocol, not by VID/PID.
- Right after the GUID in rdata a "4.2"-suffixed 16-byte blob; not a string.

## Enumeration & open (`0x4db8e0`, `0x4db730`)

1. `SetupDiGetClassDevsA(&GUID, NULL, NULL, 0x12)` — `0x12` = DIGCF_PRESENT
   (0x2) | DIGCF_DEVICEINTERFACE (0x10). Failure → "SetupDiGetClassDevs failed!\n".
2. Loop (max **4** devices, count in `0x7a2a80`):
   `SetupDiEnumDeviceInterfaces` then `SetupDiGetDeviceInterfaceDetailA`
   (0x400-byte buffer). Each device path (260 bytes) stored at
   `0x7a2a88 + i*0x104`. Failure → "Failed to allocate memory.\n".
3. For each device: `CreateFileA(path, 0xC0000000, share=3, OPEN_EXISTING,
   flags=0x40000080)` → handle saved at `0x73f170[i]` (or `-1`).
   `WinUsb_Initialize(handle, &iface)` → interface handle saved at
   `0x7a39e0 + i*4`.
4. On WinUsb_Initialize failure: `CloseHandle`, mark slot free.

## Pipe configuration (`WinUsb_SetPipePolicy`, `0x4db730`/`0x4dbd50`)

All interface-relative pipe IDs, timeout 5000 ms:

| Pipe | Policy | Value | Meaning |
|---|---|---|---|
| 0x01, 0x02, 0x03 | PIPE_TRANSFER_TIMEOUT (3) | 5000 | bulk OUT |
| 0x81, 0x82, 0x83 | PIPE_TRANSFER_TIMEOUT (3) | 5000 | bulk IN |
| 0x81, 0x82, 0x83 | AUTO_FLUSH (6) | 1 | flush each transfer |

So: OUT = 0x01/0x02/0x03, IN = 0x81/0x82/0x83. Endpoint 0x01/0x81 is the
command channel; 0x02/0x82 carries bulk pin data; 0x03/0x83 is auxiliary.

## Handshake / device type (`0x4dba90`)

```
WriteData(iface, 8-byte cmd, 8)        // pipe 0x01
buf = ReadData(iface, 0x40-byte buf)   // pipe 0x81, up to 64 bytes
if buf[0x0a] not in {5, 6, 7} -> fail  // device type
```
- Failure pops "Read device information error!".
- Device type byte (offset 0x0a of the response): **5 = TL866II, 6 = T56,
  7 = T48, 8 = T76** — confirmed by the firmware-update file selection
  (`0x4029b3`: type 6 → `updateT56.dat`, type 7 → `updateT48.dat`, else →
  `updateII.dat`; type 8 has its own `UpdateT76.Dat` path).
- The interface handle is also mapped to a device index 0..3 by comparing
  against `0x7a39e0..0x7a39ec` (`0x4dc300`).

## Low-level helpers

- **`WriteData` (`0x4dc380`)** `(iface, buf, len)`:
  `WinUsb_WritePipe(iface, 0x01, buf, len, &n, NULL)`.
- **`ReadData` (`0x4dc300`)** `(iface, buf, len)`:
  maps iface→index; if device-type byte `[index*0xec + 0x80109b] == 6` reads
  `len+1` else `len`; `WinUsb_ReadPipe(iface, 0x81, buf, len, &n, NULL)`.
- **`ReadData2` (`0x4dc000` region)**: same on pipe `0x82`.
- **`SendBlock` (`0x4dc080` region)** `(iface, buf, len)` — branches on the
  per-device type byte at `[index*0xec + 0x80109b]`:
  - type **6** (T56): `WriteData(8-byte header)` then `(len-7)` bytes, pipe 0x01.
  - type **7** (T48): 8-byte header on pipe 0x01, then `(len-8)` bytes on pipe 0x02.
  - type **5** (TL866II): header + body on pipes 0x01/0x02 with an event/timeout
    (`CreateEventA` at `0x66e354`).
- **`BulkTransfer` (`0x4dc200` region)**: writes an 8-byte header on pipe 0x01,
  then the payload on pipe 0x02 in chunks (first chunk 0x40 bytes, then the
  remainder; length arithmetic in `0x4dc214`).

## Device-type dispatch (`0x4dc420`)

Takes a type byte argument; for `0x07` checks the global type byte at
`0x7a3978`:
- `0x7a3978 == 0x31` → handler `0x4e09d0`
- else computes a count from `0x4dc3d0` (drives `[0x7a397a]`-indexed pin table
  `0x6b5804`/`0x6b6ee0`) and dispatches on the high byte of the firmware
  version word `0x7a39d4`: `0xE1` → `0x4e0400`, `0xE2` → `0x4e06d0`.

## Global state map

| Address | Size | Meaning |
|---|---|---|
| `0x7a2a80` | DWORD | enumerated device count (max 4) |
| `0x7a2a88` | 4×0x104 | device path strings |
| `0x73f170` | 4×DWORD | CreateFileA handles |
| `0x7a39e0` | 4×DWORD | WinUSB interface handles |
| `0x80109b` | per 0xec | per-device type byte (5/6/7) |
| `0x7a3978` | BYTE | firmware/device sub-type (e.g. 0x31) |
| `0x7a397a` | BYTE | device table selector (pin/`0x4dc3d0`) |
| `0x7a39d4` | DWORD | firmware version word (high byte 0xE1/0xE2) |

## Command framing

Commands are sent as an **8-byte header** on pipe 0x01 (`WriteData`), then a
data exchange on `CmdExchange` (0x4dbd50) / `BulkTransfer` (0x4dc200).

Header layout (built at e.g. `0x4c6171` for the READ block):

```
byte  0   opcode
byte  1   (unused / sub-type)
WORD  2   address or length low word   (e.g. ds:0x7a39ac)
DWORD 4   base address                (ds:0x7a39c0 if ds:0x7a39d8 & 0x1000 else 0)
```

`CmdExchange(iface, buf, len)` branches on the per-device type byte
`[idx*0xec + 0x80109b]`:
- type 6 (T48): `WriteData(iface, cmd8, 8)` then `ReadData(iface, buf, len)`.
- type 7/8 (T56): header on pipe 0x01, payload on pipe 0x02, reads on pipe
  0x82 (and 0x83 for the interleaved second stream).
- type 5 (TL866II): header on pipe 0x01, body split (`len>>1`) over pipe 0x02
  with two events + 5000 ms wait (`CreateEventA`), then **de-interleaves** two
  64-byte read streams (pipe 0x82, 0x83) into the output with the SSE loop at
  `0x4dbf60` (output stride 0x80 = two 0x40 chunks).

### READ (opcode 0x10) — `0x4c60d0`/`0x4c6157`

```
n = chip_size / block_size                // ds:0x7a39ac / arg
for i in 0..n-1:
    header = {0x10, 0, lo16(size), base_or_0}
    WriteData(iface, header, 8)
    CmdExchange(iface, buf + i*block, block)
    Verify(0x4b4780, ...)                 // checksum / status check
if size % block: send one more 0x10 block for the remainder
```

### Opcode constants seen in transport-range callers

| Opcode | Site(s) | Likely purpose |
|---|---|---|
| 0x10 | 0x4c60dc, 0x4c6171 | READ data block |
| 0x17 / 0x18 | 0x40e5be..0x40e5e7, 0x40debd..0x40df98, 0x4c69ba | program/verify data |
| 0x1d / 0x1e / 0x1f | 0x40b0ca, 0x40b557, 0x40b913, 0x4b1487, 0x4b1e45, 0x4b2908, 0x4d9b1d, 0x4d9da1, 0x4da2ba | command control |
| 0x21 | 0x4af522, 0x4f6792 | ? |
| 0x25 | 0x4b2d13, 0x4b2d6c | ? |
| 0x33 | 0x4500f0, 0x4501cf | ? |
| 0x3a | 0x4d0b40, 0x4d3847, 0x4d3b3e, 0x4d59a0, 0x4d5e99, 0x6446ba | repeated status/stream op |

Exact semantics of each opcode need per-call-site tracing (address → chip
operation) — next step after the transport skeleton.

## Firmware update files (`update*.dat`)

Selected per device type at `0x4029b3`: type 6 → `updateT56.dat`, type 7 →
`updateT48.dat`, type 8 → `UpdateT76.Dat`, else → `updateII.dat` (TL866II).

File header (T48, from `0x402b51` and the file bytes):

```
0x00  DWORD magic   0xF0480127  (T56: 0x56000149 at 0x402a3e)
0x04  DWORD id/version
0x08  DWORD 43050 / 0x0c DWORD 942 = entry count   (firmware block count)
0x10  DWORD CRC32  (chained over the entries)
0x14  DWORD payload length
0x20  ...  payload (encrypted; plaintext magic/header only)
```

Validation (`0x402aa0`/`0x402bb3`): for each of `[obj+0x4c4]` entries of
`0x114` (T48) or `0x814` (T56) bytes, chain `CRC32(entry, init=prev)` via
`0x4ee6d0` (table `0x6c3300`, no final complement); compare the total to
`[obj+0x4bc]` (the stored CRC at file offset 0x10). A separate wrapper
(`0x4ee680`) runs a second 256-byte pass over `0x7a3c10` and final-complements
— likely the key/checksum for the encrypted payload. The stored CRC does NOT
match a plain CRC32 of the file, so the payload is encrypted; the decryption
step has not been located yet.

Firmware image components referenced by the app: `BOOT1.BIN`, `BOOT2.BIN`,
`GPP1.BIN`..`GPP4.BIN`, `Flash_data.bin`.

## Next steps

- Decode the `0x1f` **0x28-byte** extended header field-by-field (addresses,
  sizes, counts) from `0x4b1dd0`..`0x4b1ec0`; trace the `0x4d9b1d` stream-loop
  variant to its caller to pin down which chip operation it drives.
- Resolve `0x17` vs `0x18` (config vs protect) by reading the `0x802ae0`/`0x803ae0`
  flag/value tables and the menu commands (解保护 / 加写保护).
- Recover the chip read/write/erase command sequence for one device type
  (start at the `0x4e0400`/`0x4e06d0` handlers for T56).
- Compare against `InfoIC2Plus.dll` exports (it hosts the chip list) and the
  `NandDLL` source for the NAND paths.
- The main-chip read/erase/program/verify sequences for the **T48** are now
  documented in `06-chip-operations.md`; use that as the reference when
  implementing the transport in the Qt6 app.

## Opcode semantics (current mapping)

Found by scanning every `mov BYTE PTR [buf],<op>` site (`tools/analyze_opcodes.py`),
then reading the surrounding header-build + error-string context. The header is
mostly the short 8-byte form `{op, byte, WORD w, DWORD d}`; `0x1f` uses a long
0x28-byte form. Evidence + confidence per row:

| Op | Meaning | Evidence | Confidence |
|---|---|---|---|
| `0x10` | READ block | `0x4c60d0`/`0x4c6157` full loop: `size(0x7a39ac)`/blocksize blocks, WriteData + CmdExchange + verify `0x4b4780`, remainder block | high |
| `0x17`/`0x18` | Config/protect commands, table-driven | `0x40e5be`: per-index flag `[edi*2+0x802ae0]&1` picks op, DWORD from table `[edi*4+0x803ae0]` into header+4, then WriteData ×2. `0x4c69ba`: header `{0x18, 0x7a39be(device mode), ...}` then "Data Protect Disable...OK!" / 解保护完成 → protect/unprotect | high (0x18=unprotect), med (0x17) |
| `0x1d` | Status/flag read | `0x40b0ca`: `{0x1d, ?, WORD addr(di), DWORD byte}` → WriteData (pipe 1) then ReadData 0x40 bytes (pipe 0x81); response bitfield processed at `0x40b120` | med |
| `0x1e` | as `0x1d` | `0x40b913` site, same shape | low |
| `0x1f` | Block program (extended header) | `0x4b1e45`: **0x28-byte** header `{0x1f, flag(!0x7475fc), WORD 0, +8..+0x18: DWORD sizes/counts 0x74760c/0x747610/0x74761c/0x747618/0x747620, ...}` then WriteData; stream-loop variant at `0x4d9b1d` sends pin/data byte in header[1] + pipe-2 payload, waits ds:0x66e348 | med |
| `0x21` | Family-handler command | sites inside the chip-family functions `0x4685b0/0x46a5a0/0x46d200/0x46faa0` (called from dispatcher `0x462c64`..`0x462c92`), also `0x4e30b0`/`0x4f65e0` | low |
| `0x25` | WRITE (program data) | `0x4b2d13`/`0x4b2d6c`: 8-byte header `{0x25,...}` via raw WinUsb_WritePipe (0x633e6c) + payload; timeout string "Write Time Out! %d" (`0x694b38`); "Curret i= %d" (`0x694b70`) progress | med |
| `0x3a` | NAND command (status byte) | `0x4d0b30` wrapper: `{0x3a, ?, WORD param}` 8+8 bytes, status at resp[1]; error path (0x4d387a) checks it; strings 自动跳过坏块 / " Auto Skipped Bad Block" / "    %s : Block#%d" / 发现坏块已替换 → NAND bad-block auto-skip in read/write loop | med |

Layout notes:
- Chip-family handler functions live in a row: `0x4685b0`, `0x46a5a0`, `0x46d200`,
  `0x46faa0` (one caller each: `0x462c92`, `0x462c88`, `0x462c64`, `0x462c76`)
  — each uses several of `0x17/0x18/0x1f/0x21/0x25`. A single monster function
  `0x596b14` (≥ 0x5a10) also exercises them all (likely the main chip
  operation engine / progress loop).
- NAND read/write path (`0x4d3800` region) treats `0x3a` responses with
  `0x79ab94` as a bad-block policy switch (0=skip, 2=replace).
- Several `0x25`/`0x5c`-ish `mov BYTE PTR` sites in `0x63fb72`..`0x641175`
  are string/escaping helpers, **not** protocol.
