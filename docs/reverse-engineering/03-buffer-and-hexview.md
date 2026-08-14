# Buffer Model & Hex View

Recovered from `Xgpro.exe` RTTI, dialog resources and string table. These notes
drive the Qt `HexView` reimplementation.

## The hex editor controls

The main window does **not** use a stock edit control for the buffer. RTTI type
descriptors in `.rdata` list three related custom controls:

| RTTI name | Role |
|---|---|
| `.?AVCmyHexEdit@@` | address + hex pane (custom-painted) |
| `.?AVCEditAscii@@` | right-hand ASCII column, scroll-synced with `CmyHexEdit` |
| `.?AVCmyEdit@@` | base class of both (derives from `CEdit`) |

So the buffer view is the classic **two-pane split**: left control draws the
address gutter + hexadecimal bytes, the right control draws the printable-ASCII
column; both are subclassed edits scrolled in lockstep. The device dialog 101
carries the mode radio pair **"8 Bits" / "16 Bits"** (control ids 1258/1259),
which switches the pane between byte-mode (bytes + ASCII) and word-mode
(16-bit words). A third `CListMemo` control is the log / status window.

Other custom controls worth matching: `CMyComBox` (chip combo), `CmyTabCtrl`
(tab control), `CListCtrl`/`CXListBox` (chip lists), `CListSectorLock`,
`CListLogicResultList`.

## Buffer operations (string-table + dialog evidence)

Strings referenced by the buffer engine:

```
Fill buffer
Clear buffer with default      Clear buffer with 0x00     Clear buffer with 0xFF
Clear current buffer(&C)       Clear all buffer(&A)
Select Buffer                  INTEL  (HEX)
CRC32 of the Buffer/File: 0x%.8X
Goto Address 0x:               (HEX): 0x          Value(HEX):
```

Dialog resources:

- **161 查找 (Find)** — 239×58 DLU, 7 items: search edit (id 1411),
  `查找内容:` label, `查找字符格式` group with `HEX` / `字符串` (string)
  radios, `查找下一个` / `取消` buttons. → hex- or ASCII-pattern search over
  the buffer, "Find Next" continues.
- **162 跳到地址 (Go to Address)** — 251×55, 4 items: `定位到地址: 0x` label
  + address edit (id 1412), 确定/取消. → address navigation in the hex pane.
- **163 选中一块数据用于复制 (Define block for copy)** — 187×60, 6 items:
  `开始地址:` / `结束地址:` edits (1413/1420), 确定/取消. → block start/end
  used by "块另存为/块定义/块填充" and "To Region(Buffer)".

Menu/context menu (resource 158) bindings for the buffer: Copy Ctrl+C,
Paste Ctrl+V, 块另存为(txt), 块定义 Ctrl+B, 块填充, 清空当前缓冲, 清空所有缓冲,
查找 Ctrl+F, 查找下一个 F3, 地址定位 Ctrl+G.

## 8-bit vs 16-bit display

- **8 Bits**: 16 bytes per row, address gutter + hex bytes + ASCII column;
  the ASCII pane (`CEditAscii`) shows `0x20..0x7e` as glyphs, everything else
  as `.` (standard behaviour).
- **16 Bits**: 8 words per row (16 bytes), word values, no ASCII column; the
  address format switches to a wider field (`0x%.4X` stays for ≤64 KiB, the
  NAND/MMC paths use `0x%.8X`/`0x%.6X` — see the `'0x%.2X_%.8X'` row/column
  addressing used by the big-density paths).

## Buffer identity & checksum

The app computes `CRC32 of the Buffer/File: 0x%.8X` after load/read and shows
it in the 芯片信息 group (累加和 label). File formats understood by the loader
(`All_Files(*.*)|*.*|BIN Files(*.BIN)|*.BIN|INTEL_HEX_Files(*.HEX)|*.HEX`):
raw BIN and Intel HEX.

## Status/log window

`CListMemo` (a `CListBox` subclass) is the "内存列表" log: operation lines,
`ERROR! Address:...  Buf_Val:... IC_Val:...` verify reports and progress
lines land here, not in a MessageBox.

## Reference addresses (for deeper work)

- RTTI type descriptors: `CmyHexEdit` at `.rdata` `0x340cf4`
  (name `.?AVCmyHexEdit@@` at `0x340cfc`), `CEditAscii` at `0x341075c`,
  `CmyEdit` at `0x3411172`, `CListMemo` at `0x3411008`.
- Message-map table for `CmyHexEdit` starts near `.rdata` `0x2edd4c`
  (WM_* entries follow the "Complete Object Locator" marker at `0x2edd28`).

## Data files (inventory, for the chip database milestone)

- `Serial25Index.dat` (134 732 B) — binary index for the **25-series SPI flash
  auto-identify** feature (`25 Flash识别`). No ASCII names inside; it maps
  vendor-ID/device-ID → serial index. Header: 0x00 magic `50 ac fe`, count at
  `0x08`, records of 0x18 bytes (`serial, flags, algo-index…`). Needs the
  `algorithm/*.alg` headers to resolve names.
- `Logic.lst` (96 476 B) — logic-IC list. Starts with a DWORD offset table
  (first record at `0x10`), each offset points at a logic chip entry inside
  the file (name + pins/VCC tables). Drives the `LOGIC TEST` dialog and the
  `Logic` category of the device list.
- `config.dat` (732 B) — persisted main-window geometry/settings (initial
  DWORDs are window x/y/cx/cy values), mirrors what the Qt port keeps in
  `QSettings`.
- `img/*.jpg` — adapter photos shown by the adapter-test / ICSP dialogs.

## Next steps

- Trace the `CmyHexEdit` message map (`.rdata 0x2edd4c`+) and `OnPaint` to
  confirm exact gutter/column metrics and the caret drawing.
- Parse `Logic.lst` + `algorithm/*.alg` headers to produce a real chip DB for
  the "Find & Select Chip" dialog.
