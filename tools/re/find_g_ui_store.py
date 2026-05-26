import struct
import sys

import pe

UI_VTABLE_RVA = 0x27159D8


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv)
    tva, tb = pe_obj.text_bytes()
    n = len(tb)

    def func_start_off(off):
        j = off
        while j > 0:
            if tb[j - 1] == 0xCC:
                while j < n and tb[j] == 0xCC:
                    j += 1
                return j
            j -= 1
        return 0

    ctor_funcs = set()
    i = 0
    while i < n - 7:
        if tb[i] == 0x48 and tb[i + 1] == 0x8D and tb[i + 2] in pe.RIP_MODRM:
            disp = struct.unpack_from("<i", tb, i + 3)[0]
            if tva + i + 7 + disp == UI_VTABLE_RVA:
                ctor_funcs.add(func_start_off(i))
        i += 1
    print(
        f"functions referencing UI vtable (lea): {[hex(tva + f) for f in sorted(ctor_funcs)]}"
    )

    found = {}
    for fs in sorted(ctor_funcs):
        end = fs
        while end < n - 1 and not (tb[end] == 0xCC and tb[end + 1] == 0xCC):
            end += 1
        j = fs
        while j < end - 6:
            if tb[j] == 0x48 and tb[j + 1] == 0x89 and tb[j + 2] in pe.RIP_MODRM:
                disp = struct.unpack_from("<i", tb, j + 3)[0]
                g = tva + j + 7 + disp
                sec = pe_obj.section_of(g)
                if sec and sec.name == ".data":
                    found.setdefault(g, 0)
                    found[g] += 1
            j += 1

    print("g_UI global candidates (written in a UI-vtable-referencing function):")
    for g, cnt in sorted(found.items(), key=lambda kv: -kv[1]):
        idv = rva2id.get(g)
        print(f"  RVA 0x{g:x} writes={cnt}" + (f" ID={idv}" if idv else " ID=<none>"))


if __name__ == "__main__":
    main()
