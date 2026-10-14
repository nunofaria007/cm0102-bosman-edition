"""Runs the database load of the original (FUN_00528540) in Unicorn, with the same memory layout and the
same replacements as the recompiled program (src/recomp_rt). The two runs must leave the same bytes.

The replacements here and in src/recomp_rt/crt.c and imports.c must stay the same. Each one says what it
answers. The heap is a bump allocator that never reuses memory, in both runs.
"""
import os
import struct
import sys
import time

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools", "recomp"))
from pe import Image, IMPORT_STUBS  # noqa: E402

PAGE = 0x1000
STACK_BASE, STACK_SIZE = 0x20000000, 0x100000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x1000
RETURN_ADDR = 0x40000000
HEAP_BASE, HEAP_SIZE = 0x70000000, 0x40000000
CW_STUB = 0x40000100           # fldcw [CW_STUB + 0x10]; hlt. Windows starts a process with CW 0x27f.
SETTINGS = 0xAE3138            # pointer to the settings block; the load reads the language from it
LANGUAGE = 0x9EB2DC
VERSION_TEXT = 0xAD4394        # "3.9.68", what FUN_0090d130 returns


def align(n):
    return (n + PAGE - 1) & ~(PAGE - 1)


class Reference:
    def __init__(self, exe, data_dir):
        self.image = Image(exe)
        self.data_dir = data_dir.rstrip("/")
        self.uc = uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(self.image.base, align(self.image.size))
        for _, va, vsize, rptr, rsize, _ in self.image.sections:
            uc.mem_write(va, self.image.data[rptr:rptr + min(rsize, vsize or rsize)])
        uc.mem_map(0, PAGE)                      # fs:[0], the SEH chain; the fs base is 0
        uc.mem_map(STACK_BASE, STACK_SIZE)
        uc.mem_map(RETURN_ADDR, PAGE)
        uc.mem_map(HEAP_BASE, HEAP_SIZE)
        uc.mem_map(IMPORT_STUBS, PAGE)
        self.heap_top = HEAP_BASE
        self.sizes = {}
        self.files = {}
        self.calls = {}
        self.error = None
        self.imports = {}
        for iat, (dll, name, stub) in self.image.imports.items():
            self.write_u32(iat, stub)
            uc.mem_write(stub, b"\xf4")
            self.imports[stub] = name
        uc.hook_add(UC_HOOK_CODE, self._on_import, begin=IMPORT_STUBS, end=IMPORT_STUBS + PAGE - 1)
        for addr, (name, nargs, pop, fn) in self.replacements().items():
            self._replace(addr, name, nargs, pop, fn)

    # Replacements: address -> (name, argument count, bytes popped by ret, function)
    def replacements(self):
        return {
            0x944E46: ("malloc", 1, 0, lambda n: self.alloc(n)),
            0x944CF1: ("operator_new", 1, 0, lambda n: self.alloc(n)),
            0x945501: ("calloc", 2, 0, lambda c, n: self.alloc(c * n)),
            0x945667: ("realloc", 2, 0, self._realloc),
            0x9452CA: ("free", 1, 0, lambda p: 0),
            0x944C94: ("operator_delete", 1, 0, lambda p: 0),
            0x945B00: ("fopen", 2, 0, self._fopen),
            0x945BFE: ("fread", 4, 0, self._fread),
            0x945A09: ("fclose", 1, 0, self._fclose),
            0x946A22: ("fseek", 3, 0, self._fseek),
            0x946AAE: ("ftell", 1, 0, lambda s: self.files[s].tell()),
            0x9469E0: ("fgetc", 1, 0, self._fgetc),
            0x9478D8: ("access", 2, 0, self._access),
            0x4C0AA0: ("findfirst", 3, 0, lambda p, d, f: 1),     # no *.lng to copy
            0x4C0B50: ("findnext", 1, 0, lambda f: 1),
            0x4C0C10: ("findclose", 1, 0, lambda f: 0),
            0x90BDE0: ("data_path", 2, 0, self._data_path),
            0x90D130: ("error_place", 3, 0, lambda a, b, c: VERSION_TEXT),
            0x5E8290: ("error_box", 0, 0, lambda: 0),
            0x802F80: ("progress", 4, 16, lambda *a: 0),            # thiscall: the progress box
            0x7A1710: ("free_memory_mb", 0, 0, lambda: 1000),
            0x7A1820: ("free_disk_mb", 1, 0, lambda a: 1000),
        }

    # Imports: name -> (argument count, function). All are stdcall.
    def import_table(self):
        return {
            "GlobalAlloc": (2, lambda flags, size: self.alloc(size)),
            "GlobalFree": (1, lambda h: 0),
            "DeleteFileA": (1, lambda p: 1),               # deletes nothing
        }

    # Memory helpers
    def read(self, addr, size):
        return bytes(self.uc.mem_read(addr, size))

    def write(self, addr, data):
        self.uc.mem_write(addr, bytes(data))

    def u32(self, addr):
        return struct.unpack("<I", self.read(addr, 4))[0]

    def write_u32(self, addr, value):
        self.write(addr, struct.pack("<I", value & 0xFFFFFFFF))

    def cstr(self, addr):
        out = bytearray()
        while True:
            chunk = self.read(addr + len(out), 64)
            z = chunk.find(b"\0")
            if z >= 0:
                return (out + chunk[:z]).decode("latin-1")
            out += chunk

    def alloc(self, size):
        addr = self.heap_top
        self.heap_top += (max(size, 1) + 15) & ~15
        if self.heap_top > HEAP_BASE + HEAP_SIZE:
            raise MemoryError("heap full")
        self.sizes[addr] = size
        return addr

    def alloc_str(self, text):
        data = text.encode("latin-1") + b"\0"
        addr = self.alloc(len(data))
        self.write(addr, data)
        return addr

    # CRT replacements
    def _realloc(self, block, size):
        if block and size <= self.sizes.get(block, 0):
            self.sizes[block] = size
            return block
        new = self.alloc(size)
        if block:
            self.write(new, self.read(block, min(self.sizes.get(block, 0), size)))
        return new

    def _fopen(self, path, mode):
        name = self.cstr(path)
        if any(c in self.cstr(mode) for c in "wa+"):
            return 0
        try:
            handle = open(name, "rb")
        except OSError:
            return 0
        stream = self.alloc(0x20)            # an MSVC FILE; the code reads its flags at +0xc
        self.files[stream] = handle
        return stream

    def _fread(self, buf, size, count, stream):
        want = size * count
        data = self.files[stream].read(want)
        self.write(buf, data)
        if len(data) < want:
            self.write_u32(stream + 0xC, self.u32(stream + 0xC) | 0x10)
        return len(data) // size if size else 0

    def _fgetc(self, stream):
        b = self.files[stream].read(1)
        if not b:
            self.write_u32(stream + 0xC, self.u32(stream + 0xC) | 0x10)
            return 0xFFFFFFFF
        return b[0]

    def _fseek(self, stream, offset, origin):
        self.write_u32(stream + 0xC, self.u32(stream + 0xC) & ~0x10)
        try:
            self.files[stream].seek(offset - (1 << 32) if offset & 0x80000000 else offset, origin)
        except (OSError, ValueError):
            return 0xFFFFFFFF
        return 0

    def _fclose(self, stream):
        self.files.pop(stream).close()
        return 0

    def _access(self, path, mode):
        return 0 if os.path.exists(self.cstr(path)) else 0xFFFFFFFF

    def _data_path(self, folder, name):
        """The file in the data folder, found without regard to case."""
        wanted = self.cstr(name).lower()
        found = sorted(e for e in os.listdir(self.data_dir) if e.lower() == wanted)
        return self.alloc_str(os.path.join(self.data_dir, found[0]) if found else wanted)

    # Hooks
    def _args(self, n):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        return struct.unpack(f"<{n}I", self.read(sp + 4, 4 * n)) if n else ()

    def _replace(self, addr, name, nargs, pop, fn):
        def on_call(uc, address, size, user_data):
            self.calls[name] = self.calls.get(name, 0) + 1
            uc.reg_write(UC_X86_REG_EAX, (fn(*self._args(nargs)) or 0) & 0xFFFFFFFF)
        self.uc.mem_write(addr, b"\xc2" + struct.pack("<H", pop) if pop else b"\xc3")
        self.uc.hook_add(UC_HOOK_CODE, on_call, begin=addr, end=addr)

    def _on_import(self, uc, address, size, user_data):
        name = self.imports.get(address, "?")
        sp = uc.reg_read(UC_X86_REG_ESP)
        entry = self.import_table().get(name)
        if entry is None:
            self.error = f"import {name} called from {self.u32(sp):#x}"
            uc.emu_stop()
            return
        nargs, fn = entry
        self.calls[name] = self.calls.get(name, 0) + 1
        uc.reg_write(UC_X86_REG_EAX, (fn(*self._args(nargs)) or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, sp + 4 + 4 * nargs)
        uc.reg_write(UC_X86_REG_EIP, self.u32(sp))

    def call(self, addr, *args, this=0):
        sp = STACK_TOP
        for a in reversed(args):
            sp -= 4
            self.write_u32(sp, a)
        sp -= 4
        self.write_u32(sp, RETURN_ADDR)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, this)
        self.error = None
        try:
            self.uc.emu_start(addr, RETURN_ADDR)
        except UcError as exc:
            raise RuntimeError(f"{addr:#x} failed at {self.uc.reg_read(UC_X86_REG_EIP):#x}: {exc}") from exc
        if self.error:
            raise RuntimeError(f"{addr:#x} failed: {self.error}")
        return self.uc.reg_read(UC_X86_REG_EAX)

    def set_control_word(self, cw):
        self.write(CW_STUB, b"\xd9\x2d" + struct.pack("<I", CW_STUB + 0x10))
        self.write(CW_STUB + 6, b"\xc3")
        self.write(CW_STUB + 0x10, struct.pack("<H", cw))
        self.call(CW_STUB)


def run_load(exe, data_dir):
    """Sets up the state before the load the same way as the recompiled driver, runs it, returns the run."""
    r = Reference(exe, data_dir)
    r.set_control_word(0x27F)
    settings = r.alloc(0x400)
    r.write_u32(SETTINGS, settings)
    r.write_u32(LANGUAGE, 0)
    path = r.alloc_str(data_dir.rstrip("/") + "/")
    t = time.time()
    r.result = r.call(0x528540, path, 1, 0)
    r.seconds = time.time() - t
    return r


if __name__ == "__main__":
    run = run_load(sys.argv[1], sys.argv[2])
    print("returned", run.result, "in %.1f s" % run.seconds, "heap top %#x" % run.heap_top)
    print(run.calls)
