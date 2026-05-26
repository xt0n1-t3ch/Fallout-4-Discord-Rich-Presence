import struct
import sys
from collections import defaultdict

import pe

UI_OFFSETS = {
    0x178: "releasedMovies",
    0x190: "menuStack",
    0x1A8: "menuMap",
    0x1E0: "menuMode",
}


def scan(pe_obj, rva2id):
    tva, tb = pe_obj.text_bytes()
    n = len(tb)

    offsets_seen = defaultdict(set)
    hits = defaultdict(lambda: defaultdict(int))

    disp_bytes = {off: struct.pack("<i", off) for off in UI_OFFSETS}

    i = 0
    while i < n - 7:
        if tb[i] == 0x48 and tb[i + 1] == 0x8B and tb[i + 2] in pe.RIP_MODRM:
            reg = pe.RIP_MODRM[tb[i + 2]]
            disp = struct.unpack_from("<i", tb, i + 3)[0]
            target = tva + i + 7 + disp
            window = tb[i + 7 : i + 7 + 64]
            for off, db in disp_bytes.items():
                widx = window.find(db)
                if (
                    widx >= 1
                    and (window[widx - 1] & 0xC0) == 0x80
                    and (window[widx - 1] & 0x07) == reg
                ):
                    offsets_seen[target].add(off)
                    hits[target][off] += 1
        i += 1

    return offsets_seen, hits


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv)
    offsets_seen, hits = scan(pe_obj, rva2id)

    ranked = sorted(
        offsets_seen.items(),
        key=lambda kv: (len(kv[1]), sum(hits[kv[0]].values())),
        reverse=True,
    )

    print("g_UI candidates ranked by distinct UI member offsets matched:")
    print(
        "(the true UI** global is read then accessed at multiple of releasedMovies/menuStack/menuMap/menuMode)"
    )
    for rva, offs in ranked[:15]:
        sec = pe_obj.section_of(rva)
        secname = sec.name if sec else "?"
        writable = sec.writable if sec else False
        static = pe_obj.qword_at_rva(rva)
        staticstr = "uninit/0" if (static is None or static == 0) else f"0x{static:x}"
        idv = rva2id.get(rva)
        offlabels = ",".join(f"{UI_OFFSETS[o]}({hits[rva][o]})" for o in sorted(offs))
        marker = "   <<< STRONG g_UI" if len(offs) >= 3 and writable else ""
        print(
            f"  RVA 0x{rva:x} distinct={len(offs)} sec={secname} W={int(writable)} static={staticstr}"
            + (f" ID={idv}" if idv else " ID=<none>")
            + f"  [{offlabels}]"
            + marker
        )


if __name__ == "__main__":
    main()
