# 04 — Algorithm files (`.alg`)

The reference distribution ships one `.alg` file per chip family in the
`algorithm/` folder (354 files). These files are **8051 machine-code images**
that the programmer's MCU executes to talk to a chip family. They contain the
low-level sequences for read / write / erase / verify / chip-identify etc.

For contributors this matters because:

- adding a new chip often only requires an **algorithm** for it;
- a chip that is electrically identical to an already-supported one can reuse
  (or be cloned from) an existing `.alg`;
- the whole design is an open problem once the container + hardware interface
  are documented — no proprietary blob format is involved, just 8051 code.

This document describes the container and what is known so far. Everything was
recovered by static analysis of the distribution files; it is a work in
progress.

## 1. File layout

| Offset  | Size | Field                                  | Notes                              |
|---------|------|----------------------------------------|------------------------------------|
| `0x000` | 16   | ASCII family name, NUL-padded          | e.g. `AT45DB\0`, `AM28F020A\0`     |
| `0x010` | ...  | sparse metadata region                 | mostly `00`; AT45D31 has `01 00 00 00 01 00 00 00` at `0x120` — purpose unknown |
| `0x220` | 2    | header magic (u16 LE)                  | `0x327C`, `0x3394`, `0x34A8`, `0x35BC` observed; varies by family |
| `0x222` | 2    | version (u16 LE)                       | `0x0005` on every file so far      |
| `0x224` | 4    | per-file hash (u32 LE)                 | not a plain CRC32; purpose unknown (integrity / signature?) |
| `0x228` | 16   | gap                                   | all `0xFF`                          |
| `0x238` | 4    | signature                             | `99 55 66 AA`                       |
| `0x23C` | 4    | prologue                              | `85 0C E0 00` = `MOV ACC,0x0C; NOP` |
| `0x240` | ...  | **code entry point**                  | 8051 machine code, runs to EOF     |

Notes:

- 353 of 354 files follow this header. `ROM40P2A.alg` has no ASCII name field.
- The `85 0C XX 00` pattern (`MOV <reg>,0x0C; NOP`) recurs throughout the code:
  `0x0C` in internal RAM appears to be the current-value register, copied to
  port registers during programming.
- The code region contains `MOVX` (external memory / port) accesses and makes
  heavy use of `MOV C,bit` / `SETB`/`CLR`/`JB` on bit-addressable SFRs
  (`0xBC`, `0xC9`, ...) — these drive the programmer's pin/voltage hardware.

## 2. Instruction set

Standard Intel 8051/8052. The tool `tools/alg_disasm.py` disassembles any
`.alg`:

```sh
# disassemble from the code entry point (default)
python3 tools/alg_disasm.py algorithm/AT45D31.alg

# dump container headers for every file
python3 tools/alg_disasm.py --scan algorithm/
```

Example from `AT45D31.alg`:

```
0240: NOP
0241: INC A
0242: MOV 0x01,TH0          ; 0x8C written to IRAM 0x01
0245: PUSH DPL
0247: MOV 0x30,R4
0249: CJNE R4,#86,01D1
024C: MOV @R1,#90
024E: ORL TH0,#00
0251: JB C9,021C            ; bit 1 of 0xC8
0254: MOV 0x0C,@R1
0256: MOVX @R1,A
0257: NOP
0258: MOVC A,@A+PC          ; table lookup (data tables embedded in code)
025A: AJMP 0400
```

## 3. Execution model (current understanding)

- The host (PC software) **uploads** the `.alg` image to the programmer and
  tells it to jump to the entry point (`0x240`). The upload command on the USB
  pipe is not yet decoded (see `02-usb-transport.md`).
- The same `.alg` files are used across TL866II / T48 / T56, so the 8051 code is
  written against a stable hardware interface the firmware provides on all
  models — a common pin-driver / voltage abstraction.
- Data tables (status bytes, chip-id lists, timing nibbles) are embedded in the
  code stream and reached via `MOVC A,@A+PC` / `MOVC A,@A+DPTR`.

### Runtime wiring in this project

`ChipDatabase::loadReferenceData()` stores each algorithm's file name in
`ChipInfo::note`, and `ChipDatabase::algorithmFile()` resolves it back to a
full path. The chip dialog shows the `.alg` file as a tooltip, the device list
has an "Algorithm" column, and `runOperation()` takes the resolved path so the
operations layer is algorithm-aware (it validates that the file exists before
reaching the hardware layer).

## 4. Reverse-engineering state

| Area                           | Status                     |
|--------------------------------|----------------------------|
| Container header               | done (above)               |
| Instruction decoding           | done (`tools/alg_disasm.py`) |
| Code entry point / prologue    | done                       |
| Hardware register map (pins, VCC/VPP, shift registers) | unknown — the SFR/direct addresses above the standard 8052 map need decoding |
| Host upload + execute protocol | unknown — needs USB capture / RE of the host binary |
| Operation dispatch (how host selects "read chip" vs "erase") | unknown |
| Per-file hash at `0x224`       | unknown (not CRC32)        |

### Suggested next steps for contributors

1. **Capture the protocol.** Sniff USB (e.g. `usbmon` + Wireshark) while
   running Xgpro through one Read of a known SPI flash, and correlate the first
   bulk transfers with the upload of the matching `.alg`. That nails the upload
   command and the entry-point convention.
2. **Map the hardware registers.** Pick a simple algorithm (a 25-series SPI
   flash has few pins), disassemble it fully, and identify every `MOVX`/port
   write; derive which bytes select pins, drive SCK/MOSI/MISO, set VCC, set
   VPP. Cross-check against the TL866 hardware documentation from the open
   minipro project (the TL866 shares much of the pin-driver design with the
   newer models).
3. **Document the dispatch table.** Find the jump/call pattern the firmware
   uses to enter each sub-operation (read sector, program sector, verify,
   erase, ID). This is likely a table of `LCALL`/`LJMP` targets near the entry
   point.
4. **Write new algorithms.** Once (1)–(3) are understood, a new SPI flash
   supporting standard commands can be added by cloning an existing SPI-flash
   `.alg` and adjusting the instruction set / chip-id in its embedded tables.

## 5. Related docs

- `02-usb-transport.md` — device handshake and pipe layout (host side).
- `03-buffer-and-hexview.md` — the buffer the algorithms read from / write to.
- `tools/alg_disasm.py` — 8051 disassembler + header dumper.
