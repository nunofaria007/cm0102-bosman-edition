"""Runs functions of the original (cm0102.exe) in the Unicorn emulator.

The DirectDraw back buffer is replaced with a fake surface whose Lock, Unlock
and Blt write to an 800x600 RGB565 buffer in emulated memory.
"""
import hashlib
import struct

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

WIDTH, HEIGHT = 800, 600
PITCH = WIDTH * 2
FB_SIZE = PITCH * HEIGHT

EXPECTED_SHA256 = "0c78e7da6d0bcb4603fa179449bbe6d1b6b5e1c627a22c445d14d489bd9bff86"

PAGE = 0x1000
STACK_BASE, STACK_SIZE = 0x20000000, 0x100000
HEAP_BASE, HEAP_SIZE = 0x30000000, 0x1000000
RETURN_ADDR = 0x40000000
FB_BASE = 0x50000000
SURFACE = 0x60000000
VTABLE = SURFACE + 0x100
STUBS = 0x61000000
STUB_STRIDE = 8

# Globals of the original.
G_DISPLAY_OFF = 0xAEBE0C     # non-zero disables every drawing function
G_BACK_BUFFER = 0xAEBDE4     # IDirectDrawSurface *
G_SCREEN_W = 0xAEBE08
G_SCREEN_H = 0xAEBDD0
G_PIXEL_FORMAT = 0xAE3158    # DDPIXELFORMAT, 32 bytes

FUN_INIT_PALETTE = 0x5E4560  # sets the colour globals from the pixel format

# CRT functions replaced with host-side versions.
CRT_FREE = 0x9452CA
CRT_CALLOC = 0x945501
CRT_FCLOSE = 0x945A09
CRT_FOPEN = 0x945B00
CRT_FREAD = 0x945BFE

# IDirectDrawSurface vtable slots used by the drawing code: slot -> stdcall argument bytes.
SLOT_BLT, SLOT_LOCK, SLOT_UNLOCK = 0x14 // 4, 0x64 // 4, 0x80 // 4
SLOT_ARG_BYTES = {SLOT_BLT: 0x18, SLOT_LOCK: 0x14, SLOT_UNLOCK: 0x08}

DDBLT_COLORFILL = 0x400


def _align(n):
    return (n + PAGE - 1) & ~(PAGE - 1)


class Original:
    def __init__(self, exe_path):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.error = None
        self._load_pe(exe_path)
        uc = self.uc
        uc.mem_map(STACK_BASE, STACK_SIZE)
        uc.mem_map(HEAP_BASE, HEAP_SIZE)
        uc.mem_map(RETURN_ADDR, PAGE)
        uc.mem_map(FB_BASE, _align(FB_SIZE))
        uc.mem_map(SURFACE, PAGE)
        uc.mem_map(STUBS, PAGE)
        self.heap_top = HEAP_BASE

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
        self._replace(CRT_FREE, 1, lambda block: 0)
        self._replace(CRT_FOPEN, 2, self._fopen)
        self._replace(CRT_FREAD, 4, self._fread)
        self._replace(CRT_FCLOSE, 1, self._fclose)

        self.write_u32(G_DISPLAY_OFF, 0)
        self.write_u32(G_BACK_BUFFER, SURFACE)
        self.write_u32(G_SCREEN_W, WIDTH)
        self.write_u32(G_SCREEN_H, HEIGHT)
        uc.mem_write(G_PIXEL_FORMAT, struct.pack("<8I", 32, 0x40, 0, 16, 0xF800, 0x07E0, 0x001F, 0))
        self.call(FUN_INIT_PALETTE)

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
        table = pe + 24 + optional_size
        for i in range(section_count):
            _, vsize, va, rsize, rptr = struct.unpack_from("<8sIIII", data, table + i * 40)
            self.uc.mem_write(base + va, data[rptr:rptr + min(rsize, vsize or rsize)])

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
        return text.decode()

    def alloc_c_string(self, text):
        data = text.encode() + b"\0"
        addr = self.alloc(len(data))
        self.write(addr, data)
        return addr

    def alloc(self, size):
        addr = self.heap_top
        self.heap_top += (size + 15) & ~15
        if self.heap_top > HEAP_BASE + HEAP_SIZE:
            raise MemoryError("emulated heap is full")
        self.write(addr, bytes(size))
        return addr

    def reset_heap(self):
        self.heap_top = HEAP_BASE

    def framebuffer(self):
        return self.read(FB_BASE, FB_SIZE)

    def set_framebuffer(self, data):
        assert len(data) == FB_SIZE
        self.write(FB_BASE, data)

    # Calls

    def call(self, addr, *args):
        """Calls a cdecl function. Returns EAX."""
        sp = STACK_BASE + STACK_SIZE - 0x1000
        for arg in reversed(args):
            sp -= 4
            self.write_u32(sp, arg)
        sp -= 4
        self.write_u32(sp, RETURN_ADDR)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.error = None
        try:
            self.uc.emu_start(addr, RETURN_ADDR, timeout=60 * 1000 * 1000)
        except UcError as exc:
            raise RuntimeError(f"emulation of {addr:#x} failed: {exc}") from exc
        if self.error:
            raise RuntimeError(f"emulation of {addr:#x} failed: {self.error}")
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN_ADDR:
            raise RuntimeError(f"emulation of {addr:#x} did not return in time")
        return self.uc.reg_read(UC_X86_REG_EAX)

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

    def _replace(self, addr, arg_count, host_function):
        """Makes a cdecl function of the original return the result of a host function."""
        def on_call(uc, address, size, user_data):
            uc.reg_write(UC_X86_REG_EAX, host_function(*self._stack_args(arg_count)))
        self.uc.mem_write(addr, b"\xc3")
        self.uc.hook_add(UC_HOOK_CODE, on_call, begin=addr, end=addr)

    def _fopen(self, path, mode):
        name = self.read_c_string(path)
        try:
            handle = open(name, "rb")
        except OSError:
            return 0
        stream = self.alloc(4)
        self.files[stream] = handle
        return stream

    def _fread(self, buffer, size, count, stream):
        data = self.files[stream].read(size * count)
        self.write(buffer, data)
        return len(data) // size if size else 0

    def _fclose(self, stream):
        self.files.pop(stream).close()
        return 0
