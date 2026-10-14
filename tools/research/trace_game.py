"""Runs the database load and game creation of the original in Unicorn and records the functions it runs.

Research only. Usage: ~/.venvs/cm0102/bin/python -I trace.py <exe> <data dir> <out prefix>

It needs cg.pkl, a pickled call graph of the original (procedures, callers, callees) that the research
agent dumped from REA. That file is not in the repo: dump it again from REA before a run, and change the
path below. See docs/research/2026-10-14-game-creation-inventory.md for what the runs found.
"""
import bisect
import json
import os
import pickle
import re
import struct
import sys
import time

sys.path.insert(0, "/Users/max/Downloads/Game/cm0102-mac/tests/diff")
import original as O  # noqa: E402

O.HEAP_BASE = 0x70000000
O.HEAP_SIZE = 0x40000000
from unicorn import UC_HOOK_BLOCK, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_PROT_ALL  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_ECX  # noqa: E402

EXE, DATA, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
IMPSTUBS = 0x62000000


class Tracer(O.Original):
    def __init__(self):
        super().__init__(EXE, DATA)
        self.sizes = {}
        self.blocks = set()
        self.progress = []
        self.unknown = None
        self.log = []
        self._imports()
        uc = self.uc
        # CRT and helpers
        self._replace(0x945667, 2, self._realloc)
        self._replace(0x946a22, 3, self._fseek)
        self._replace(0x9469e0, 1, self._fgetc)
        self._replace(0x945ce6, 4, self._fwrite)
        self._replace(0x946aae, 1, lambda st: self.files[st].tell())
        self._replace(0x9478d8, 2, self._access)
        self.where = ("start", 0)
        self.first = {}
        self._replace(0x945b13, 5, self._makepath)
        self._replace(0x4c0aa0, 3, self._findfirst)
        self._replace(0x4c0b50, 1, lambda f: 1)
        self._replace(0x4c0c10, 1, lambda f: 0)
        self._replace(0x90cc50, 2, lambda a, b: 1)
        self._replace(0x946ebb, 1, lambda p: 1000000000)
        self._replace(0x947299, 0, lambda: 12345)
        self._replace(0x7a1710, 0, lambda: 1000)
        self._replace(0x7a1820, 1, lambda a: 1000)
        self._replace_ret(0x802f80, 4, self._progress)       # thiscall, 4 args
        self._replace_ret(0x5d8020, 1, lambda *a: 0)
        self._replace_ret(0x5d8480, 0, lambda *a: 0)
        def where(uc, address, size, ud):
            sp = uc.reg_read(UC_X86_REG_ESP)
            ret, a1, a2, line = struct.unpack("<4I", self.read(sp, 16))
            self.log.append(("errorplace", hex(ret), line))
            print("ERROR at", hex(ret), "line", line, flush=True)
        uc.hook_add(UC_HOOK_CODE, where, begin=0x90D130, end=0x90D130)
        procs, _, _ = pickle.load(open("/private/tmp/claude-501/-Users-max-Downloads-Game/c42489a6-676d-4d69-8ee2-5f658f2640c4/scratchpad/gc/cg.pkl", "rb"))
        self.hooks = {}
        for p in procs:
            if 0x401000 <= p < 0x944000 and p not in (0x7a1710, 0x7a1820, 0x802f80, 0x5d8020, 0x5d8480, 0x4c0aa0, 0x4c0b50, 0x4c0c10, 0x90cc50, 0x90bde0, 0x7fee80, 0x90d130, 0x5e8290):
                self.hooks[p] = uc.hook_add(UC_HOOK_CODE, self._on_entry, begin=p, end=p)

    # helpers
    def alloc(self, size):
        a = super().alloc(max(size, 1))
        if hasattr(self, "sizes"):
            self.sizes[a] = size
        return a

    def alloc_sized(self, size):
        return self.alloc(size)

    def _realloc(self, block, size):
        if block and size <= self.sizes.get(block, 0):
            self.sizes[block] = size
            return block
        new = self.alloc_sized(size)
        if block:
            old = self.sizes.get(block, 0)
            self.write(new, self.read(block, min(old, size)))
        return new

    SANDBOX = os.environ["TRACE_SANDBOX"] if "TRACE_SANDBOX" in os.environ else os.path.join(os.path.dirname(os.path.abspath(__file__)), "sandbox")

    def _fopen(self, path, mode):
        name = self.read_c_string(path)
        m = self.read_c_string(mode)
        box = os.path.join(self.SANDBOX, os.path.basename(name.replace("\\", "/")))
        try:
            if any(c in m for c in "wa+"):
                handle = open(box, m.replace("t", "").replace("b", "") + "b")
            else:
                handle = open(box if os.path.exists(box) else name, "rb")
        except OSError:
            return 0
        stream = self.alloc(0x20)
        self.files[stream] = handle
        if hasattr(self, "log"):
            self.log.append(("fopen", name, m))
        return stream

    def _fgetc(self, stream):
        b = self.files[stream].read(1)
        if not b:
            self.write_u32(stream + 0xC, 0x10)          # _IOEOF
        return b[0] if b else 0xFFFFFFFF

    def _fread(self, buffer, size, count, stream):
        data = self.files[stream].read(size * count)
        self.write(buffer, data)
        if len(data) < size * count:
            self.write_u32(stream + 0xC, 0x10)
        return len(data) // size if size else 0

    def _fwrite(self, buf, size, count, stream):
        self.files[stream].write(self.read(buf, size * count))
        return count

    def _access(self, path, mode):
        name = self.read_c_string(path)
        box = os.path.join(self.SANDBOX, os.path.basename(name.replace("\\", "/")))
        return 0 if (os.path.exists(name) or os.path.exists(box)) else 0xFFFFFFFF

    def _fseek(self, stream, offset, origin):
        self.log.append(("fseek", stream, offset, origin, self.files[stream].name, hex(struct.unpack("<I", self.read(self.uc.reg_read(UC_X86_REG_ESP), 4))[0])))
        self.write_u32(stream + 0xC, 0)
        try:
            self.files[stream].seek(offset if offset < 0x80000000 else offset - (1 << 32), origin)
        except OSError:
            return 0xFFFFFFFF
        return 0

    def _makepath(self, out, drive, folder, name, ext):
        parts = [self.read_c_string(x) if x else "" for x in (drive, folder, name, ext)]
        path = os.path.join(parts[0] + parts[1], parts[2])
        if parts[3]:
            path += ("" if parts[3].startswith(".") else ".") + parts[3]
        self.write(out, path.encode("latin-1") + b"\0")
        return 0

    def _findfirst(self, pattern, data, flag):
        p = self.read_c_string(pattern)
        self.log.append(("findfirst", p))
        return 0 if p.lower().endswith("cm0102.exe") else 1

    def _progress(self, this, text, step, total, channel):
        self.progress.append((self.read_c_string(text), step, total, channel))
        self.where = (self.progress[-1][0], step)
        if self.where[0] == "Initialising Leagues":
            pickle.dump(self.first, open(OUT + "-first.pkl", "wb"))
            self.log.append(("date", self.read(0xAE2C90, 8).hex()))
        if step % 1000 == 0 or self.progress[-1][0] != "Player histories":
            print("progress", self.progress[-1], len(self.blocks), flush=True)
        return 0

    def _replace_ret(self, addr, nargs, fn):
        """A thiscall or stdcall function that pops nargs dwords."""
        def on_call(uc, address, size, user_data):
            args = self._stack_args(nargs) if nargs else ()
            this = uc.reg_read(UC_X86_REG_ECX)
            uc.reg_write(UC_X86_REG_EAX, (fn(this, *args) or 0) & 0xFFFFFFFF)
        self.uc.mem_write(addr, b"\xc2" + struct.pack("<H", nargs * 4) if nargs else b"\xc3")
        self.uc.hook_add(UC_HOOK_CODE, on_call, begin=addr, end=addr)

    def _sprintf(self, buffer, fmt, *values):
        values = self._stack_args(14)[2:]
        return self._sprintf2(buffer, fmt, *values)

    def _sprintf2(self, buffer, fmt, *values):
        f = self.read_c_string(fmt)
        vals = list(values)
        out = ""
        for m in re.finditer(r"%([-+ #0]*)(\d*)(?:\.(\d+))?(l|h)?([dioxXucsf%])|[^%]+", f):
            if not m.group(5):
                out += m.group(0)
                continue
            k = m.group(5)
            if k == "%":
                out += "%"
                continue
            v = vals.pop(0) if vals else 0
            spec = "%" + m.group(1) + m.group(2) + ("." + m.group(3) if m.group(3) else "")
            if k == "s":
                out += (spec + "s") % self.read_c_string(v)
            elif k == "c":
                out += chr(v & 0xFF)
            elif k in "di":
                out += (spec + "d") % (v - (1 << 32) if v & 0x80000000 else v)
            elif k == "f":
                out += "?"
            else:
                out += (spec + k) % v
        self.write(buffer, out.encode("latin-1", "replace") + b"\0")
        return len(out)

    def _on_block(self, uc, address, size, user_data):
        self.blocks.add(address)

    def _on_entry(self, uc, address, size, user_data):
        self.blocks.add(address)
        self.first.setdefault(address, self.where)
        h = self.hooks.pop(address, None)
        if h is not None:
            uc.hook_del(h)

    # imports
    def _imports(self):
        data = open(EXE, "rb").read()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        n = struct.unpack_from("<H", data, pe + 6)[0]
        opt = struct.unpack_from("<H", data, pe + 20)[0]
        secs = []
        for i in range(n):
            _, vs, va, rs, rp = struct.unpack_from("<8sIIII", data, pe + 24 + opt + i * 40)
            secs.append((va, vs, rs, rp))

        def off(rva):
            for va, vs, rs, rp in secs:
                if va <= rva < va + max(vs, rs):
                    return rp + rva - va
        imp = struct.unpack_from("<I", data, pe + 24 + 104)[0]
        self.uc.mem_map(IMPSTUBS, 0x1000)
        self.impnames = {}
        d = off(imp)
        k = 0
        while True:
            ilt, _, _, name, iat = struct.unpack_from("<5I", data, d)
            d += 20
            if name == 0:
                break
            i = 0
            while True:
                e = struct.unpack_from("<I", data, off(ilt or iat) + 4 * i)[0]
                if e == 0:
                    break
                nm = data[off(e) + 2:].split(b"\0")[0].decode() if not e & 0x80000000 else "ord%d" % (e & 0xFFFF)
                stub = IMPSTUBS + k * 4
                self.uc.mem_write(stub, b"\xf4\x90\x90\x90")
                self.write_u32(0x400000 + iat + 4 * i, stub)
                self.impnames[stub] = nm
                i += 1
                k += 1
        self.uc.hook_add(UC_HOOK_CODE, self._on_import, begin=IMPSTUBS, end=IMPSTUBS + 0xFFF)

    IMPORTS = {
        "GlobalAlloc": (2, lambda s, flags, size: s.alloc_sized(size)),
        "GlobalFree": (1, lambda s, h: 0),
        "GetLogicalDriveStringsA": (2, lambda s, n, buf: s.write(buf, b"D:\\\0\0") or 4),
        "GetDriveTypeA": (1, lambda s, p: 5),
        "DeleteFileA": (1, lambda s, p: 1),
        "GlobalMemoryStatus": (1, lambda s, p: s.write(p, struct.pack("<8I", 32, 0, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30)) or 0),
        "GetTickCount": (0, lambda s: 100000),
        "HeapSize": (3, lambda s, h, f, p: s.sizes.get(p, 0)),
        "HeapAlloc": (3, lambda s, h, f, n: s.alloc_sized(n)),
        "HeapFree": (3, lambda s, h, f, p: 1),
        "HeapReAlloc": (4, lambda s, h, f, p, n: s._realloc(p, n)),
        "GetProcessHeap": (0, lambda s: 1),
        "GetCurrentDirectoryA": (2, lambda s, n, b: s.write(b, b"C:\\CM\0") or 5),
        "SetCurrentDirectoryA": (1, lambda s, p: 1),
        "GetFullPathNameA": (4, lambda s, p, n, b, f: s.write(b, s.read(p, 260).split(b"\0")[0] + b"\0") or len(s.read(p, 260).split(b"\0")[0])),
        "FindFirstFileA": (2, lambda s, p, d: 0xFFFFFFFF),
        "FindNextFileA": (2, lambda s, h, d: 0),
        "FindClose": (1, lambda s, h: 1),
        "CloseHandle": (1, lambda s, h: 1),
        "ShowWindow": (2, lambda s, h, c: 0),
        "MessageBoxA": (4, lambda s, a, b, c, d: 1),
        "OutputDebugStringA": (1, lambda s, p: 0),
        "RaiseException": (4, lambda s, c, f, n, a: s.log.append(("RaiseException", hex(c))) or 0),
        "CreateDirectoryA": (2, lambda s, p, a: 1),
        "RemoveDirectoryA": (1, lambda s, p: 1),
        "GetTempPathA": (2, lambda s, n, b: s.write(b, b"C:\\T\\\0") or 5),
        "CreateFileA": (7, lambda s, *a: 0xFFFFFFFF),
        "GetLocalTime": (1, lambda s, p: s.write(p, struct.pack("<8H", 2001, 7, 0, 14, 12, 0, 0, 0)) or 0),
        "GetSystemTime": (1, lambda s, p: s.write(p, struct.pack("<8H", 2001, 7, 0, 14, 12, 0, 0, 0)) or 0),
        "PeekMessageA": (5, lambda s, *a: 0),
        "GetAsyncKeyState": (1, lambda s, k: 0),
        "GetKeyState": (1, lambda s, k: 0),
        "GetCursorPos": (1, lambda s, p: s.write(p, bytes(8)) or 1),
        "SetCursor": (1, lambda s, c: 0),
        "GetModuleHandleA": (1, lambda s, p: 0x400000),
        "timeGetTime": (0, lambda s: 100000),
        "GetFileAttributesA": (1, lambda s, p: 0xFFFFFFFF),
        "GetLastError": (0, lambda s: 2),
        "SetLastError": (1, lambda s, e: 0),
        "Sleep": (1, lambda s, ms: 0),
        "GetSystemTimeAsFileTime": (1, lambda s, p: s.write(p, bytes(8)) or 0),
        "QueryPerformanceCounter": (1, lambda s, p: s.write(p, bytes(8)) or 1),
        "GetCurrentThreadId": (0, lambda s: 1),
        "EnterCriticalSection": (1, lambda s, p: 0),
        "LeaveCriticalSection": (1, lambda s, p: 0),
        "InitializeCriticalSection": (1, lambda s, p: 0),
        "DeleteCriticalSection": (1, lambda s, p: 0),
    }

    def _on_import(self, uc, address, size, user_data):
        name = self.impnames.get(address, "?")
        if name not in self.IMPORTS:
            self.unknown = name
            sp = uc.reg_read(UC_X86_REG_ESP)
            ret = struct.unpack("<I", self.read(sp, 4))[0]
            frames = struct.unpack("<64I", self.read(sp, 256))
            calls = [hex(x) for x in frames if 0x401000 <= x < 0x960000]
            return self._fail("import %s from %#x stack %s" % (name, ret, calls[:12]))
        n, fn = self.IMPORTS[name]
        args = self._stack_args(n)
        r = fn(self, *args)
        sp = uc.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack("<I", self.read(sp, 4))[0]
        uc.reg_write(UC_X86_REG_EAX, (r or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, sp + 4 + 4 * n)
        uc.reg_write(UC_X86_REG_EIP, ret)

    def run(self, addr, *args, this=0):
        sp = O.STACK_BASE + O.STACK_SIZE - 0x1000
        for a in reversed(args):
            sp -= 4
            self.write_u32(sp, a)
        sp -= 4
        self.write_u32(sp, O.RETURN_ADDR)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, this)
        self.error = None
        t = time.time()
        start = addr
        last = (len(self.progress), time.time())
        self.samples = []
        while True:
            try:
                self.uc.emu_start(start, O.RETURN_ADDR, timeout=20 * 1000 * 1000)
            except Exception as exc:
                eip = self.uc.reg_read(UC_X86_REG_EIP)
                raise RuntimeError(f"{addr:#x} failed at {eip:#x}: {exc} {self.error}")
            eip = self.uc.reg_read(UC_X86_REG_EIP)
            if self.error or eip == O.RETURN_ADDR:
                break
            self.samples.append(eip)
            if len(self.progress) != last[0]:
                last = (len(self.progress), time.time())
            elif time.time() - last[1] > 240:
                raise RuntimeError(f"{addr:#x} stuck, eip samples {[hex(x) for x in self.samples[-12:]]}")
            start = eip
        if self.error:
            eip = self.uc.reg_read(UC_X86_REG_EIP)
            raise RuntimeError(f"{addr:#x} failed at {eip:#x}: {self.error}")
        eax = self.uc.reg_read(UC_X86_REG_EAX)
        print(f"{addr:#x} -> {eax:#x} in {time.time() - t:.1f}s, blocks {len(self.blocks)}", flush=True)
        return eax


def u32(t, a):
    return struct.unpack("<I", t.read(a, 4))[0]


def date(t, a):
    d, y, leap, z = struct.unpack("<hHHH", t.read(a, 8))
    return (d, y, leap)


def main():
    t = Tracer()
    t.heap_floor = t.heap_top
    settings = t.alloc(0x400)
    t.write_u32(0xAE3138, settings)
    t.write_u32(0x9EB2DC, 0)          # language index: English
    phases = {}
    # FUN_0081b890: seed and the first game date
    t.run(0x90D0A0, 12345)
    t.run(0x54A130, 1, 0, 2001, 0xFFFFFFFF, this=0xAE2C90)
    print("game date after start screen", date(t, 0xAE2C90))
    path = t.alloc_c_string(DATA + "/")
    r = t.run(0x524D50, path, 1)
    phases["load"] = set(t.blocks)
    seen = set(t.blocks)
    counts = {hex(a): u32(t, a) for a in range(0xAE2340, 0xAE23A4, 4)}
    ptrs = {hex(a): u32(t, a) for a in range(0xAE23A4, 0xAE2408, 4)}
    print("load returned", r)
    json.dump({"counts": counts, "ptrs": ptrs, "progress": t.progress, "log": t.log[:50]}, open(OUT + "-load.json", "w"), indent=1)
    t.blocks.clear()
    t.run(0x827D50)
    t.run(0x667150)
    phases["leagues"] = set(t.blocks) - seen
    seen |= t.blocks
    t.progress.clear()
    try:
        r = t.run(0x828110, 0)
        print("create returned", r)
    except RuntimeError as exc:
        print("create failed:", exc, t.log[-5:])
    phases["create"] = set(t.blocks) - seen
    print("game date", date(t, 0xAE2C90), "start", date(t, 0xDD6EC8), "slice", t.read(0xAE2C88, 1))
    after = {hex(a): u32(t, a) for a in range(0xAE2340, 0xAE2408, 4)}
    json.dump({"log": [str(x) for x in t.log], "after": after, "progress": t.progress[:200], "nprog": len(t.progress),
               "date": date(t, 0xAE2C90), "start": date(t, 0xDD6EC8)}, open(OUT + "-create.json", "w"), indent=1)
    pickle.dump((phases, t.first), open(OUT + "-blocks.pkl", "wb"))


main()
