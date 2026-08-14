# 06 — Chip operations (read / erase / program / verify)

How the host drives a chip operation over the USB transport, recovered from
`Xgpro.exe` (i386 MFC). This is the layer that turns a menu command into the
`0x10`/`0x11`/`0x0e`/… byte sequences of `02-usb-transport.md`. It applies to
the **T48 (device type 7)** unless noted.

## Flow overview

```
user clicks socket button / Z-X-C-V key
  └─ worker (0x445180 / 0x445880 / 0x445f80 / 0x446680)
      └─ job context 0x4e25c0 + thread
          └─ wrapper dispatch on ds:0x7a3978 (chip-family subtype):
                '1'  → 0x4c9110
                '-'  → 0x4da4a0
                else → engine 0x4c7040        ← T48 main path
```

The menu handlers (command 1002 Read / 1003 Program / 1005 Verify /
0x801c Erase) do **not** run the engine themselves — they only validate
(0x47f300), record the op type at `[obj+0x63c54]`, and run the modal dialog
pump (0x50714f). The actual engine is started by the socket-button workers
(or the Z/X/C/V keyboard shortcuts).

## Engine 0x4c7040 — opcode table

All headers are the 8-byte form `{op, 0, WORD len, DWORD addr}` on pipe 0x01.
`WriteData` = 0x4dc380, `ReadData` = 0x4dc300, `CmdExchange` = 0x4dbd50,
`SendBlock` = 0x4dc070.

| Op | Meaning | Builder | Header | Response |
|---|---|---|---|---|
| `0x0e` | erase | 0x40ee71 / 0x409d70 | `{0x0e, 0, 0, 0, DWORD erase_code}` | WriteData(8), ReadData(0x40) |
| `0x0d` | blank check | 0x4c6460 | `{0x0d, 0, WORD bs, DWORD addr}` | CmdExchange(bs) |
| `0x11` | program block | 0x4c8174 | `{0x11, 0, WORD bs, DWORD addr}` + data | SendBlock(hdr + payload) |
| `0x10` | verify / read block | 0x4c8426 / 0x4c5f50 | `{0x10, 0, WORD bs, DWORD addr}` | WriteData(8) + CmdExchange(bs) |
| `0x0a` | program user row / encryption record | 0x4c86bd | `{0x0a, 0, WORD 0x20, DWORD addr}` + 32 B | WriteData(0x28) |
| `0x0b` | read user row | 0x4c886c | `{0x0b, 0, WORD 0x40, DWORD 0}` | WriteData(8) + ReadData(0x40) |
| `0x07` | program UserID | 0x4c89fc | `{0x07, 0, WORD 0x20}` + 32 B | WriteData(0x40) |
| `0x06` | read UserID | 0x4c75b4 / 0x4c8b01 | `{0x06, 0, 0, 0}` | WriteData(8) + ReadData(0x40 / 0x28) |
| `0x0c` | send block (pre-verify data) | 0x4c78be | `{0x0c, …}` | — |

Byte 1 of every header is never written by the builder — it stays 0 (reserved).

## Read (opcode 0x10) — engine 0x4c5f50

```
count = ds:0x7a39ac / blocksize          # chip size / block size
for i in 0..count-1:
    addr = (ds:0x7a39d8 & 0x1000) ? ds:0x7a39c0 + i*bs : i*bs
    hdr  = {0x10, 0, WORD bs, DWORD addr}
    WriteData(iface, hdr, 8)
    CmdExchange(iface, buf + i*bs, bs)   # data in
    Verify(0x4b4780, …)
if size % bs:
    read remainder with low16 size
```

## Program (opcode 0x11) — engine 0x4c7040 → 0x4c8150

```
for addr in 0..chip_size step blocksize:
    hdr = {0x11, 0, WORD blocksize, DWORD addr}   # addr math as in READ
    SendBlock(iface, hdr + block_data, blocksize + 8)
        # T48: 8-byte header on pipe 0x01, then (len-8) bytes on pipe 0x02
verify step (if ds:0x80187c set):
    per block: opcode 0x10 via WriteData + CmdExchange, compare 0x4b3060
```

## Erase (opcode 0x0e)

Engine 0x4c7040 → 0x40ea80 when `ds:0x801879` (erase-before checkbox) is set;
standalone erase used by blank-check/erase is 0x409d70:

```
erase_code = chip_table[ds:0x7a39a8 * 0x54 + 0x696c6c]
hdr = {0x0e, 0, 0, 0, DWORD erase_code}
WriteData(iface, hdr, 8)
ReadData(iface, 0x40-byte buf, …)          # status
# then opcode 0x04 complete command + ~3 s settle
```

## Chip parameters

Loaded by 0x4edaa0 (chip-param loader) from the device database, into:

| Global | Source | Meaning |
|---|---|---|
| `0x7a3978` | desc[0x00] | chip-family subtype |
| `0x7a39a8` | desc[0x34] | chip-table index (stride 0x54) |
| `0x7a39ac` | desc[0x3c] | chip size (recomputed = 0x7a39b0 * blocksize) |
| `0x7a39b0` | desc[0x40] → `0x20` | block count |
| `0x7a39c0` | — | base address (used when 0x7a39d8 & 0x1000) |
| `0x7a39d8` | desc flags \| 0x800 | addr-mode / NAND-spare flags |
| `0x80109b` | — | device type (5/6/7/8) |
| `0x80187x` | dialog checkboxes | op gating (erase/verify/blank) |

Checkbox gating (from msg 0x444…0x59a):
`0x801879` = erase-before, `0x80187c` = verify-after, `0x80187e` = blank-before.
The engine also switches on op-controller radio state: DATA Memory (0x407) →
read path, Encryption Table (0x57e) / User ID (0x596) → those sub-ops.

## Algorithm upload (opcode 0x26)

Loader 0x4bb330 reads `algorithm\<name>.alg` (name table 0x696084, selected by
fw subtype), validates it, and uploads:

- sub `0x00` init: `{0x26, 0, 0, 0x20, id}`
- sub `0x01` data chunks ≤ 504 B: `{0x26, 0x01, WORD len, 0}` — one pipe-0x01 write
- sub `0x02` finalize
- sub `0xaf` = download-pins (magic `0xaa55ddee`)

## Open questions

- The alternative program path 0x4b0ec0 → 0x4b19f0/0x4b2500 (opcodes `0x1f`
  extended 0x28-byte header + `0x25`) exists for fw subtype `'1'`; whether any
  user-facing flow reaches it on a T48 is not fully closed.
- Exact erase header bytes +4..+7 of 0x40ea80 (buffer zero-init assumed).
- The 0x4dc200 punch-list transport helper is not yet decoded.
- Whether the `0x26` upload runs at device open or per-operation is not
  confirmed from a single path (caller 0x412a00 also sends a `0x37` serial-read).
