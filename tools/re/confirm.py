import struct
import sys
from collections import Counter

import pe

MENUMAP_OFFSET = 0x1A8
RWLOCK_ID = 2707105


def find_rip_loads(tb, tva, target_rva):
    sites = []
    i = 0
    n = len(tb)
    while i < n - 7:
        if tb[i] == 0x48 and tb[i + 1] == 0x8B and tb[i + 2] in pe.RIP_MODRM:
            disp = struct.unpack_from("<i", tb, i + 3)[0]
            if tva + i + 7 + disp == target_rva:
                sites.append((i, pe.RIP_MODRM[tb[i + 2]]))
        i += 1
    return sites


def confirm_ui(pe_obj, id2rva):
    tva, tb = pe_obj.text_bytes()
    rwlock_rva = id2rva.get(RWLOCK_ID)
    print(
        f"RW-lock global (id {RWLOCK_ID}) RVA = {'0x%x' % rwlock_rva if rwlock_rva else 'MISS'}"
    )
    if not rwlock_rva:
        return

    rwlock_disp_sites = []
    i = 0
    n = len(tb)
    while i < n - 7:
        b0, b1, b2 = tb[i], tb[i + 1], tb[i + 2]
        if b0 == 0x48 and b1 in (0x8B, 0x8D) and b2 in pe.RIP_MODRM:
            disp = struct.unpack_from("<i", tb, i + 3)[0]
            if tva + i + 7 + disp == rwlock_rva:
                rwlock_disp_sites.append(i)
        i += 1
    print(f"RW-lock referenced at {len(rwlock_disp_sites)} code site(s)")

    g_ui = Counter()
    disp_1a8 = struct.pack("<i", MENUMAP_OFFSET)
    WINDOW = 0x120
    for site in rwlock_disp_sites:
        lo = max(0, site - WINDOW)
        hi = min(n - 7, site + WINDOW)
        j = lo
        while j < hi:
            if tb[j] == 0x48 and tb[j + 1] == 0x8B and tb[j + 2] in pe.RIP_MODRM:
                reg = pe.RIP_MODRM[tb[j + 2]]
                disp = struct.unpack_from("<i", tb, j + 3)[0]
                gtarget = tva + j + 7 + disp
                w = tb[j + 7 : j + 7 + 96]
                widx = w.find(disp_1a8)
                if (
                    widx >= 1
                    and (w[widx - 1] & 0xC0) == 0x80
                    and (w[widx - 1] & 0x07) == reg
                ):
                    g_ui[gtarget] += 1
            j += 1

    print("\ng_UI globals appearing in RW-lock-guarded menuMap access (DEFINITIVE):")
    for rva, cnt in g_ui.most_common(8):
        idv = id2rva and None
        sec = pe_obj.section_of(rva)
        idv = None
        for k, v in id2rva.items():
            if v == rva:
                idv = k
                break
        print(
            f"  RVA 0x{rva:x} cooccur={cnt} sec={sec.name if sec else '?'}"
            + (f" ID={idv}" if idv else " ID=<none>")
        )


def search_rtti_names(pe_obj, needles):
    data = pe_obj.data
    print("\nRTTI / string search for audio-tick class candidates:")
    for needle in needles:
        nb = needle.encode()
        pos = 0
        found = []
        while True:
            k = data.find(nb, pos)
            if k < 0:
                break
            pos = k + 1
            ctx = data[max(0, k - 4) : k + len(nb) + 4]
            found.append((k, ctx))
            if len(found) >= 4:
                break
        if found:
            for k, ctx in found:
                print(f"  '{needle}' @fileoff 0x{k:x}  ctx={ctx!r}")
        else:
            print(f"  '{needle}'  NOT FOUND")


def main():
    pe_obj, (id2rva, rva2id) = pe.open_targets(sys.argv)
    confirm_ui(pe_obj, id2rva)
    search_rtti_names(
        pe_obj,
        [
            "BGSAnimSoundStateManager",
            "AnimSoundStateManager",
            "AnimationSound",
            ".?AVBSAnimationGraphChannel",
            "ProcessManager",
            "AudioManager",
            "BSAudioManager",
        ],
    )


if __name__ == "__main__":
    main()
