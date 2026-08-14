#!/usr/bin/env python3
"""Dump resources from the Xgpro reference PE to a readable format.

Parses dialogs, menus, string tables, version info and extracts bitmaps/icons
from the reference Windows executable. The output guides the Qt6 port.

Usage:
    dump_resources.py <xgpro.exe> <outdir> [--dialogs-json]

Requires: pefile (pip install pefile)
"""
import argparse
import json
import os
import struct
import sys

try:
    import pefile
except ImportError:
    sys.exit("pefile is required: pip install pefile")


ALIGN = 4


def align(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


def read_u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def read_u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def read_s16(data, off):
    return struct.unpack_from("<h", data, off)[0]


def parse_sz_or_ord(data, off, aligned=False):
    """Reads an aligned szOrOrd: 0xFFFF means ordinal follows."""
    v = read_u16(data, off)
    if v == 0xFFFF:
        ord_ = read_u16(data, off + 2)
        return {"type": "ordinal", "value": ord_}, off + 4
    end = find_wstr_end(data, off)
    s = data[off:end].decode("utf-16-le", errors="replace")
    nxt = end + 2
    if aligned:
        nxt = align(nxt)
    return {"type": "string", "value": s}, nxt


def find_wstr_end(data, start):
    """Find the null terminator of a UTF-16LE string, honouring 2-byte alignment."""
    start &= ~1
    for i in range(start, len(data) - 1, 2):
        if data[i] == 0 and data[i + 1] == 0:
            return i
    return len(data)


def read_u8(data, off):
    return data[off]


def read_wstr(data, p, aligned=True):
    """Read a null-terminated UTF-16LE string.

    `aligned=True` (dialogs, version blocks) pads to a DWORD boundary; menu
    item strings follow each other with no padding.
    """
    end = find_wstr_end(data, p)
    s = data[p:end].decode("utf-16-le", errors="replace")
    nxt = end + 2
    if aligned:
        nxt = align(nxt)
    return s, nxt


def parse_dlg_template(data, off=0, verbose=False):
    """Parse a Win32 dialog template (both DLGTEMPLATEEX and DLGTEMPLATE)."""
    dlg_ver, signature = read_u16(data, off), read_u16(data, off + 2)
    extended = dlg_ver == 1 and signature == 0xFFFF
    dlg = {"dlg_ver": dlg_ver, "signature": signature, "extended": extended}
    # For extended templates the first DWORD is dlgVer+signature; for classic
    # DLGTEMPLATE those 4 bytes ARE the style DWORD, so start right there.
    p = off + 4 if extended else off
    if extended:
        dlg["help_id"] = read_u32(data, p); p += 4
        dlg["ex_style"] = read_u32(data, p); p += 4
        dlg["style"] = read_u32(data, p); p += 4
    else:
        dlg["style"] = read_u32(data, p); p += 4
        dlg["ex_style"] = read_u32(data, p); p += 4
        dlg["help_id"] = 0
    dlg["c_items"] = read_u16(data, p); p += 2
    dlg["x"], dlg["y"] = read_s16(data, p), read_s16(data, p + 2)
    dlg["cx"], dlg["cy"] = read_s16(data, p + 4), read_s16(data, p + 6)
    p += 8
    dlg["menu"], p = parse_sz_or_ord(data, p)
    dlg["class"], p = parse_sz_or_ord(data, p)
    dlg["title"], p = parse_sz_or_ord(data, p)
    dlg["items"] = []
    if dlg["style"] & 0x40:  # DS_SETFONT
        ptsize = read_u16(data, p); p += 2
        dlg["font_points"] = ptsize
        if extended:
            # DLGTEMPLATEEX font block: weight(W), italic(B), charset(B), typeface(W[])
            dlg["weight"] = read_u16(data, p); p += 2
            dlg["italic"] = read_u8(data, p); p += 1
            dlg["charset"] = read_u8(data, p); p += 1
        else:
            # DLGTEMPLATE font block: pointSize(W), then (non-standard) typeface
            dlg["weight"] = 0
            dlg["italic"] = 0
            dlg["charset"] = 0
        dlg["font_face"], p = read_wstr(data, p)
    for _ in range(dlg["c_items"]):
        item, p = parse_dlg_item(data, p, extended)
        dlg["items"].append(item)
    return dlg, p


def parse_dlg_item(data, p, extended=True, align_strs=False):
    """Parse a dialog item template (extended or classic).

    Class/title sz_Or_Ord strings follow each other without padding; only the
    item as a whole is padded to a DWORD boundary before the next item.
    """
    if extended:
        help_id = read_u32(data, p); p += 4
        ex_style = read_u32(data, p); p += 4
        style = read_u32(data, p); p += 4
    else:
        style = read_u32(data, p); p += 4
        ex_style = read_u32(data, p); p += 4
        help_id = 0
    x, y, cx, cy = read_s16(data, p), read_s16(data, p + 2), read_s16(data, p + 4), read_s16(data, p + 6)
    p += 8
    if extended:
        ctrl_id = read_u32(data, p); p += 4
    else:
        ctrl_id = read_u16(data, p); p += 2
    cls, p = parse_sz_or_ord(data, p, aligned=align_strs)
    text, p = parse_sz_or_ord(data, p, aligned=align_strs)
    cb_extra = read_u16(data, p); p += 2
    p = align(p)
    extra = data[p:p + cb_extra]
    p += cb_extra
    p = align(p)
    return {
        "help_id": help_id,
        "ex_style": ex_style,
        "style": style,
        "x": x, "y": y, "cx": cx, "cy": cy,
        "ctrl_id": ctrl_id,
        "class": cls,
        "text": text,
        "cb_extra": cb_extra,
        "extra_hex": extra.hex(),
    }, p


# Control class atom mapping
ATOM_TO_CLASS = {
    0x0080: "BUTTON",
    0x0081: "EDIT",
    0x0082: "STATIC",
    0x0083: "LISTBOX",
    0x0084: "SCROLLBAR",
    0x0085: "COMBOBOX",
}


def dlg_to_rc(d):
    """Render a parsed dialog template back into a .rc-style listing."""
    def fmt_style(s, kind):
        out = []
        if kind == "dlg":
            for bit, name in [(0x00000004, "WS_CHILD"), (0x00000010, "WS_VISIBLE"),
                              (0x00020000, "WS_MINIMIZEBOX"), (0x00040000, "WS_MAXIMIZEBOX"),
                              (0x00800000, "DS_MODALFRAME"), (0x00400000, "DS_SETFONT"),
                              (0x00000001, "DS_ABSALIGN"), (0x00100000, "DS_CONTEXTHELP"),
                              (0x00000002, "DS_SYSMODAL"), (0x00000008, "DS_SETFOREGROUND")]:
                if s & bit:
                    out.append(name)
        else:
            for bit, name in [(0x00000004, "WS_CHILD"), (0x00000010, "WS_VISIBLE"),
                              (0x00000001, "WS_GROUP"), (0x00000002, "WS_TABSTOP"),
                              (0x00001000, "SS_OWNERDRAW"), (0x00000002, "SS_LEFT")]:
                pass
        return " | ".join(out) if out else "0x%08X" % s

    lines = [f"DLG {d.get('title', {}).get('value', '?')}  id=?"]
    lines.append(f"  style={hex(d['style'])} ex_style={hex(d['ex_style'])} pos=({d['x']},{d['y']},{d['cx']},{d['cy']}) items={d['c_items']}")
    lines.append(f"  font={d.get('font_face', '')} {d.get('font_points', '')}pt weight={d.get('weight', '')}")
    for it in d["items"]:
        cls = it["class"]
        if cls.get("type") == "ordinal":
            cname = ATOM_TO_CLASS.get(cls["value"], f"ord{cls['value']}")
        else:
            cname = cls.get("value", "?")
        txt = it["text"]
        txts = txt.get("value", "") if txt.get("type") == "string" else f"id={txt.get('value')}"
        lines.append(
            f"    id={it['ctrl_id']:8d} {cname:10s} ({it['x']:4d},{it['y']:4d},{it['cx']:4d},{it['cy']:4d}) "
            f"style=0x{it['style']:08x} text={txts!r}"
        )
    return "\n".join(lines)


def walk_resources(pe, rva):
    """Yield (name, data) for every leaf resource, names as path tuples."""
    data = pe.__data__
    for e0 in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        rt_name = e0.name if e0.name else e0.id
        entries = e0
        for e1 in entries.directory.entries:
            n1 = e1.name if e1.name else e1.id
            for e2 in e1.directory.entries:
                n2 = e2.name if e2.name else e2.id
                leaf = e2.data.struct
                off = leaf.OffsetToData
                size = leaf.Size
                rva_off = pe.get_offset_from_rva(off)
                yield (rt_name, n1, n2), data[rva_off:rva_off + size]


def parse_string_table(data):
    out = {}
    for i in range(16):
        off = i * 18
        n = read_u16(data, off)
        if n == 0:
            continue
        base = 16 * 16
        for j in range(n):
            c = read_u16(data, base + j * 2)
            if c != 0xFFFF:
                out[base // 16 + j] = c
    # not directly usable for multibyte strings; return raw
    return out


def dump_string_table(data):
    """Decode an RT_STRING block into {string_id: text}.

    A block holds 16 strings: a 16-entry WORD length table, then the UTF-16
    data packed in order (zero-length entries consume no space).
    """
    result = {}
    p = 16 * 2
    for j in range(16):
        n = read_u16(data, j * 2)
        if n == 0:
            continue
        s = data[p:p + n * 2].decode("utf-16-le", errors="replace")
        p += n * 2
        result[j] = s
    return result


def parse_menu(data):
    """Parse an RT_MENU (classic MENUITEMTEMPLATE format) into nested items.

    Layout follows MENU_ParseResource in Wine user32/menu.c:
      - WORD versionNumber, WORD offset; items start at 4 + offset
      - item: WORD flags; if MF_POPUP (0x10): WCHAR[] text then submenu items,
        else WORD id then WCHAR[] text
      - strings are NOT padded; the next field follows the null terminator
      - every level ends with an item carrying MF_END (0x80); that flag is
        stripped from the returned flags
    """
    if len(data) < 4:
        return []
    off = 4 + read_u16(data, 2)

    def read_menu_items(p):
        items = []
        while True:
            flags = read_u16(data, p); p += 2
            end_flag = bool(flags & 0x80)
            flags &= ~0x80
            item = {"flags": flags,
                    "popup": bool(flags & 0x10),
                    "grayed": bool(flags & 0x0001),
                    "disabled": bool(flags & 0x0002),
                    "checked": bool(flags & 0x0008),
                    "menubarbreak": bool(flags & 0x0020),
                    "menubreak": bool(flags & 0x0040),
                    "help": bool(flags & 0x4000),
                    "end": end_flag}
            if flags & 0x10:
                item["text"], p = read_wstr(data, p, aligned=False)
                item["items"], p = read_menu_items(p)
            else:
                item["id"] = read_u16(data, p); p += 2
                item["text"], p = read_wstr(data, p, aligned=False)
            items.append(item)
            if end_flag:
                break
        return items, p

    return read_menu_items(off)[0]


def parse_version(data):
    """Parse a VS_VERSION_INFO resource tree into a flat dict."""
    info = {}
    if read_u16(data, 0) == 0:
        return info

    def key_base(p):
        """Return (key, base_of_value) for a version block at p."""
        key_end = find_wstr_end(data, p + 6)
        key = data[p + 6:key_end].decode("utf-16-le", errors="replace")
        while key_end + 2 < len(data) and data[key_end + 2] == 0 and data[key_end + 3] == 0:
            key_end += 2
        return key, align(key_end + 2)

    p = 0
    wlen = read_u16(data, p)
    wval = read_u16(data, p + 2)
    wtype = read_u16(data, p + 4)
    key, base = key_base(p)
    if key == "VS_VERSION_INFO" and wval >= 13:
        # VS_FIXEDFILEINFO is 52 bytes (13 DWORDs)
        f = data[base:base + 52]
        fi = struct.unpack_from("<IIHHHHIIIIIIIII", f)
        info["fixed"] = {
            "signature": "0x%08X" % fi[0],
            "struct_version": "0x%X" % fi[1],
            "file_version": "%d.%d.%d.%d" % (fi[2] >> 16, fi[2] & 0xFFFF, fi[3] >> 16, fi[3] & 0xFFFF),
            "product_version": "%d.%d.%d.%d" % (fi[4] >> 16, fi[4] & 0xFFFF, fi[5] >> 16, fi[5] & 0xFFFF),
            "file_flags_mask": "0x%08X" % fi[6],
            "file_flags": "0x%08X" % fi[7],
            "file_os": "0x%08X" % fi[8],
            "file_type": "0x%08X" % fi[9],
            "file_subtype": "0x%08X" % fi[10],
            "file_date": "0x%08X%08X" % (fi[11], fi[12]),
        }

    def walk_chain(p, end):
        while p < end:
            wlen = read_u16(data, p)
            if wlen < 6:
                break
            wval = read_u16(data, p + 2)
            wtype = read_u16(data, p + 4)
            if p + 6 >= len(data):
                break
            key, base = key_base(p)
            child = align(base + wval * 2)
            if wval == 0:
                walk_chain(child, min(p + wlen, len(data)))
            elif base + wval * 2 <= len(data):
                val = data[base:base + wval * 2]
                if wtype == 1:
                    info[key] = val.decode("utf-16-le", errors="replace").rstrip("\x00")
                else:
                    info[key] = ["%04X" % x for x in struct.unpack("<%dH" % wval, val)]
            p = align(p + wlen)

    walk_chain(base + 52, p + wlen)  # root value is VS_FIXEDFILEINFO (52 bytes)
    return info


def main():
    ap = argparse.ArgumentParser(description="Dump resources from the Xgpro PE")
    ap.add_argument("exe", help="path to Xgpro.exe")
    ap.add_argument("outdir", help="output directory")
    ap.add_argument("--dialogs-json", action="store_true", help="also emit a JSON of every dialog")
    args = ap.parse_args()

    pe = pefile.PE(args.exe)
    os.makedirs(args.outdir, exist_ok=True)

    dialogs = {}
    menus = {}
    strings = {}
    RT_NAMES = {1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG", 6: "STRING",
                9: "ACCELERATOR", 12: "GROUP_CURSOR", 14: "GROUP_ICON", 16: "VERSION",
                24: "MANIFEST", 241: "TOOLBAR"}
    for (rt, n1, n2), blob in walk_resources(pe, 0):
        rt = RT_NAMES.get(rt, rt)
        rt_s = str(rt)
        n1_s = str(n1)
        if rt == "DIALOG":
            try:
                d, _ = parse_dlg_template(blob)
                dialogs[n1] = d
                print(f"--- DIALOG {n1} ({len(blob)} bytes) ---")
                print(dlg_to_rc(d))
                print()
            except Exception as e:
                print(f"--- DIALOG {n1}: parse error {e}")
        elif rt == "MENU":
            try:
                menus[n1] = parse_menu(blob)
                print(f"--- MENU {n1} ---")
                print(json.dumps(menus[n1], ensure_ascii=False, indent=1))
                print()
            except Exception as e:
                print(f"--- MENU {n1}: parse error {e}")
        elif rt == "STRING":
            strings[n1] = dump_string_table(blob)
        elif rt == "VERSION":
            try:
                print(f"--- VERSION {n1} ---")
                print(parse_version(blob))
                print()
            except Exception as e:
                print("version parse error", e)
        elif rt == "BITMAP":
            path = os.path.join(args.outdir, f"bitmap_{n1}.bmp")
            with open(path, "wb") as f:
                f.write(blob)
            print(f"bitmap {n1} -> {os.path.basename(path)}")
        elif rt == "ICON":
            path = os.path.join(args.outdir, f"icon_{n1}.ico")
            with open(path, "wb") as f:
                f.write(blob)
            print(f"icon {n1} -> {os.path.basename(path)}")
        elif rt == "CURSOR":
            path = os.path.join(args.outdir, f"cursor_{n1}.cur")
            with open(path, "wb") as f:
                f.write(blob)
        elif rt == "TOOLBAR":
            print(f"toolbar {n1} {blob.hex()}")

    # write the string table
    with open(os.path.join(args.outdir, "strings.json"), "w", encoding="utf-8") as f:
        json.dump({str(k): v for k, v in strings.items()}, f, ensure_ascii=False, indent=1)

    # write the menus (128 = main menu, 158 = toolbar/popup)
    with open(os.path.join(args.outdir, "menus.json"), "w", encoding="utf-8") as f:
        json.dump({str(k): v for k, v in menus.items()}, f, ensure_ascii=False, indent=1)

    if args.dialogs_json:
        with open(os.path.join(args.outdir, "dialogs.json"), "w", encoding="utf-8") as f:
            json.dump({str(k): v for k, v in dialogs.items()}, f, ensure_ascii=False, indent=1)


if __name__ == "__main__":
    main()
