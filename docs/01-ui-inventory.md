# UI Inventory

Recovered from the resource tree of `Xgpro.exe` with `tools/dump_resources.py`.
All sizes are dialog units (DLU) unless noted; text is UTF-16LE as stored in the
binary (Chinese labels in Simplified Chinese, lang id 2052).

## Menus

### Main menu (resource 128)

MFC convention: the file/help/edit command IDs are the standard `0xE1xx`
range; the application-specific commands reuse the dialog-control IDs
(e.g. "读芯片内容(&R)" is command 1002, the Read button in dialog 101).

```
文件(&F)
  打开文件(&O)...            Ctrl+O   0xE101 (ID_FILE_OPEN)
  保存到文件(&S)             Ctrl+S   0x03EF (1007)
  ---
  复制                       Ctrl+C   0xE122 (ID_EDIT_COPY)
  粘贴                       Ctrl+V   0xE125 (ID_EDIT_PASTE)
  块另存为(&B)   txt文件              0x8039 (32825)
  块定义                      Ctrl+B   0x8032 (32818)
  ---
  块填充(&F)                          0x800E (32782)
  清空当前缓存(&C)                    0x8020 (32800)
  清空所有缓冲(&A)                    0x8021 (32821)
  ---
  查找                       Ctrl+F   0x8016 (32822)
  查找下一个                 F3       0x8017 (32823)
  地址定位(&G)               Ctrl+G   0x8018 (32824)
  ---
  退出(&X)                            0xE141 (ID_APP_EXIT)
芯片选择(&S)
  查找选择芯片（&S）                   0x0406 (1030)
  ---
  25 Flash识别                        0x803B (32843)
  ---
  ADD BY USER                         0x803C (32858)
工程(&P)
  打开工程(&O)                        0x8020 (32826)
  保存工程(&B)                        0x8021 (32827)
  工程另存为(&S)                      0x8022 (32828)
  ---
  关闭当前工程(&C)                    0x8023 (32829)
  ---
  工程属性(&A)                        0x8024 (32830)
  更改工程密码(&K)                    0x8025 (32831)
操作(&D)
  读芯片内容(&R)                      0x03EA (1002)
  芯片ID识别(&I)                      0x801E (32798)
  数据校验(&V)                        0x03ED (1005)
  ---
  芯片编程(&P)                        0x03EB (1003)
  擦除芯片内容(&E)                    0x801C (32796)
  芯片查空(&B)                        0x8021 (32801)
  ---
  自动编号设定                        0x8027 (32807)
  ---
  测试                                0x802A (32810)
  ---
  多机编程                            0x804F (32847)
  NAND_坏块检查                       0x8051 (32849)
  LOGIC TEST                          0x8060 (32864)
  TV/VGA Tools                        0x8067 (32871)
系统工具(&V)
  计算器(&T)                          0x8004 (32772)
  ---
  编程器自检                          0x8028 (32808)
  ---
  固件FLASH刷新                       0x8003 (32771)
  适配器测试                          0x806C (32876)
帮助(&H)
  帮助(&H)                   F1       0xE140 (ID_HELP)
  ---
  关于 MiniPro(&A)                    0xE141 (ID_APP_ABOUT)
  ---
  编程器在线升级                      0x8041 (32833)
Language(&L)          (built at runtime; entries loaded from Language*.INI)
  简体中文(&S)                        0x8026 (32834)
  &English                            0x8027 (32835)
  &Russian                            0x8032 (32850)
  &Polish                             0x8033 (32851)
  German                              0x8034 (32852)
  UserDefine1                         0x8036 (32854)
  UserDefine2                         0x8037 (32855)
  &Turkish(T)                         0x8038 (32856)
  &CZech(C)                           0x803C (32872)
  &Italian(I)                         0x803E (32874)
  &French(F)                          0x803F (32877)
  &Hungarian                          0x8040 (32878)
  User_Set                            0x803D (32873)
```

### Right-click / toolbar popup (resource 158)

Two top-level popups: `右键` (right-click context menu on the device list) and
`DownList` (the drop-down attached to the toolbar button). Shares the same
command IDs as menu 128.

```
右键
  复制            Ctrl+C   0xE122
  粘贴            Ctrl+V   0xE125
  块另存为(&S)    txt文件  0x8039
  块定义          Ctrl+B   0x8032
  ---
  块填充(&F)                0x8033 (32819)
  清空当前缓冲(&C)          0x8034 (32820)
  清空所有缓冲(&A)          0x8021 (32821)
  ---
  查找            Ctrl+F   0x8016
  查找下一个      F3       0x8017
  地址定位(&G)    Ctrl+G   0x8018
DownList
  ---最近操作的10个芯片----           0x8029 (32841, grayed header)
```

## Dialogs (39 total)

### Main window — dialog 101 (572x356 DLU, 65 items, no title bar, "MS Shell Dlg" 9pt)

Layout regions (x,y in DLU):

| Region | Controls |
|---|---|
| Top-left: device/interface | id 1274 "芯片选择" (7,2,185,33), id 1276 "选择编程接口" (11,35,209,22), id 1030 "27C512A" (10,13), id 1290 "40PIN锁紧座" (14,45), id 1291 "ICSP串行接口" (74,45), id 1145 "ICSP_VCC Enable" (144,46), id 1258 "8 Bits", id 1259 "16 Bits", id 1454 "Vcc current Imax:", id 1455 combobox (298,45), id 1242 "Save Log" (502,43), id 1241 "Clear" (542,43), id 1450 "Upgrade is avaliable", id 1111 (463,11) |
| Top-right: chip info | id 1126 "芯片信息(No Project opened)" (195,2,375,33), id 1277 "芯片类型:", id 1125 "unkown", id 1034 "累加和:", id 1124 " 0000 0000", id 1133 "时间:", id 1036 " 2000-00-00", id 1203 "V" |
| Center: chip list | id 1000 List1 SysListView32 (7,58,464,184), id 1433 scrollbar (471,59), id 1261 LISTBOX (490,59,74,183), id 1262/1264 " 待写入文件:"/" 保存到文件:", id 1429/1431 edit (410,304/319), id 1263 "选择数据文件", id 1271 "选择目标文件", id 1083 "Static", id 1436 "编号自增" |
| Tabs | id 1272 SysTabControl32 (7,244,352,16) |
| Bottom-left: programming settings | id 1278 "编程设置" (7,267,215,83), id 1422 "Pin Detect", id 1443 "检查ID", id 1400 "编程前先擦除", id 1405 "编程后校验", id 1417 "EraseOTP", id 1434 "编程前查空", id 1423 "跳过写0xFF", id 1279 "编程范围:", id 1023 "部分", id 1024 "全部", id 1268 "0x", id 1081/1082 edit (110,337/168,336), id 1269 "->", id 1549 "Block:", id 1548 edit (249,323), id 1552 updown |
| Bottom-right: SPI EEPROM status + chip config | id 1020 "SPI EEPROM(25/35/95系列) 状态位" (289,298,258,46), id 1025 "读状态", id 1026 "写状态", id 1027 List2 (326,274,242,24), id 1076 "芯片配置信息" (227,267,341,83), id 1077 "static", id 1089 "USERID:", id 1092 "编程前取消写保护", id 1093 "编程后加写保护", id 1094 "解保护", id 1136 "保护模式，部分功能已被禁用!" |

Device-list column model (from the app's own list columns): the two-column
list (id 1000/1007) shows chip family vs device; item 1136 is a grayed
"protected mode" notice; item 1020/1076 are checkbox-style state groups.

### About / prompt dialogs (selected)

- **100 About** — 800 bytes, 10 items, "Times New Roman" 9pt. Title: "关于 MiniPro".
- **156** — nonextended template, classic style (54 bytes).
- **194 / 204 / 206** — 64 bytes, nonextended templates (single-line prompts).
- **208 / 211 / 216 / 218 / 226 / 229** — 114..1184 bytes, nonextended.
- **30721 "新建"** — nonextended, 5 items: 新建(&N) static, edit 100, 确定 1, 取消 2, 帮助(&H) 57670.
- **30722** — nonextended, 10 items (strings incl. "检查:", "校验:", "出错:", "芯片ID:" labels with edits).
- **30734** — nonextended, 0 items (empty 52-byte dialog).

Full per-item dumps are in `external/reference/dialogs.json`; RC-style
renderings are in the `tools/dump_resources.py` console output.

## Notes for the Qt port

- Dialog units: 1 DLU ≈ 1/4 font-width, 1/8 font-height (MS Shell Dlg 9pt →
  8px width, 16px height). Convert `x`/`y` × 8px and `cx`/`cy` × 8px.
- `0xFFFF` prefix on a class/text field means "ordinal": class ordinals 0x80
  BUTTON, 0x81 EDIT, 0x82 STATIC, 0x83 LISTBOX, 0x84 SCROLLBAR, 0x85 COMBOBOX;
  a text ordinal (e.g. 0x98 on id 1111) is an icon ID.
- Command IDs 1002/1003/1005/1007/1030 in menu 128 == dialog-control IDs in
  dialog 101 (MFC `ON_COMMAND` pattern) — the menu and the on-screen buttons
  share handlers.
- `Language(&L)` popup is rebuilt at startup from `Language*.INI`; resource
  strings live in the 2052 string table (`external/reference/strings.json`).
