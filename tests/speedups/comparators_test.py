import ctypes, random, struct, math
from unicorn import *
from unicorn.x86_const import *
FN = {
 0x53fd40: "81ec080200008b84240c0200008b8c2410020000dd05a08996008b008b0985c0c744240000000000c744240400000000746985c974658b406985c074058b5038eb0233d28b416985c074058b4038eb0233c085d27405ddd8dd422385c0740e8b50238b40278954240089442404dc542400dfe0f6c401740cddd883c8ff81c408020000c3dc5c2400dfe0f6c4417571b80100000081c408020000c38d4c24088d94240801000051526a006a0068c85a9b00ddd8e8074f400083c4148d4424088d8c240801000068e73f00005051e81ed33c005068e870980068d024ae00e87d4e40006a0068d024ae0068e0709800e85d840a0083c424c705347ab6000000000033c081c408020000c3",
 0x7c6d90: "568b74240885f67507b8010000005ec38b54240c85d2750583c8ff5ec38b460a8b4a0a3bc17607b8010000005ec3730583c8ff5ec38b46048b4a043bc17607b8010000005ec32bc15ef7d81bc0c3",
 0x7c6de0: "568b74240885f67507b8010000005ec38b54240c85d274318b46048b4a043bc17607b8010000005ec3721e668b4608668b4a08663bc17e07b8010000005ec375088b068b0a2bc15ec383c8ff5ec3",
 0x778ed0: "8b4424048b088b51618b4c24080fbf42098b118b4a610fbf51092bd08bc2c3",
}
KIND = {0x53fd40: 2, 0x7c6d90: 3, 0x7c6de0: 4, 0x778ed0: 5}
lib = ctypes.CDLL("./shim.so")
SZ = 0x1000000
buf = (ctypes.c_uint8*SZ)()
lib.setmem(ctypes.cast(buf, ctypes.c_void_p))
for c,k in KIND.items(): assert lib.kind_of(c)==k
RET = 0x1000
def run(addr, code, mem, a, b, uses_x87):
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(0x400000, 0x600000)           # code region 0x400000..0xa00000, holds g_f64_0_0 (0x9689a0)
    mu.mem_write(addr, code)
    mu.mem_map(0xa00000, 0x600000)           # the data (0xa00000.. 0x1000000)
    mu.mem_map(0x100000, 0x100000)           # stack and return address
    mu.mem_map(0x1000, 0x1000)
    for off, data in mem.items(): mu.mem_write(off, data)
    mu.mem_write(0x1000, b"\x90")            # the return address is a nop; we stop there
    esp = 0x1f0000
    mu.mem_write(esp, struct.pack("<III", RET, a, b))
    mu.reg_write(UC_X86_REG_ESP, esp)
    mu.mem_write(0x9689a0, struct.pack("<d", 0.0))
    mu.emu_start(addr, RET, count=2000)
    return struct.unpack("<i", struct.pack("<I", mu.reg_read(UC_X86_REG_EAX)))[0]
rng = random.Random(5)
bad = 0; n = 0
def vals():
    return rng.choice([0.0, 1.5, -1.5, 51.5, 51.5, 40.7, -3.2, float("nan"), float("inf"), -0.0, 1e-300])
for it in range(6000):
    mem = {}
    # clubs: a table of cities (0xb00000..), stadiums, clubs, and the arrays of pointers
    cities = [0xb00000 + 0x40*i for i in range(6)]
    for c in cities: mem[c+0x23] = struct.pack("<d", vals())
    stad = [0xb10000 + 0x80*i for i in range(6)]
    for i,s_ in enumerate(stad): mem[s_+0x38] = struct.pack("<I", rng.choice(cities + [0]))
    clubs = [0xb20000 + 0x100*i for i in range(6)]
    for c in clubs: mem[c+0x69] = struct.pack("<I", rng.choice(stad + [0]))
    ptrs = 0xb30000
    mem[ptrs] = struct.pack("<II", rng.choice(clubs), rng.choice(clubs))
    # history records
    hist = [0xb40000 + 0x20*i for i in range(6)]
    for h in hist:
        mem[h] = struct.pack("<I", rng.choice([5, 6, 7, 0x80000000, 0xffffffff]))
        mem[h+4] = struct.pack("<I", rng.choice([1, 2, 0x90000000, 0xfffffff0]))
        mem[h+8] = struct.pack("<h", rng.choice([-5, 0, 1, 1999, 32767, -32768]))
        mem[h+0xa] = struct.pack("<I", rng.choice([1, 2, 0x80000001, 0xffffffff]))
    # staff: pointer -> staff -> player data (+0x61) -> word at +9
    pds = [0xb50000 + 0x40*i for i in range(6)]
    for p in pds: mem[p+9] = struct.pack("<h", rng.choice([-20, 0, 5, 100, 200, 30000, -30000]))
    staff = [0xb60000 + 0x100*i for i in range(6)]
    for s_ in staff: mem[s_+0x61] = struct.pack("<I", rng.choice(pds))
    sp = 0xb70000
    mem[sp] = struct.pack("<II", rng.choice(staff), rng.choice(staff))
    for addr, hexcode in FN.items():
        code = bytes.fromhex(hexcode)
        if addr == 0x53fd40:
            a, b = ptrs, ptrs + 4
            if (struct.unpack("<II", mem[ptrs])[0] == 0) : continue
        elif addr == 0x778ed0:
            a, b = sp, sp + 4
        else:
            a = rng.choice(hist + [0]); b = rng.choice(hist + [0])
        # native on a copy of the same memory
        ctypes.memset(buf, 0, SZ)
        for off, data in mem.items(): ctypes.memmove(ctypes.addressof(buf)+off, data, len(data))
        r = ctypes.c_int32()
        ok = lib.shim(KIND[addr], a, b, ctypes.byref(r))
        want = run(addr, code, mem, a, b, addr == 0x53fd40)
        n += 1
        if not ok or r.value != want:
            bad += 1
            if bad < 8: print("MISMATCH", hex(addr), hex(a), hex(b), "native", r.value, "ok", ok, "original", want)
print("pairs", n, "mismatches", bad)
