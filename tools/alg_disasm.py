#!/usr/bin/env python3
"""Disassemble Xgpro algorithm files (.alg) as Intel 8051 machine code.

The reference Xgpro distribution ships one .alg per chip family. Each file is a
8051/8052 firmware image that the programmer's MCU executes to talk to the
chip. See docs/reverse-engineering/04-algorithm-format.md for the container
layout.

Usage:
    alg_disasm.py <file.alg> [start] [count]
        Disassemble `count` instructions from byte offset `start`.
        Defaults: start = 0x240 (code entry), count = until end of file.

    alg_disasm.py --info <file.alg> ...
        Print the container header fields for each file:
        family name, header magic, version, per-file hash, size.

    alg_disasm.py --scan <dir>
        Print the header fields for every *.alg in a directory (defaults to
        the standard "algorithm" subfolder).
"""

import os
import struct
import sys

# --------------------------------------------------------------------------
# Container layout (see docs/reverse-engineering/04-algorithm-format.md)
# --------------------------------------------------------------------------
NAME_OFFSET = 0x00
NAME_BYTES = 16
HDR_OFFSET = 0x220        # u16 LE magic, u16 LE version, u32 LE hash
HASH_OFFSET = 0x224
GAP_OFFSET = 0x228        # 16 bytes of 0xFF
SIGNATURE_OFFSET = 0x238  # 4-byte marker: 99 55 66 AA
PROLOGUE_OFFSET = 0x23C   # 85 0C E0 00  (MOV ACC,0x0C; NOP)
CODE_OFFSET = 0x240       # code entry point

SIGNATURE = bytes([0x99, 0x55, 0x66, 0xAA])

# --------------------------------------------------------------------------
# 8051 disassembler
# --------------------------------------------------------------------------
_NAMES = ["R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"]
_SFR = {
    0x80: "P0", 0x81: "SP", 0x82: "DPL", 0x83: "DPH", 0x87: "PCON",
    0x88: "TCON", 0x89: "TMOD", 0x8A: "TL0", 0x8B: "TL1", 0x8C: "TH0",
    0x8D: "TH1", 0x90: "P1", 0x98: "SCON", 0x99: "SBUF", 0xA0: "P2",
    0xA8: "IE", 0xB0: "P3", 0xB8: "IP", 0xD0: "PSW", 0xE0: "ACC",
    0xF0: "B",
}
_ACALLS = (0x11, 0x31, 0x51, 0x71, 0x91, 0xB1, 0xD1, 0xF1)
_AJMPS = (0x01, 0x21, 0x41, 0x61, 0x81, 0xA1, 0xC1, 0xE1)


def _direct(b):
    return _SFR.get(b, "0x%02X" % b)


def disassemble(data, org=0):
    """Yield (address, mnemonic) pairs decoding 8051 code in `data`."""
    pc, n = 0, len(data)
    while pc < n:
        st = pc
        op = data[pc]
        pc += 1
        b1 = data[pc] if pc < n else 0
        b2 = data[pc + 1] if pc + 1 < n else 0

        def rel():
            return org + pc + ((b1 ^ 0x80) - 0x80) + 1

        def addr11():
            return (org + st & 0xF800) | ((op & 0xE0) << 3) | b1

        m = None
        if op == 0x00:
            m = "NOP"
        elif op in _AJMPS:
            m = "AJMP %04X" % addr11(); pc += 1
        elif op == 0x02:
            m = "LJMP %04X" % ((b1 << 8) | b2); pc += 2
        elif op == 0x03: m = "RR A"
        elif op == 0x04: m = "INC A"
        elif op == 0x05:
            m = "INC %s" % _direct(b1); pc += 1
        elif op in (0x06, 0x07): m = "INC @R%d" % (op & 1)
        elif 0x08 <= op <= 0x0F: m = "INC %s" % _NAMES[op - 8]
        elif op == 0x10:
            m = "JBC %02X,%04X" % (b1, rel()); pc += 2
        elif op in _ACALLS:
            m = "ACALL %04X" % addr11(); pc += 1
        elif op == 0x12:
            m = "LCALL %04X" % ((b1 << 8) | b2); pc += 2
        elif op == 0x13: m = "RRC A"
        elif op == 0x14: m = "DEC A"
        elif op == 0x15:
            m = "DEC %s" % _direct(b1); pc += 1
        elif op in (0x16, 0x17): m = "DEC @R%d" % (op & 1)
        elif 0x18 <= op <= 0x1F: m = "DEC %s" % _NAMES[op - 0x18]
        elif op == 0x20:
            m = "JB %02X,%04X" % (b1, rel()); pc += 2
        elif op == 0x22: m = "RET"
        elif op == 0x23: m = "RL A"
        elif op == 0x24:
            m = "ADD A,#%02X" % b1; pc += 1
        elif op == 0x25:
            m = "ADD A,%s" % _direct(b1); pc += 1
        elif op in (0x26, 0x27): m = "ADD A,@R%d" % (op & 1)
        elif 0x28 <= op <= 0x2F: m = "ADD A,%s" % _NAMES[op - 0x28]
        elif op == 0x30:
            m = "JNB %02X,%04X" % (b1, rel()); pc += 2
        elif op == 0x32: m = "RETI"
        elif op == 0x33: m = "RLC A"
        elif op == 0x34:
            m = "ADDC A,#%02X" % b1; pc += 1
        elif op == 0x35:
            m = "ADDC A,%s" % _direct(b1); pc += 1
        elif op in (0x36, 0x37): m = "ADDC A,@R%d" % (op & 1)
        elif 0x38 <= op <= 0x3F: m = "ADDC A,%s" % _NAMES[op - 0x38]
        elif op == 0x40:
            m = "JC %04X" % rel(); pc += 1
        elif op == 0x42:
            m = "ORL %s,A" % _direct(b1); pc += 1
        elif op == 0x43:
            m = "ORL %s,#%02X" % (_direct(b1), b2); pc += 2
        elif op == 0x44:
            m = "ORL A,#%02X" % b1; pc += 1
        elif op == 0x45:
            m = "ORL A,%s" % _direct(b1); pc += 1
        elif op in (0x46, 0x47): m = "ORL A,@R%d" % (op & 1)
        elif 0x48 <= op <= 0x4F: m = "ORL A,%s" % _NAMES[op - 0x48]
        elif op == 0x50:
            m = "JNC %04X" % rel(); pc += 1
        elif op == 0x52:
            m = "ANL %s,A" % _direct(b1); pc += 1
        elif op == 0x53:
            m = "ANL %s,#%02X" % (_direct(b1), b2); pc += 2
        elif op == 0x54:
            m = "ANL A,#%02X" % b1; pc += 1
        elif op == 0x55:
            m = "ANL A,%s" % _direct(b1); pc += 1
        elif op in (0x56, 0x57): m = "ANL A,@R%d" % (op & 1)
        elif 0x58 <= op <= 0x5F: m = "ANL A,%s" % _NAMES[op - 0x58]
        elif op == 0x60:
            m = "JZ %04X" % rel(); pc += 1
        elif op == 0x62:
            m = "XRL %s,A" % _direct(b1); pc += 1
        elif op == 0x63:
            m = "XRL %s,#%02X" % (_direct(b1), b2); pc += 2
        elif op == 0x64:
            m = "XRL A,#%02X" % b1; pc += 1
        elif op == 0x65:
            m = "XRL A,%s" % _direct(b1); pc += 1
        elif op in (0x66, 0x67): m = "XRL A,@R%d" % (op & 1)
        elif 0x68 <= op <= 0x6F: m = "XRL A,%s" % _NAMES[op - 0x68]
        elif op == 0x70:
            m = "JNZ %04X" % rel(); pc += 1
        elif op == 0x72:
            m = "ORL C,%02X" % b1; pc += 1
        elif op == 0x73: m = "JMP @A+DPTR"
        elif op == 0x74:
            m = "MOV A,#%02X" % b1; pc += 1
        elif op == 0x75:
            m = "MOV %s,#%02X" % (_direct(b1), b2); pc += 2
        elif op in (0x76, 0x77):
            m = "MOV @R%d,#%02X" % (op & 1, b1); pc += 1
        elif 0x78 <= op <= 0x7F:
            m = "MOV %s,#%02X" % (_NAMES[op - 0x78], b1); pc += 1
        elif op == 0x80:
            m = "SJMP %04X" % rel(); pc += 1
        elif op == 0x82:
            m = "ANL C,%02X" % b1; pc += 1
        elif op == 0x83: m = "MOVC A,@A+PC"
        elif op == 0x84: m = "DIV AB"
        elif op == 0x85:
            m = "MOV %s,%s" % (_direct(b2), _direct(b1)); pc += 2
        elif op == 0x86:
            m = "MOV %s,@R0" % _direct(b1); pc += 1
        elif op == 0x87:
            m = "MOV %s,@R1" % _direct(b1); pc += 1
        elif 0x88 <= op <= 0x8F:
            m = "MOV %s,%s" % (_direct(b1), _NAMES[op - 0x88]); pc += 1
        elif op == 0x90:
            m = "MOV DPTR,#%04X" % ((b1 << 8) | b2); pc += 2
        elif op == 0x92:
            m = "MOV %02X,C" % b1; pc += 1
        elif op == 0x93: m = "MOVC A,@A+DPTR"
        elif op == 0x94:
            m = "SUBB A,#%02X" % b1; pc += 1
        elif op == 0x95:
            m = "SUBB A,%s" % _direct(b1); pc += 1
        elif op in (0x96, 0x97): m = "SUBB A,@R%d" % (op & 1)
        elif 0x98 <= op <= 0x9F: m = "SUBB A,%s" % _NAMES[op - 0x98]
        elif op == 0xA0:
            m = "ORL C,/%02X" % b1; pc += 1
        elif op == 0xA2:
            m = "MOV C,%02X" % b1; pc += 1
        elif op == 0xA3: m = "INC DPTR"
        elif op == 0xA4: m = "MUL AB"
        elif op in (0xA6, 0xA7):
            m = "MOV @R%d,%s" % (op & 1, _direct(b1)); pc += 1
        elif 0xA8 <= op <= 0xAF:
            m = "MOV %s,%s" % (_NAMES[op - 0xA8], _direct(b1)); pc += 1
        elif op == 0xB0:
            m = "ANL C,/%02X" % b1; pc += 1
        elif op == 0xB2:
            m = "CPL %02X" % b1; pc += 1
        elif op == 0xB3: m = "CPL C"
        elif op == 0xB4:
            m = "CJNE A,#%02X,%04X" % (b1, rel()); pc += 2
        elif op == 0xB5:
            m = "CJNE A,%s,%04X" % (_direct(b1), rel()); pc += 2
        elif op in (0xB6, 0xB7):
            m = "CJNE @R%d,#%02X,%04X" % (op & 1, b1, rel()); pc += 2
        elif 0xB8 <= op <= 0xBF:
            m = "CJNE %s,#%02X,%04X" % (_NAMES[op - 0xB8], b1, rel()); pc += 2
        elif op == 0xC0:
            m = "PUSH %s" % _direct(b1); pc += 1
        elif op == 0xC2:
            m = "CLR %02X" % b1; pc += 1
        elif op == 0xC3: m = "CLR C"
        elif op == 0xC4: m = "SWAP A"
        elif op == 0xC5:
            m = "XCH A,%s" % _direct(b1); pc += 1
        elif op in (0xC6, 0xC7): m = "XCH A,@R%d" % (op & 1)
        elif 0xC8 <= op <= 0xCF: m = "XCH A,%s" % _NAMES[op - 0xC8]
        elif op == 0xD0:
            m = "POP %s" % _direct(b1); pc += 1
        elif op == 0xD2:
            m = "SETB %02X" % b1; pc += 1
        elif op == 0xD3: m = "SETB C"
        elif op == 0xD4: m = "DA A"
        elif op == 0xD5:
            m = "DJNZ %s,%04X" % (_direct(b1), rel()); pc += 2
        elif op in (0xD6, 0xD7): m = "XCHD A,@R%d" % (op & 1)
        elif 0xD8 <= op <= 0xDF:
            m = "DJNZ %s,%04X" % (_NAMES[op - 0xD8], rel()); pc += 1
        elif op == 0xE0: m = "MOVX A,@DPTR"
        elif op in (0xE2, 0xE3): m = "MOVX A,@R%d" % (op & 1)
        elif op == 0xE4: m = "CLR A"
        elif op == 0xE5:
            m = "MOV A,%s" % _direct(b1); pc += 1
        elif op in (0xE6, 0xE7): m = "MOV A,@R%d" % (op & 1)
        elif 0xE8 <= op <= 0xEF: m = "MOV A,%s" % _NAMES[op - 0xE8]
        elif op == 0xF0: m = "MOVX @DPTR,A"
        elif op in (0xF2, 0xF3): m = "MOVX @R%d,A" % (op & 1)
        elif op == 0xF4: m = "CPL A"
        elif op == 0xF5:
            m = "MOV %s,A" % _direct(b1); pc += 1
        elif op in (0xF6, 0xF7): m = "MOV @R%d,A" % (op & 1)
        elif 0xF8 <= op <= 0xFF: m = "MOV %s,A" % _NAMES[op - 0xF8]
        else:
            m = "DB %02X" % op
        yield org + st, m


def header_info(path):
    with open(path, "rb") as f:
        data = f.read()
    name = data[NAME_OFFSET:NAME_OFFSET + NAME_BYTES].split(b"\x00")[0].decode("latin1")
    magic = struct.unpack_from("<H", data, HDR_OFFSET)[0]
    version = struct.unpack_from("<H", data, HDR_OFFSET + 2)[0]
    hsh = struct.unpack_from("<I", data, HASH_OFFSET)[0]
    sig = data[SIGNATURE_OFFSET:SIGNATURE_OFFSET + 4] == SIGNATURE
    return {
        "name": name, "magic": magic, "version": version,
        "hash": hsh, "size": len(data), "sig": sig,
    }


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    if argv[0] == "--info":
        for path in argv[1:]:
            info = header_info(path)
            print("%-40s name=%-16s magic=0x%04X ver=%d hash=0x%08X size=%d sig=%s"
                  % (path, info["name"], info["magic"], info["version"],
                     info["hash"], info["size"], info["sig"]))
        return 0
    if argv[0] == "--scan":
        folder = argv[1] if len(argv) > 1 else "algorithm"
        files = sorted(fn for fn in os.listdir(folder) if fn.endswith(".alg"))
        for fn in files:
            info = header_info(os.path.join(folder, fn))
            print("%-16s %-16s 0x%04X v%d 0x%08X %d" %
                  (fn, info["name"], info["magic"], info["version"],
                   info["hash"], info["size"]))
        return 0

    path = argv[0]
    start = int(argv[1], 0) if len(argv) > 1 else CODE_OFFSET
    count = int(argv[2], 0) if len(argv) > 2 else (1 << 30)
    with open(path, "rb") as f:
        data = f.read()
    printed = 0
    for addr, mnem in disassemble(data[start:], org=start):
        if printed >= count:
            break
        print("%04X: %s" % (addr, mnem))
        printed += 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
