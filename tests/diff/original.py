"""Runs functions of the original (cm0102.exe) in the Unicorn emulator.

The DirectDraw back buffer is replaced with a fake surface whose Lock, Unlock
and Blt write to an 800x600 RGB565 buffer in emulated memory.
"""
import hashlib
import os
import re
import struct
import tempfile

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

WIDTH, HEIGHT = 800, 600
PITCH = WIDTH * 2
FB_SIZE = PITCH * HEIGHT

EXPECTED_SHA256 = "0c78e7da6d0bcb4603fa179449bbe6d1b6b5e1c627a22c445d14d489bd9bff86"

PAGE = 0x1000
STACK_BASE, STACK_SIZE = 0x20000000, 0x100000
CLEAN_STACK = 0x10000        # zeroed below the stack top before each call
# 1 GB: the database load allocates about 80 MB, game creation much more.
HEAP_BASE, HEAP_SIZE = 0x70000000, 0x40000000
RETURN_ADDR = 0x40000000
FB_BASE = 0x50000000
SURFACE = 0x60000000
VTABLE = SURFACE + 0x100
STUBS = 0x61000000
STUB_STRIDE = 8
# Pages of the test harness: 0x60 surface, 0x61 surface stubs, 0x62 stack fakes (cases_stack),
# 0x63 load model (cases_select), 0x64 window stub (cases_window), 0x65 import stubs, 0x66 random
# loop (cases_random). A new page takes the next free number.
IMPORT_STUBS = 0x65000000    # one hlt for each import of the original, 4 bytes apart

# Globals of the original.
G_DISPLAY_OFF = 0xAEBE0C     # non-zero disables every drawing function
G_BACK_BUFFER = 0xAEBDE4     # IDirectDrawSurface *
G_SCREEN_W = 0xAEBE08
G_SCREEN_H = 0xAEBDD0
G_PIXEL_FORMAT = 0xAE3158    # DDPIXELFORMAT, 32 bytes

FUN_INIT_PALETTE = 0x5E4560  # sets the colour globals from the pixel format

# -1 selects the bitmap fonts. The original starts at -2, which selects the T2K font engine.
G_FONT_MODE = 0x9CC2C4
# Fonts of 0x1404 bytes each, addressed by font number. Only 1 to 7 exist: the place that the
# code works out for font 0 holds the pixel format and the palette.
G_FONTS = 0xAE1DAC
FONT_SIZE = 0x1404
FUN_FONT_LOAD = 0x5E4BA0     # (font number, path)

# The text function reads parts of its stack frame that it never set, when it cuts a line after
# a line break, a control character or an earlier cut. What it finds there is whatever earlier
# calls left. The test makes it zeros.
FUN_TEXT = 0x5E6D80
TEXT_FRAME = 0x400
FONT_FILES = {1: "arial_narrow_10.fnt", 2: "arial_narrow_11.fnt", 3: "arial_14.fnt", 4: "arial_16.fnt",
              5: "arial_18.fnt", 6: "trade_cond_24_bold.fnt", 7: "trade_cond_28_bold.fnt"}

# Calls of the screen draw that the test answers itself.
FUN_SCREEN_TITLE = 0x7FEE80  # thiscall on the screen stack: the title of the screen, its background name
FUN_DATA_PATH = 0x90BDE0     # (folder key, file name): the path of a file of the original
FUN_FLIP = 0x5E2EB0          # (x1, y1, x2, y2): shows a box of the back buffer

# CRT functions replaced with host-side versions.
CRT_DELETE = 0x944C94
CRT_NEW = 0x944CF1
CRT_MALLOC = 0x944E46
CRT_FREE = 0x9452CA
CRT_CALLOC = 0x945501
CRT_FCLOSE = 0x945A09
CRT_FOPEN = 0x945B00
CRT_FREAD = 0x945BFE
CRT_SPLITPATH = 0x944CFF
CRT_SPRINTF = 0x944C9F
CRT_REALLOC = 0x945667
CRT_FWRITE = 0x945CE6
CRT_FSEEK = 0x946A22
CRT_FTELL = 0x946AAE
CRT_FGETC = 0x9469E0
CRT_ACCESS = 0x9478D8
CRT_TIME = 0x946EBB
CRT_CLOCK = 0x947299
# The findfirst wrappers of the original: first (pattern, data, flag), next, close.
FUN_FIND_FIRST, FUN_FIND_NEXT, FUN_FIND_CLOSE = 0x4C0AA0, 0x4C0B50, 0x4C0C10
FUN_COPY_FILE = 0x90CC50     # (from, to): the loader copies the .lng files with it
FUN_FREE_MEMORY = 0x7A1710   # free memory in MB
FUN_MEMORY_CHECK = 0x7A1820
MSVC_EOF = 0x10              # _IOEOF in the flags of a FILE, +0xc

# The error box of the original: FUN_005e8290(title, message, 0) shows it. Before that the code
# pushes the file name, extension and line for a sprintf and calls FUN_0090d130, which returns the
# version text "3.9.68" (0xAD4394). The test reads the line from those pushes.
FUN_ERROR_PLACE = 0x90D130
VERSION_TEXT = 0xAD4394
FUN_ERROR_BOX = 0x5E8290

# IDirectDrawSurface vtable slots used by the drawing code: slot -> stdcall argument bytes.
SLOT_BLT, SLOT_LOCK, SLOT_UNLOCK = 0x14 // 4, 0x64 // 4, 0x80 // 4
SLOT_ARG_BYTES = {SLOT_BLT: 0x18, SLOT_LOCK: 0x14, SLOT_UNLOCK: 0x08}

DDBLT_COLORFILL = 0x400


def _align(n):
    return (n + PAGE - 1) & ~(PAGE - 1)


class Original:
    def __init__(self, exe_path, data_dir):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.error = None
        self.exe_path = exe_path
        self._load_pe(exe_path)
        uc = self.uc
        uc.mem_map(STACK_BASE, STACK_SIZE)
        uc.mem_map(HEAP_BASE, HEAP_SIZE)
        uc.mem_map(RETURN_ADDR, PAGE)
        uc.mem_map(FB_BASE, _align(FB_SIZE))
        uc.mem_map(SURFACE, PAGE)
        uc.mem_map(STUBS, PAGE)
        # Functions with exception frames read and write fs:[0]. The segment base is 0 here.
        uc.mem_map(0, PAGE)
        self.heap_floor = HEAP_BASE
        self.heap_top = HEAP_BASE
        self.sizes = {}              # block -> size, for realloc and HeapSize

        uc.mem_write(SURFACE, struct.pack("<I", VTABLE))
        for slot in range(64):
            stub = STUBS + slot * STUB_STRIDE
            uc.mem_write(VTABLE + slot * 4, struct.pack("<I", stub))
            arg_bytes = SLOT_ARG_BYTES.get(slot)
            code = b"\xf4" if arg_bytes is None else b"\xc2" + struct.pack("<H", arg_bytes)
            uc.mem_write(stub, code)
        uc.hook_add(UC_HOOK_CODE, self._on_surface_call, begin=STUBS, end=STUBS + PAGE - 1)

        self.files = {}
        self._replace(CRT_CALLOC, 2, lambda count, size: self.alloc(count * size))
        self._replace(CRT_MALLOC, 1, self.alloc)
        self._replace(CRT_NEW, 1, self.alloc)
        self.frees = 0
        self._replace(CRT_FREE, 1, self._free)
        self._replace(CRT_DELETE, 1, lambda block: 0)
        self._replace(CRT_FOPEN, 2, self._fopen)
        self._replace(CRT_FREAD, 4, self._fread)
        self._replace(CRT_FCLOSE, 1, self._fclose)
        self._replace(CRT_SPLITPATH, 0, lambda: 0)
        self._replace(CRT_SPRINTF, 2, self._sprintf)
        self.sandbox_dir = tempfile.TemporaryDirectory(prefix="cm0102-diff-")
        self.sandbox = self.sandbox_dir.name
        self._replace(CRT_REALLOC, 2, self._realloc)
        self._replace(CRT_FWRITE, 4, self._fwrite)
        self._replace(CRT_FSEEK, 3, self._fseek)
        self._replace(CRT_FTELL, 1, lambda stream: self.files[stream].tell())
        self._replace(CRT_FGETC, 1, self._fgetc)
        self._replace(CRT_ACCESS, 2, self._access)
        self._replace(CRT_TIME, 1, self._time)
        self._replace(CRT_CLOCK, 0, lambda: 12345)
        self._replace(FUN_FIND_FIRST, 3, self._find_first)
        self._replace(FUN_FIND_NEXT, 1, lambda find: 1)
        self._replace(FUN_FIND_CLOSE, 1, lambda find: 0)
        self._replace(FUN_COPY_FILE, 2, lambda source, target: 1)
        self._replace(FUN_FREE_MEMORY, 0, lambda: 1000)
        self._replace(FUN_MEMORY_CHECK, 1, lambda value: 1000)
        self._imports(exe_path)
        uc.hook_add(UC_HOOK_CODE, self._on_text_entry, begin=FUN_TEXT, end=FUN_TEXT)
        self.error_lines = []
        self.data_dir = data_dir
        self.pictures_dir = os.path.join(os.path.dirname(data_dir), "Pictures")
        self.no_data_files = False     # the path lookup then names a file that does not exist
        self.title = self.alloc_c_string("")
        self._replace(FUN_SCREEN_TITLE, 0, lambda: self.title)
        self._replace(FUN_DATA_PATH, 2, self._data_path)
        self.flips = []
        uc.hook_add(UC_HOOK_CODE, self._on_flip, begin=FUN_FLIP, end=FUN_FLIP)
        self._replace(FUN_ERROR_PLACE, 3, lambda name, ext, line: self.error_lines.append(line) or VERSION_TEXT)
        self._replace(FUN_ERROR_BOX, 0, lambda: 0)

        self.write_u32(G_DISPLAY_OFF, 0)
        self.write_u32(G_BACK_BUFFER, SURFACE)
        self.write_u32(G_SCREEN_W, WIDTH)
        self.write_u32(G_SCREEN_H, HEIGHT)
        uc.mem_write(G_PIXEL_FORMAT, struct.pack("<8I", 32, 0x40, 0, 16, 0xF800, 0x07E0, 0x001F, 0))
        self.call(FUN_INIT_PALETTE)
        self.write_u32(G_FONT_MODE, -1)
        for font, name in FONT_FILES.items():
            if not self.load_font(font, os.path.join(data_dir, name)):
                raise SystemExit(f"the original cannot load {name} from {data_dir}")

    def _load_pe(self, path):
        with open(path, "rb") as exe:
            data = exe.read()
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256:
            raise SystemExit(f"{path}: SHA256 {digest} is not the expected cm0102.exe")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        section_count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        image_size = struct.unpack_from("<I", data, pe + 24 + 56)[0]
        self.uc.mem_map(base, _align(image_size))
        self.image_base = base
        self.image = bytearray(_align(image_size))     # as loaded, before any test patches code
        table = pe + 24 + optional_size
        for i in range(section_count):
            _, vsize, va, rsize, rptr = struct.unpack_from("<8sIIII", data, table + i * 40)
            section = data[rptr:rptr + min(rsize, vsize or rsize)]
            self.image[va:va + len(section)] = section
            self.uc.mem_write(base + va, section)

    def pristine(self, addr, size):
        """Bytes of the original as loaded, before any test patched them."""
        return bytes(self.image[addr - self.image_base:addr - self.image_base + size])

    # Memory helpers

    def read(self, addr, size):
        return bytes(self.uc.mem_read(addr, size))

    def write(self, addr, data):
        self.uc.mem_write(addr, bytes(data))

    def write_u32(self, addr, value):
        self.write(addr, struct.pack("<I", value & 0xFFFFFFFF))

    def read_c_string(self, addr):
        text = bytearray()
        while (byte := self.read(addr + len(text), 1)) != b"\0":
            text += byte
        return text.decode("latin-1")

    def alloc_bytes(self, data):
        addr = self.alloc(len(data))
        self.write(addr, data)
        return addr

    def put_pixel(self, x, y, value):
        self.write(FB_BASE + y * PITCH + x * 2, struct.pack("<H", value))

    def alloc_c_string(self, text):
        data = text.encode("latin-1") + b"\0"
        addr = self.alloc(len(data))
        self.write(addr, data)
        return addr

    def alloc(self, size):
        addr = self.heap_top
        self.heap_top += (size + 15) & ~15
        if self.heap_top > HEAP_BASE + HEAP_SIZE:
            raise MemoryError("emulated heap is full")
        self.write(addr, bytes(size))
        self.sizes[addr] = size
        return addr

    def load_font(self, font, path):
        """Loads a font with the loader of the original and keeps its memory through heap resets."""
        loaded = self.call(FUN_FONT_LOAD, font, self.alloc_c_string(path))
        self.heap_floor = self.heap_top
        return loaded

    def reset_heap(self):
        """Frees everything but the fonts, which the original loads once at the start."""
        self.heap_top = self.heap_floor

    def framebuffer(self):
        return self.read(FB_BASE, FB_SIZE)

    def set_framebuffer(self, data):
        assert len(data) == FB_SIZE
        self.write(FB_BASE, data)

    # Calls

    def call(self, addr, *args, this=0):
        """Calls a cdecl function, or a thiscall function with `this`. Returns EAX as a signed number."""
        sp = STACK_BASE + STACK_SIZE - 0x1000
        # Some functions of the original read stack memory that they never set.
        self.write(sp - CLEAN_STACK, bytes(CLEAN_STACK))
        for arg in reversed(args):
            sp -= 4
            self.write_u32(sp, arg)
        sp -= 4
        self.write_u32(sp, RETURN_ADDR)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_ECX, this)
        self.error = None
        try:
            self.uc.emu_start(addr, RETURN_ADDR, timeout=60 * 1000 * 1000)
        except UcError as exc:
            raise RuntimeError(f"emulation of {addr:#x} failed: {exc}") from exc
        if self.error:
            raise RuntimeError(f"emulation of {addr:#x} failed: {self.error}")
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN_ADDR:
            raise RuntimeError(f"emulation of {addr:#x} did not return in time")
        eax = self.uc.reg_read(UC_X86_REG_EAX)
        return eax - (1 << 32) if eax & 0x80000000 else eax

    # Hooks

    def _stack_args(self, count):
        sp = self.uc.reg_read(UC_X86_REG_ESP)
        return struct.unpack(f"<{count}I", self.read(sp + 4, count * 4))

    def _fail(self, message):
        self.error = message
        self.uc.emu_stop()

    def _on_surface_call(self, uc, address, size, user_data):
        slot = (address - STUBS) // STUB_STRIDE
        if slot == SLOT_LOCK:
            _, rect, desc, _, _ = self._stack_args(5)
            left, top = struct.unpack("<2i", self.read(rect, 8)) if rect else (0, 0)
            self.write(desc + 0x08, struct.pack("<3I", HEIGHT, WIDTH, PITCH))
            self.write_u32(desc + 0x24, FB_BASE + top * PITCH + left * 2)
        elif slot == SLOT_BLT:
            _, rect, _, _, flags, fx = self._stack_args(6)
            if not flags & DDBLT_COLORFILL or not rect:
                return self._fail(f"unsupported Blt, flags {flags:#x}")
            left, top, right, bottom = struct.unpack("<4i", self.read(rect, 16))
            colour = self.read(fx + 0x50, 2)
            if not (0 <= left <= right <= WIDTH and 0 <= top <= bottom <= HEIGHT):
                return self._fail(f"Blt rectangle off screen: {left},{top},{right},{bottom}")
            row = colour * (right - left)
            for y in range(top, bottom):
                self.write(FB_BASE + y * PITCH + left * 2, row)
        elif slot != SLOT_UNLOCK:
            return self._fail(f"unexpected IDirectDrawSurface call, vtable offset {slot * 4:#x}")
        uc.reg_write(UC_X86_REG_EAX, 0)

    def _on_text_entry(self, uc, address, size, user_data):
        self.write(uc.reg_read(UC_X86_REG_ESP) - TEXT_FRAME, bytes(TEXT_FRAME))

    def _replace(self, addr, arg_count, host_function):
        """Makes a cdecl function of the original return the result of a host function.
        Returns a function that undoes the replacement."""
        def on_call(uc, address, size, user_data):
            result = host_function(*self._stack_args(arg_count))
            uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        return self._hook_at(addr, b"\xc3", on_call)

    def replace_thiscall(self, addr, arg_count, host_function):
        """The same for a thiscall or stdcall function, which pops its arguments. The host function
        gets ECX first. Returns a function that undoes the replacement."""
        def on_call(uc, address, size, user_data):
            args = self._stack_args(arg_count) if arg_count else ()
            result = host_function(uc.reg_read(UC_X86_REG_ECX), *args)
            uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        code = b"\xc2" + struct.pack("<H", arg_count * 4) if arg_count else b"\xc3"
        return self._hook_at(addr, code, on_call)

    def _hook_at(self, addr, code, on_call):
        """A later replacement of the same address takes the place of the earlier one until it is
        undone."""
        replaced = self.__dict__.setdefault("replaced", {})
        old = self.read(addr, len(code))
        earlier = replaced.get(addr)
        if earlier:
            self.uc.hook_del(earlier[0])
        self.uc.mem_write(addr, code)
        hook = self.uc.hook_add(UC_HOOK_CODE, on_call, begin=addr, end=addr)
        replaced[addr] = (hook, on_call)
        # Unicorn checks the address range of a hook when it translates code. A block that it
        # translated before keeps running without the new hook.
        self.uc.ctl_flush_tb()

        def undo():
            self.uc.hook_del(replaced.pop(addr)[0])
            self.uc.mem_write(addr, old)
            if earlier:
                replaced[addr] = (self.uc.hook_add(UC_HOOK_CODE, earlier[1], begin=addr, end=addr), earlier[1])
            self.uc.ctl_flush_tb()
        return undo

    def _data_path(self, folder, name):
        """The CM3_DATA or CM3_PICTURES folder. A backslash in the name separates folders."""
        if self.no_data_files:
            return self.alloc_c_string(os.path.join(self.data_dir, "no such file"))
        if self.read_c_string(folder) == "CM3_PICTURES":
            path = self.pictures_dir
            for part in self.read_c_string(name).replace("/", "\\").split("\\"):
                found = [e for e in os.listdir(path) if e.lower() == part.lower()] if os.path.isdir(path) else []
                path = os.path.join(path, found[0] if found else part)
            return self.alloc_c_string(path)
        wanted = self.read_c_string(name).lower()
        found = [entry for entry in os.listdir(self.data_dir) if entry.lower() == wanted]
        return self.alloc_c_string(os.path.join(self.data_dir, found[0]) if found else wanted)

    def _on_flip(self, uc, address, size, user_data):
        box = list(struct.unpack("<4i", self.read(uc.reg_read(UC_X86_REG_ESP) + 4, 16)))
        self.flips.append((box, self.framebuffer()))

    def _free(self, block):
        self.frees += 1
        return 0

    def _sprintf(self, buffer, text_format):
        """sprintf for %s, %c and the integer formats, with flags and widths. %f gives "?"."""
        values = list(self._stack_args(16)[2:])
        text = ""
        pattern = r"%([-+ #0]*)(\d*)(?:\.(\d+))?(l|h)?([dioxXucsf%])|%|[^%]+"
        for match in re.finditer(pattern, self.read_c_string(text_format)):
            kind = match.group(5)
            if not kind:
                text += match.group(0)
                continue
            if kind == "%":
                text += "%"
                continue
            value = values.pop(0) if values else 0
            spec = "%" + match.group(1) + match.group(2) + ("." + match.group(3) if match.group(3) else "")
            if kind == "s":
                text += (spec + "s") % self.read_c_string(value)
            elif kind == "c":
                text += chr(value & 0xFF)
            elif kind in "di":
                text += (spec + "d") % (value - (1 << 32) if value & 0x80000000 else value)
            elif kind == "f":
                text += "?"
            else:
                text += (spec + kind) % value
        self.write(buffer, text.encode("latin-1", "replace") + b"\0")
        return len(text)

    # The file layer. A read comes from the named file, or from the file of the same name in the
    # sandbox folder when the original wrote one there. A write goes to the sandbox folder only.

    def _sandbox_path(self, name):
        return os.path.join(self.sandbox, os.path.basename(name.replace("\\", "/")))

    def _fopen(self, path, mode):
        name = self.read_c_string(path)
        mode = self.read_c_string(mode) if mode else "rb"
        box = self._sandbox_path(name)
        try:
            if any(c in mode for c in "wa+"):
                handle = open(box, mode.replace("t", "").replace("b", "") + "b")
            else:
                handle = open(box if os.path.exists(box) else name, "rb")
        except OSError:
            return 0
        # A FILE of MSVC is 0x20 bytes. Code of the original reads its flags at +0xc.
        stream = self.alloc(0x20)
        self.files[stream] = handle
        return stream

    def _fread(self, buffer, size, count, stream):
        data = self.files[stream].read(size * count)
        self.write(buffer, data)
        if len(data) < size * count:
            self.write_u32(stream + 0xC, MSVC_EOF)
        return len(data) // size if size else 0

    def _fclose(self, stream):
        self.files.pop(stream).close()
        return 0

    def _fwrite(self, buffer, size, count, stream):
        self.files[stream].write(self.read(buffer, size * count))
        return count

    def _fseek(self, stream, offset, origin):
        self.write_u32(stream + 0xC, 0)
        try:
            self.files[stream].seek(offset - (1 << 32) if offset & 0x80000000 else offset, origin)
        except OSError:
            return -1
        return 0

    def _fgetc(self, stream):
        byte = self.files[stream].read(1)
        if not byte:
            self.write_u32(stream + 0xC, MSVC_EOF)
            return -1
        return byte[0]

    def _access(self, path, mode):
        name = self.read_c_string(path)
        return 0 if os.path.exists(name) or os.path.exists(self._sandbox_path(name)) else -1

    def _time(self, out):
        if out:
            self.write_u32(out, 1000000000)
        return 1000000000

    def _find_first(self, pattern, data, flag):
        """Finds only cm0102.exe, which the CD check of game creation looks for."""
        return 0 if self.read_c_string(pattern).lower().endswith("cm0102.exe") else 1

    def _realloc(self, block, size):
        """In place when the block shrinks: FUN_00524d70 asserts that realloc keeps the address."""
        if block and size <= self.sizes.get(block, 0):
            self.sizes[block] = size
            return block
        new = self.alloc(size)
        if block:
            self.write(new, self.read(block, min(self.sizes.get(block, 0), size)))
        return new

    # Imports of the original. Each entry of the import table points to a hlt stub, and the hook
    # runs a host function in its place. An import with no host function stops the emulation.

    IMPORTS = {
        "GlobalAlloc": (2, lambda s, flags, size: s.alloc(size)),
        "GlobalFree": (1, lambda s, h: 0),
        "GetLogicalDriveStringsA": (2, lambda s, n, buf: s.write(buf, b"D:\\\0\0") or 4),
        "GetDriveTypeA": (1, lambda s, p: 5),
        "DeleteFileA": (1, lambda s, p: 1),
        "GlobalMemoryStatus": (1, lambda s, p: s.write(p, struct.pack("<8I", 32, 0, *[1 << 30] * 6))),
        "GetTickCount": (0, lambda s: 100000),
        "HeapSize": (3, lambda s, h, f, p: s.sizes.get(p, 0)),
        "HeapAlloc": (3, lambda s, h, f, n: s.alloc(n)),
        "HeapFree": (3, lambda s, h, f, p: 1),
        "HeapReAlloc": (4, lambda s, h, f, p, n: s._realloc(p, n)),
        "GetProcessHeap": (0, lambda s: 1),
        "GetCurrentDirectoryA": (2, lambda s, n, b: s.write(b, b"C:\\CM\0") or 5),
        "SetCurrentDirectoryA": (1, lambda s, p: 1),
        "FindFirstFileA": (2, lambda s, p, d: 0xFFFFFFFF),
        "FindNextFileA": (2, lambda s, h, d: 0),
        "FindClose": (1, lambda s, h: 1),
        "CloseHandle": (1, lambda s, h: 1),
        "MessageBoxA": (4, lambda s, a, b, c, d: 1),
        "OutputDebugStringA": (1, lambda s, p: 0),
        "CreateDirectoryA": (2, lambda s, p, a: 1),
        "RemoveDirectoryA": (1, lambda s, p: 1),
        "GetTempPathA": (2, lambda s, n, b: s.write(b, b"C:\\T\\\0") or 5),
        "CreateFileA": (7, lambda s, *a: 0xFFFFFFFF),
        "GetLocalTime": (1, lambda s, p: s.write(p, struct.pack("<8H", 2001, 7, 0, 14, 12, 0, 0, 0))),
        "GetSystemTime": (1, lambda s, p: s.write(p, struct.pack("<8H", 2001, 7, 0, 14, 12, 0, 0, 0))),
        "GetFileAttributesA": (1, lambda s, p: 0xFFFFFFFF),
        "GetLastError": (0, lambda s: 2),
        "SetLastError": (1, lambda s, e: 0),
        "Sleep": (1, lambda s, ms: 0),
        "timeGetTime": (0, lambda s: 100000),
        "GetModuleHandleA": (1, lambda s, p: 0x400000),
    }

    def _imports(self, exe_path):
        with open(exe_path, "rb") as exe:
            data = exe.read()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        section_count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        sections = [struct.unpack_from("<8sIIII", data, pe + 24 + optional_size + i * 40)[1:]
                    for i in range(section_count)]

        def offset(rva):
            for vsize, va, rsize, rptr in sections:
                if va <= rva < va + max(vsize, rsize):
                    return rptr + rva - va
            raise ValueError(f"rva {rva:#x} is in no section")

        self.uc.mem_map(IMPORT_STUBS, PAGE)
        self.import_names = {}
        descriptor = offset(struct.unpack_from("<I", data, pe + 24 + 104)[0])
        while True:
            lookup, _, _, name, iat = struct.unpack_from("<5I", data, descriptor)
            descriptor += 20
            if name == 0:
                break
            for i in range(1 << 16):
                entry = struct.unpack_from("<I", data, offset(lookup or iat) + 4 * i)[0]
                if entry == 0:
                    break
                if entry & 0x80000000:
                    symbol = f"ordinal {entry & 0xFFFF}"
                else:
                    symbol = data[offset(entry) + 2:].split(b"\0")[0].decode()
                stub = IMPORT_STUBS + 4 * len(self.import_names)
                self.uc.mem_write(stub, b"\xf4")
                self.write_u32(base + iat + 4 * i, stub)
                self.import_names[stub] = symbol
        self.uc.hook_add(UC_HOOK_CODE, self._on_import, begin=IMPORT_STUBS, end=IMPORT_STUBS + PAGE - 1)

    def _on_import(self, uc, address, size, user_data):
        symbol = self.import_names.get(address, "?")
        if symbol not in self.IMPORTS:
            return self._fail(f"import {symbol} has no host function")
        arg_count, host_function = self.IMPORTS[symbol]
        result = host_function(self, *self._stack_args(arg_count))
        sp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, sp + 4 + 4 * arg_count)
        uc.reg_write(UC_X86_REG_EIP, struct.unpack("<I", self.read(sp, 4))[0])

    # Tables

    @staticmethod
    def table_row(pointer, base, record_size, rows):
        """The row of a pointer into a table of the original: -1 for NULL, the row for a pointer to the
        start of a row, and the value itself for anything else, such as an id that the original left
        in a pointer field."""
        if pointer == 0:
            return -1
        offset = pointer - base
        if 0 <= offset < record_size * rows and offset % record_size == 0:
            return offset // record_size
        return pointer
