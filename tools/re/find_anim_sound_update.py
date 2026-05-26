import struct
import sys

import pe

CLASS_NAME = b".?AVBGSAnimSoundStateManager@@"


def func_start(tb, tva, instr_rva):
    j = instr_rva - tva
    while j > 0:
        if tb[j - 1] == 0xCC:
            while j < len(tb) and tb[j] == 0xCC:
                j += 1
            return tva + j
        j -= 1
    return None


def count_e8_callers(tb, tva, target_rva, limit=64):
    sites = []
    start = 0
    while True:
        idx = tb.find(b"\xe8", start)
        if idx < 0 or idx + 5 > len(tb):
            break
        start = idx + 1
        rel = struct.unpack_from("<i", tb, idx + 1)[0]
        callee = tva + idx + 5 + rel
        if callee == target_rva:
            sites.append(tva + idx)
            if len(sites) >= limit:
                break
    return sites


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv)
    data = pe_obj.data
    base = pe_obj.imagebase
    tva, tb = pe_obj.text_bytes()

    pos = data.find(CLASS_NAME)
    if pos < 0:
        print("FAIL: RTTI name not found:", CLASS_NAME.decode())
        return
    name_rva = None
    for s in pe_obj.sections:
        if s.rawptr <= pos < s.rawptr + s.rawsize:
            name_rva = s.vaddr + (pos - s.rawptr)
            break
    td_rva = name_rva - 0x10
    print(
        f"RTTI name '{CLASS_NAME.decode()}' fileoff=0x{pos:x} name_rva=0x{name_rva:x} TypeDescriptor_rva=0x{td_rva:x}"
    )

    rdata = pe_obj.section(".rdata")
    rd = data[rdata.rawptr : rdata.rawptr + rdata.rawsize]
    cols = []
    needle = struct.pack("<i", td_rva)
    start = 0
    while True:
        k = rd.find(needle, start)
        if k < 0:
            break
        start = k + 1
        col_off = k - 0x0C
        if col_off < 0:
            continue
        sig = struct.unpack_from("<I", rd, col_off)[0]
        if sig == 1:
            col_rva = rdata.vaddr + col_off
            cols.append(col_rva)
    print(f"COLs referencing TypeDescriptor: {[hex(c) for c in cols]}")

    for col_rva in cols:
        col_va = base + col_rva
        needle_va = struct.pack("<Q", col_va)
        start = 0
        while True:
            k = rd.find(needle_va, start)
            if k < 0:
                break
            start = k + 1
            vtbl_rva = rdata.vaddr + k + 8
            print(f"\n=== vtable for COL 0x{col_rva:x} at RVA 0x{vtbl_rva:x} ===")
            idx = 0
            while True:
                slot_rva = vtbl_rva + idx * 8
                va = pe_obj.qword_at_rva(slot_rva)
                if va is None:
                    break
                fn_rva = va - base
                sec = pe_obj.section_of(fn_rva)
                if sec is None or sec.name != ".text":
                    break
                callers = count_e8_callers(tb, tva, fn_rva)
                callerinfo = ""
                if callers:
                    cs = []
                    for site in callers[:6]:
                        fs = func_start(tb, tva, site)
                        fid = rva2id.get(fs) if fs else None
                        cs.append(
                            f"site=0x{site:x}@func0x{fs:x}"
                            + (f"(id={fid})" if fid else "")
                        )
                    callerinfo = " | callers=" + "; ".join(cs)
                fnid = rva2id.get(fn_rva)
                print(
                    f"  vfunc[{idx:2d}] rva=0x{fn_rva:x}"
                    + (f" id={fnid}" if fnid else "")
                    + f" e8callers={len(callers)}"
                    + callerinfo
                )
                idx += 1
                if idx > 40:
                    break


if __name__ == "__main__":
    main()
