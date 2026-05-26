import struct
import sys
from collections import Counter

import pe

RWLOCK_ID = 2707105
MENUMAP_OFF = 0x1A8


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv)
    tva, tb = pe_obj.text_bytes()
    n = len(tb)
    rwlock_rva = id2rva[RWLOCK_ID]
    print(f"RW-lock rva=0x{rwlock_rva:x}")

    def func_start_off(instr_off):
        j = instr_off
        while j > 0:
            if tb[j - 1] == 0xCC:
                while j < n and tb[j] == 0xCC:
                    j += 1
                return j
            j -= 1
        return 0

    rwlock_funcs = set()
    i = 0
    while i < n - 7:
        if tb[i] == 0x48 and tb[i + 1] in (0x8B, 0x8D) and tb[i + 2] in pe.RIP_MODRM:
            disp = struct.unpack_from("<i", tb, i + 3)[0]
            if tva + i + 7 + disp == rwlock_rva:
                rwlock_funcs.add(func_start_off(i))
        i += 1

    getmenu_funcs = [tva + fs for fs in sorted(rwlock_funcs)]
    func_set = set(getmenu_funcs)
    print(
        f"RW-lock-referencing funcs (UI menu code): {len(getmenu_funcs)} | members used: {len(func_set)}"
    )

    mov_tally = Counter()
    lea_tally = Counter()
    start = 0
    while True:
        ci = tb.find(b"\xe8", start)
        if ci < 0 or ci + 5 > n:
            break
        start = ci + 1
        rel = struct.unpack_from("<i", tb, ci + 1)[0]
        if tva + ci + 5 + rel not in func_set:
            continue
        lo = max(0, ci - 0x48)
        j = lo
        while j < ci - 6:
            if tb[j] == 0x48 and tb[j + 2] == 0x0D and tb[j + 1] in (0x8B, 0x8D):
                d = struct.unpack_from("<i", tb, j + 3)[0]
                gva = tva + j + 7 + d
                if tb[j + 1] == 0x8B:
                    mov_tally[gva] += 1
                else:
                    lea_tally[gva] += 1
            j += 1

    def show(title, tally):
        print(title)
        for rva, cnt in tally.most_common(8):
            sec = pe_obj.section_of(rva)
            idv = rva2id.get(rva)
            static = pe_obj.qword_at_rva(rva)
            sv = "uninit/0" if (static is None or static == 0) else f"0x{static:x}"
            print(
                f"  RVA 0x{rva:x} cnt={cnt} sec={sec.name if sec else '?'} static={sv}"
                + (f" ID={idv}" if idv else " ID=<none>")
            )

    show("g_UI as UI** (mov rcx,[rip+G] before GetMenu) -> HEAP singleton:", mov_tally)
    show(
        "g_UI as embedded object (lea rcx,[rip+G] before GetMenu) -> SDM in-module:",
        lea_tally,
    )


if __name__ == "__main__":
    main()
