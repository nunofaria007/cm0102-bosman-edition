"""The host qsort6 with each host compare of qsort6_cmp.c against the original's qsort 0x945333 (bytes from the
exe, 0x945333..0x945501, and the compares) run in Unicorn: the rows, eax and the stack bytes must be equal.
Run: gcc -O1 -shared -fPIC -o shim6.so qsort6_general_shim.c && python3 qsort6_general_test.py"""
import ctypes, random, struct, sys
from unicorn import *
from unicorn.x86_const import *

QSORT = bytes.fromhex(open(__file__.replace("qsort6_general_test.py", "qsort_original.hex")).read().strip())
CMP = {
 0x7c6d40: "8b5424045685d2750583c8ff5ec38b74240c85f67507b8010000005ec38b028b0e3bc175158a520480faff74118a46043cff740a0fbec80fbec22bc15ec333c05ec3",
}
# the compares from IDA (the bytes of the list in comparators_test.py of the perf branch)
CMP.update({
 0x757ea0: "8b54240456" "85d27505" "83c8ff5ec3" "8b74240c" "85f67507" "b8010000005ec3" "8b028b0e" "3bc17514" "8b420483f8ff7408" "8b4e0483f9ff7504" "33c05ec3" "2bc15ec3",
 0x844740: "8b54240456" "85d27505" "83c8ff5ec3" "8b74240c" "85f67507" "b8010000005ec3" "8b42048b4e04" "3bc17512" "8b0283f8ff7407" "8b0e83f9ff7504" "33c05ec3" "2bc15ec3",
 0x83b670: "8b4c24048b54240856" "8b41088b7208" "2bc65e7508" "8b41048b4a04" "2bc1c3",
 0x4b0810: "8b4424048b5424088b088b0285c9750885c00f95c18bc1c385c0750483c8ffc30fbf90800000000fbf81800000002bc2c3",
})
lib = ctypes.CDLL("./shim6.so")
lib.run_qsort6.restype = ctypes.c_uint32
lib.run_qsort6.argtypes = [ctypes.c_uint32] * 9
SZ = 0x1000000
buf = (ctypes.c_uint8 * SZ)()
lib.setmem(ctypes.cast(buf, ctypes.c_void_p))
CMP = {c: code for c, code in CMP.items() if lib.has_cmp(c)}     # the compares that qsort6_host_cmp lists
assert 0x7c6d40 in CMP

RET = 0x1000
ESP = 0x1f0000
STACK_LO, STACK_HI = ESP - 0x400, ESP + 0x40      # what the test fills and compares
WIDTH = {0x7c6d40: 6, 0x757ea0: 8, 0x844740: 8, 0x83b670: 12, 0x4b0810: 4}
rng = random.Random(77)

def regs():
    return [rng.getrandbits(32) for _ in range(5)]   # ebp ebx esi edi ecx

def make_rows(cmp, num, ids):
    """rows with many equal keys; also the special values of each compare; returns (bytes, extra memory)"""
    w = WIDTH[cmp]
    extra = {}
    rows = bytearray()
    if cmp == 0x4b0810:
        clubs = [0xc00000 + 0x100 * i for i in range(max(2, ids))]
        for c in clubs:
            extra[c + 0x80] = struct.pack("<h", rng.choice([-32768, -5, 0, 0, 7, 7, 100, 32767, rng.randint(-50, 50)]))
        for _ in range(num):
            rows += struct.pack("<I", rng.choice(clubs + ([0] if rng.random() < 0.15 else [])))
        return bytes(rows), extra
    keys = [0, 1, 2, 3, 5, 7, 0x7fffffff, 0x80000000, 0x80000001, 0xfffffffe, 0xffffffff] if rng.random() < 0.3 \
        else list(range(ids))
    for _ in range(num):
        if cmp == 0x7c6d40:
            r = struct.pack("<I", rng.choice(keys)) + bytes([rng.choice([0xff, 0xfe, 0, 1, 2, 3]), rng.getrandbits(8)])
        elif cmp in (0x757ea0, 0x844740):
            r = struct.pack("<II", rng.choice(keys), rng.choice(keys + [0xffffffff]))
        else:
            r = struct.pack("<III", rng.getrandbits(32), rng.choice(keys), rng.choice(keys))
        rows += r
    return bytes(rows), extra

def run_original(cmp, base, num, width, rows, extra, stack, r):
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(0x400000, 0x600000)
    mu.mem_write(0x945333, QSORT)
    mu.mem_write(cmp, bytes.fromhex(CMP[cmp]))
    mu.mem_map(0xa00000, 0x600000)
    mu.mem_map(0x100000, 0x100000)
    mu.mem_map(0x1000, 0x1000)
    mu.mem_write(0x1000, b"\x90")
    mu.mem_write(STACK_LO, stack)
    for off, d in extra.items(): mu.mem_write(off, d)
    mu.mem_write(base, rows)
    mu.mem_write(ESP, struct.pack("<IIIII", RET, base, num, width, cmp))
    ebp, ebx, esi, edi, ecx = r
    for reg, v in ((UC_X86_REG_ESP, ESP), (UC_X86_REG_EBP, ebp), (UC_X86_REG_EBX, ebx), (UC_X86_REG_ESI, esi),
                   (UC_X86_REG_EDI, edi), (UC_X86_REG_ECX, ecx), (UC_X86_REG_EAX, 0x5a5a5a5a), (UC_X86_REG_EDX, 0x33)):
        mu.reg_write(reg, v)
    mu.emu_start(0x945333, RET, count=20_000_000)
    return (bytes(mu.mem_read(base, num * width)), mu.reg_read(UC_X86_REG_EAX), bytes(mu.mem_read(STACK_LO, STACK_HI - STACK_LO)),
            mu.reg_read(UC_X86_REG_ECX), mu.reg_read(UC_X86_REG_EDX))

def run_host(cmp, base, num, width, rows, extra, stack, r):
    ctypes.memset(buf, 0, SZ)
    ctypes.memmove(ctypes.addressof(buf) + STACK_LO, stack, len(stack))
    for off, d in extra.items(): ctypes.memmove(ctypes.addressof(buf) + off, d, len(d))
    ctypes.memmove(ctypes.addressof(buf) + base, rows, len(rows))
    ebp, ebx, esi, edi, ecx = r
    got = lib.run_qsort6(base, num, width, cmp, ESP, ebp, ebx, esi, edi)
    # the original pushes its arguments in the caller; the test puts them in the stack image as the caller does
    return (bytes(buf[base:base + num * width]), got, bytes(buf[STACK_LO:STACK_HI]))

bad = {"rows": 0, "eax": 0, "stack": 0}
total = {}
ecx_edx_differs = 0
seen = 0
sizes = list(range(0, 40)) + [40, 63, 64, 65, 100, 127, 200, 333]
for cmp in CMP:
    w = WIDTH[cmp]
    n_sorts = 0
    for num in sizes:
        for k in range(8 if num < 100 else 3):
            ids = rng.choice([1, 2, 3, 5, max(1, num // 2), num * 3 + 1])
            rows, extra = make_rows(cmp, num, ids)
            base = 0xb00000
            r = regs()
            stack = bytes(rng.getrandbits(8) for _ in range(STACK_HI - STACK_LO))
            stack = bytearray(stack)
            # the caller's frame: the return address and the four arguments at esp
            stack[ESP - STACK_LO:ESP - STACK_LO + 20] = struct.pack("<IIIII", RET, base, num, w, cmp)
            stack = bytes(stack)
            o_rows, o_eax, o_stack, o_ecx, o_edx = run_original(cmp, base, num, w, rows, extra, stack, r)
            h_rows, h_eax, h_stack = run_host(cmp, base, num, w, rows, extra, stack, r)
            n_sorts += 1
            want = h_eax if h_eax else 0x5a5a5a5a
            if o_rows != h_rows: bad["rows"] += 1; print("rows differ", hex(cmp), num, k)
            elif o_eax != want: bad["eax"] += 1; print("eax differs", hex(cmp), num, k, hex(o_eax), hex(want))
            elif o_stack != h_stack:
                bad["stack"] += 1
                at = next(i for i in range(len(o_stack)) if o_stack[i] != h_stack[i])
                print("stack differs", hex(cmp), num, k, "esp%+d" % (STACK_LO + at - ESP))
            if num >= 2 and (o_ecx != r[4] or o_edx != 0x33): ecx_edx_differs += 1
    total[hex(cmp)] = n_sorts
print("sorts", total, "mismatches", bad, "(ecx or edx left changed by the original in %d sorts)" % ecx_edx_differs)
sys.exit(1 if any(bad.values()) else 0)
