"""The host bsearch6 against the original's bsearch 0x946464 (bytes from the exe, 0x946464..0x9464f0) with the
original compares, run in Unicorn: eax, the stack bytes (below and above esp, with the argument words that the
original moves) and the callee-saved registers must be equal.
Run: gcc -O1 -shared -fPIC -o bshim6.so bsearch6_shim.c && python3 bsearch6_test.py"""
import ctypes, random, struct, sys
from unicorn import *
from unicorn.x86_const import *

BSEARCH = bytes.fromhex("558bec8b45105356578d78ff8b4d0c0faf7d1403f9894d0c3bcf774b8bd8d1eb74508945108bc38365100175038d43ff0faf451403450c8bf056ff7508ff55185985c05974287d102b7514837d10008bfe750d8d43ffeb0a8b451403f089750c8bc3397d0c76b533c05f5e5b5dc38bc6ebf785c074f1ff750cff7508ff5518f7d81bc059f7d023450c59ebdd")
assert len(BSEARCH) == 0x8c
CMP = {
 0x7c6d40: "8b5424045685d2750583c8ff5ec38b74240c85f67507b8010000005ec38b028b0e3bc175158a520480faff74118a46043cff740a0fbec80fbec22bc15ec333c05ec3",
 0x757ea0: "8b54240456" "85d27505" "83c8ff5ec3" "8b74240c" "85f67507" "b8010000005ec3" "8b028b0e" "3bc17514" "8b420483f8ff7408" "8b4e0483f9ff7504" "33c05ec3" "2bc15ec3",
 0x844740: "8b54240456" "85d27505" "83c8ff5ec3" "8b74240c" "85f67507" "b8010000005ec3" "8b42048b4e04" "3bc17512" "8b0283f8ff7407" "8b0e83f9ff7504" "33c05ec3" "2bc15ec3",
 0x83b670: "8b4c24048b54240856" "8b41088b7208" "2bc65e7508" "8b41048b4a04" "2bc1c3",
}
lib = ctypes.CDLL("./bshim6.so")
lib.run_bsearch6.restype = ctypes.c_uint32
lib.run_bsearch6.argtypes = [ctypes.c_uint32] * 10
SZ = 0x1000000
buf = (ctypes.c_uint8 * SZ)()
lib.setmem(ctypes.cast(buf, ctypes.c_void_p))
for c in CMP: assert lib.has_cmp(c), hex(c)
assert not lib.has_cmp(0x4b0810)

RET = 0x1000
ESP = 0x1f0000
STACK_LO, STACK_HI = ESP - 0x400, ESP + 0x40
WIDTH = {0x7c6d40: 6, 0x757ea0: 8, 0x844740: 8, 0x83b670: 12}
rng = random.Random(4242)
BASE, KEY = 0xb00000, 0xc00000

def row(cmp, keys):
    if cmp == 0x7c6d40:
        return struct.pack("<I", rng.choice(keys)) + bytes([rng.choice([0xff, 0xfe, 0, 1, 2, 3]), rng.getrandbits(8)])
    if cmp in (0x757ea0, 0x844740):
        return struct.pack("<II", rng.choice(keys), rng.choice(keys + [0xffffffff]))
    return struct.pack("<III", rng.getrandbits(32), rng.choice(keys), rng.choice(keys))

def sort_key(cmp, r):
    if cmp == 0x757ea0: return struct.unpack("<II", r)
    if cmp == 0x844740: return struct.unpack("<II", r)[::-1]
    if cmp == 0x83b670: return struct.unpack("<III", r)[2], struct.unpack("<III", r)[1]
    return struct.unpack("<I", r[:4])[0], r[4]

def make(cmp, num):
    w = WIDTH[cmp]
    keys = [0, 1, 2, 3, 5, 7, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff] if rng.random() < 0.3 \
        else list(range(rng.choice([1, 2, 5, max(1, num // 2), num * 3 + 1])))
    rows = [row(cmp, keys) for _ in range(num)]
    if rng.random() < 0.6: rows.sort(key=lambda r: sort_key(cmp, r))
    key = rows[rng.randrange(num)] if num and rng.random() < 0.6 else row(cmp, keys)
    return b"".join(rows), key

def run_original(cmp, num, width, rows, key, stack, r):
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(0x400000, 0x600000); mu.mem_write(0x946464, BSEARCH); mu.mem_write(cmp, bytes.fromhex(CMP[cmp]))
    mu.mem_map(0xa00000, 0x600000); mu.mem_map(0x100000, 0x100000); mu.mem_map(0x1000, 0x1000)
    mu.mem_write(0x1000, b"\x90"); mu.mem_write(STACK_LO, stack); mu.mem_write(BASE, rows); mu.mem_write(KEY, key)
    ebp, ebx, esi, edi, ecx = r
    for reg, v in ((UC_X86_REG_ESP, ESP), (UC_X86_REG_EBP, ebp), (UC_X86_REG_EBX, ebx), (UC_X86_REG_ESI, esi),
                   (UC_X86_REG_EDI, edi), (UC_X86_REG_ECX, ecx), (UC_X86_REG_EAX, 0x5a5a5a5a), (UC_X86_REG_EDX, 0x33)):
        mu.reg_write(reg, v)
    mu.emu_start(0x946464, RET, count=2_000_000)
    return (mu.reg_read(UC_X86_REG_EAX), bytes(mu.mem_read(STACK_LO, STACK_HI - STACK_LO)),
            [mu.reg_read(g) for g in (UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP)])

def run_host(cmp, num, width, rows, key, stack, r):
    ctypes.memset(buf, 0, SZ)
    ctypes.memmove(ctypes.addressof(buf) + STACK_LO, stack, len(stack))
    ctypes.memmove(ctypes.addressof(buf) + BASE, rows, len(rows))
    ctypes.memmove(ctypes.addressof(buf) + KEY, key, len(key))
    ebp, ebx, esi, edi, ecx = r
    got = lib.run_bsearch6(KEY, BASE, num, width, cmp, ESP, ebp, ebx, esi, edi)
    return got, bytes(buf[STACK_LO:STACK_HI])

bad = {"eax": 0, "stack": 0, "regs": 0}
total, found, steps = {}, 0, 0
for cmp in CMP:
    w = WIDTH[cmp]
    n = 0
    for num in list(range(0, 40)) + [63, 64, 65, 100, 127, 200, 333, 1000]:
        for k in range(10 if num < 100 else 4):
            rows, key = make(cmp, num) if num else (b"", row(cmp, [0, 1, 2]))
            r = [rng.getrandbits(32) for _ in range(5)]
            stack = bytearray(rng.getrandbits(8) for _ in range(STACK_HI - STACK_LO))
            stack[ESP - STACK_LO:ESP - STACK_LO + 24] = struct.pack("<IIIIII", RET, KEY, BASE, num, w, cmp)
            stack = bytes(stack)
            o_eax, o_stack, o_regs = run_original(cmp, num, w, rows, key, stack, r)
            h_eax, h_stack = run_host(cmp, num, w, rows, key, stack, r)
            n += 1; found += h_eax != 0
            if o_eax != h_eax: bad["eax"] += 1; print("eax differs", hex(cmp), num, k, hex(o_eax), hex(h_eax))
            elif o_stack != h_stack:
                bad["stack"] += 1
                at = next(i for i in range(len(o_stack)) if o_stack[i] != h_stack[i])
                print("stack differs", hex(cmp), num, k, "esp%+d" % (STACK_LO + at - ESP))
            if o_regs != r[:4] + [ESP + 4]: bad["regs"] += 1; print("regs not restored", hex(cmp), num, k)
    total[hex(cmp)] = n
print("searches", total, "found", found, "mismatches", bad)
sys.exit(1 if any(bad.values()) else 0)
