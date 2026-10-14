"""Reads cm0102.exe: sections, imports and the bytes at an address."""
import hashlib
import struct

EXPECTED_SHA256 = "0c78e7da6d0bcb4603fa179449bbe6d1b6b5e1c627a22c445d14d489bd9bff86"

# The address of the replacement of the k-th import. The runtime writes the same values into the
# import address table, and so does the Unicorn run of the test (tools/research/trace_game.py uses the
# same layout).
IMPORT_STUBS = 0x62000000


class Image:
    def __init__(self, path):
        data = open(path, "rb").read()
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256:
            raise SystemExit(f"{path}: SHA256 {digest} is not the expected cm0102.exe")
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        self.size = struct.unpack_from("<I", data, pe + 24 + 56)[0]
        self.sections = []
        for i in range(count):
            name, vsize, va, rsize, rptr = struct.unpack_from("<8sIIII", data, pe + 24 + optional + i * 40)
            flags = struct.unpack_from("<I", data, pe + 24 + optional + i * 40 + 36)[0]
            self.sections.append((name.rstrip(b"\0").decode(), self.base + va, vsize, rptr, rsize, flags))
        text = self.sections[0]
        self.text = (text[1], text[1] + text[2])
        self.imports = self._imports(pe)       # IAT address -> (dll, name, stub address)

    def _offset(self, addr):
        for _, va, vsize, rptr, rsize, _ in self.sections:
            if va <= addr < va + max(vsize, rsize):
                if addr - va >= rsize:
                    return None
                return rptr + addr - va
        return None

    def read(self, addr, size):
        off = self._offset(addr)
        if off is None:
            return bytes(size)
        return self.data[off:off + size]

    def u32(self, addr):
        return struct.unpack("<I", self.read(addr, 4))[0]

    def u8(self, addr):
        return self.read(addr, 1)[0]

    def is_code(self, addr):
        return self.text[0] <= addr < self.text[1]

    def _imports(self, pe):
        rva = struct.unpack_from("<I", self.data, pe + 24 + 104)[0]
        result = {}
        d = self.base + rva
        k = 0
        while True:
            ilt, _, _, name, iat = struct.unpack("<5I", self.read(d, 20))
            d += 20
            if name == 0:
                break
            dll = self.read(self.base + name, 64).split(b"\0")[0].decode()
            i = 0
            while True:
                entry = self.u32(self.base + (ilt or iat) + 4 * i)
                if entry == 0:
                    break
                if entry & 0x80000000:
                    fn = "%s_ord%d" % (dll.split(".")[0].upper(), entry & 0xFFFF)   # e.g. WSOCK32_ord57
                else:
                    fn = self.read(self.base + entry + 2, 128).split(b"\0")[0].decode()
                result[self.base + iat + 4 * i] = (dll, fn, IMPORT_STUBS + 4 * k)
                i += 1
                k += 1
        return result
