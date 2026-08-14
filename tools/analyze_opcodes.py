#!/usr/bin/env python3
"""Analyze Xgpro.exe opcode sites: enclosing function, callers, referenced strings.

Usage: analyze_opcodes.py <exe> <opcode> [opcode...]
"""
import re
import subprocess
import sys
import struct
import pefile


def strings_at(pe, va):
    try:
        d = pe.get_data(va, 64)
    except Exception:
        return None
    i = 0
    out = []
    while i < 8:
        b = d.split(b"\x00")[i]
        if len(b) >= 3:
            try:
                out.append(b.decode("utf-8"))
            except Exception:
                pass
        i += 1
    return out


def main():
    exe = sys.argv[1]
    opcodes = sys.argv[2:]
    pe = pefile.PE(exe)
    base = pe.OPTIONAL_HEADER.ImageBase
    # IAT map
    iat = {}
    for e in pe.DIRECTORY_ENTRY_IMPORT:
        for imp in e.imports:
            if imp.name:
                iat[imp.address] = f"{e.dll.decode().split('.')[0]}!{imp.name.decode()}"
    raw = subprocess.run(
        ["objdump", "-d", "-M", "intel", exe],
        capture_output=True, text=True).stdout
    # parse objdump into (addr, mnemonic, full line)
    ins = []  # (va, line)
    for ln in raw.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\t(.*)$", ln)
        if m:
            ins.append((int(m.group(1), 16), ln.strip()))
    vlist = [(va, line) for va, line in ins]
    by_addr = {va: line for va, line in ins}

    for op in opcodes:
        opi = int(op, 16)
        print(f"===== opcode 0x{op} =====")
        sites = [va for va, line in vlist if f",0x{opi:02x}" in line and "BYTE PTR" in line]
        # filter to those with a following 8-byte-header write context (any)
        for va in sites:
            # find enclosing function start: scan backward for int3 padding
            fstart = None
            idx = [i for i, (a, _) in enumerate(ins) if a == va]
            if not idx:
                continue
            i = idx[0]
            j = i
            while j > 0:
                a, line = ins[j]
                if line.endswith("int3"):
                    a2, line2 = ins[j + 1]
                    if a2 < va:
                        fstart = a2
                        break
                j -= 1
            # function end: next int3 padding after fstart
            fend = None
            if fstart is not None:
                for a, line in ins[i + 1:i + 2000]:
                    if line.endswith("int3"):
                        fend = a
                        break
            fs = f"{fstart:#x}" if fstart is not None else "?"
            fe = f"{fend:#x}" if fend is not None else "?"
            print(f"\n-- site {va:#x}  func {fs}..{fe} --")
            # string refs inside function (push 0x6.....)
            strs = set()
            calls = set()
            for a, line in ins[i - 100:i + 1200]:
                if fend is not None and a >= fend:
                    break
                m = re.search(r"push\s+0x(6[0-9a-f]{5})", line)
                if m:
                    s = strings_at(pe, int(m.group(1), 16))
                    if s:
                        strs.add(s[0] if s else "")
                m = re.search(r"call\s+0x([0-9a-f]+)", line)
                if m:
                    calls.add(int(m.group(1), 16))
            for s in sorted(strs):
                print(f"  str: {s!r}")
            for c in sorted(calls):
                if c in iat:
                    print(f"  call {c:#x} ({iat[c]})")
            # callers of fstart
            if fstart is not None:
                callers = set()
                for a, line in vlist:
                    m = re.search(r"call\s+0x([0-9a-f]+)$", line)
                    if m and int(m.group(1), 16) == fstart:
                        callers.add(a)
                if callers:
                    print(f"  callers: " + ", ".join(f"{c:#x}" for c in sorted(callers)))


if __name__ == "__main__":
    main()
