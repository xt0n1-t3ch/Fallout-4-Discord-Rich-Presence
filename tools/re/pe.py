import struct

DEFAULT_EXE = r"C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Fallout4.exe"
DEFAULT_BIN = r"C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\F4SE\Plugins\version-1-11-191-0.bin"

IMAGE_SCN_MEM_WRITE = 0x80000000
IMAGE_SCN_MEM_EXECUTE = 0x20000000


class Section:
    __slots__ = ("name", "vaddr", "vsize", "rawsize", "rawptr", "chars")

    def __init__(self, name, vaddr, vsize, rawsize, rawptr, chars):
        self.name = name
        self.vaddr = vaddr
        self.vsize = vsize
        self.rawsize = rawsize
        self.rawptr = rawptr
        self.chars = chars

    @property
    def writable(self):
        return bool(self.chars & IMAGE_SCN_MEM_WRITE)

    @property
    def executable(self):
        return bool(self.chars & IMAGE_SCN_MEM_EXECUTE)

    def contains(self, rva):
        return self.vaddr <= rva < self.vaddr + max(self.vsize, self.rawsize)


class PE:
    def __init__(self, path):
        self.path = path
        self.data = open(path, "rb").read()
        e_lfanew = struct.unpack_from("<I", self.data, 0x3C)[0]
        assert self.data[e_lfanew : e_lfanew + 4] == b"PE\x00\x00", "not a PE"
        coff = e_lfanew + 4
        num_sections = struct.unpack_from("<H", self.data, coff + 2)[0]
        opt_size = struct.unpack_from("<H", self.data, coff + 16)[0]
        opt = coff + 20
        self.imagebase = struct.unpack_from("<Q", self.data, opt + 24)[0]
        sec = opt + opt_size
        self.sections = []
        for i in range(num_sections):
            o = sec + i * 40
            name = self.data[o : o + 8].rstrip(b"\x00").decode("latin1")
            vsize = struct.unpack_from("<I", self.data, o + 8)[0]
            vaddr = struct.unpack_from("<I", self.data, o + 12)[0]
            rawsize = struct.unpack_from("<I", self.data, o + 16)[0]
            rawptr = struct.unpack_from("<I", self.data, o + 20)[0]
            chars = struct.unpack_from("<I", self.data, o + 36)[0]
            self.sections.append(Section(name, vaddr, vsize, rawsize, rawptr, chars))

    def section(self, name):
        for s in self.sections:
            if s.name == name:
                return s
        return None

    def section_of(self, rva):
        for s in self.sections:
            if s.contains(rva):
                return s
        return None

    def text_bytes(self):
        s = self.section(".text")
        return s.vaddr, self.data[s.rawptr : s.rawptr + s.rawsize]

    def rva_to_off(self, rva):
        s = self.section_of(rva)
        if s is None:
            return None
        off = rva - s.vaddr
        if off >= s.rawsize:
            return None
        return s.rawptr + off

    def qword_at_rva(self, rva):
        off = self.rva_to_off(rva)
        if off is None or off + 8 > len(self.data):
            return None
        return struct.unpack_from("<Q", self.data, off)[0]

    def dword_at_rva(self, rva):
        off = self.rva_to_off(rva)
        if off is None or off + 4 > len(self.data):
            return None
        return struct.unpack_from("<I", self.data, off)[0]


def load_addrlib(path):
    bd = open(path, "rb").read()
    count = struct.unpack_from("<Q", bd, 0)[0]
    if 8 + count * 16 != len(bd):
        raise ValueError(
            "addrlib size mismatch: header count=%d expects %d bytes, file is %d"
            % (count, 8 + count * 16, len(bd))
        )
    id2rva = {}
    rva2id = {}
    o = 8
    for _ in range(count):
        idv = struct.unpack_from("<Q", bd, o)[0]
        rv = struct.unpack_from("<Q", bd, o + 8)[0]
        o += 16
        id2rva[idv] = rv
        rva2id[rv] = idv
    return id2rva, rva2id


RIP_MODRM = {0x05: 0, 0x0D: 1, 0x15: 2, 0x1D: 3, 0x25: 4, 0x2D: 5, 0x35: 6, 0x3D: 7}
REG_NAMES = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi"]


def open_targets(argv):
    exe = DEFAULT_EXE
    binp = DEFAULT_BIN
    if len(argv) > 1 and argv[1]:
        exe = argv[1]
    if len(argv) > 2 and argv[2]:
        binp = argv[2]
    return PE(exe), load_addrlib(binp)


def self_check(pe, id2rva):
    expect = {2228917: 0xC334B0, 2698043: 0x32D2020}
    ok = True
    for idv, want in expect.items():
        got = id2rva.get(idv)
        flag = "OK" if got == want else "MISMATCH"
        if got != want:
            ok = False
        print(
            f"  self-check id={idv} rva={('0x%x' % got) if got else 'MISS'} want=0x{want:x} [{flag}]"
        )
    print(f"  imagebase=0x{pe.imagebase:x} addrlib parse {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    import sys

    pe, (id2rva, rva2id) = open_targets(sys.argv)
    print(f"PE {pe.path}")
    for s in pe.sections:
        print(
            f"  {s.name:8s} vaddr=0x{s.vaddr:08x} vsize=0x{s.vsize:08x} raw=0x{s.rawsize:08x} "
            f"W={int(s.writable)} X={int(s.executable)}"
        )
    print(f"addrlib entries={len(id2rva)}")
    self_check(pe, id2rva)
