import struct
import sys

import pe


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv[:1] + sys.argv[3:])
    name = (sys.argv[1] if len(sys.argv) > 1 else ".?AVUI@@").encode()
    data = pe_obj.data
    base = pe_obj.imagebase

    pos = data.find(name)
    if pos < 0:
        print(f"FAIL: RTTI name {name!r} not found")
        return
    name_rva = None
    for s in pe_obj.sections:
        if s.rawptr <= pos < s.rawptr + s.rawsize:
            name_rva = s.vaddr + (pos - s.rawptr)
            break
    td_rva = name_rva - 0x10
    print(
        f"RTTI {name.decode()} name_rva=0x{name_rva:x} TypeDescriptor_rva=0x{td_rva:x}"
    )

    rdata = pe_obj.section(".rdata")
    rd = data[rdata.rawptr : rdata.rawptr + rdata.rawsize]
    needle = struct.pack("<i", td_rva)
    cols = []
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
        base_offset = struct.unpack_from("<I", rd, col_off + 0x04)[0]
        if sig == 1:
            cols.append((rdata.vaddr + col_off, base_offset))
    print(f"COLs: {[(hex(c), off) for c, off in cols]}")

    for col_rva, base_offset in cols:
        col_va = base + col_rva
        needle_va = struct.pack("<Q", col_va)
        start = 0
        while True:
            k = rd.find(needle_va, start)
            if k < 0:
                break
            start = k + 1
            vtbl_rva = rdata.vaddr + k + 8
            tag = " <<< PRIMARY vtable (subobject offset 0)" if base_offset == 0 else ""
            idv = rva2id.get(vtbl_rva)
            print(
                f"  vtable_rva=0x{vtbl_rva:x} subobj_offset={base_offset}"
                + (f" ID={idv}" if idv else " ID=<none>")
                + tag
            )


if __name__ == "__main__":
    main()
