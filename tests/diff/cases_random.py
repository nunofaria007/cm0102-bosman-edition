"""The random numbers of utils.cpp: rand and srand of MSVC, the seed and the generator that draws from the table
of 51,000 numbers in the exe. The port reads that table from the same exe."""
import os
import struct
import tempfile

from unicorn.x86_const import UC_X86_REG_FPCW

CRT_SRAND = 0x9469FA             # cdecl (seed)
CRT_RAND = 0x946A04              # cdecl (), 0 to 0x7fff
FUN_RANDOM = 0x90CFC0            # cdecl (n)
FUN_RANDOM_SEED = 0x90D0A0       # cdecl (seed), the time for 0

G_HOLDRAND = 0xAD6C30
G_POINTER = 0xDE1F60             # into the table
G_END = 0xDE2798                 # 0 before the seed, then the last number of the table
G_OFFSET = 0xDE1F5C
TABLE = 0xAA2454
TABLE_SIZE = 51000
END = TABLE + 4 * (TABLE_SIZE - 1)

# The MSVC runtime of the original sets the x87 precision to 53 bits at start, as for a double. The test
# does not run that start, so it sets the control word itself.
FPU_CONTROL = 0x27F

# A loop in emulated memory that calls the generator once for each count: cdecl (number, counts, results).
LOOP = 0x66000000
LOOP_CODE = (b"\x53\x56\x57"                      # push ebx, esi, edi
             b"\x8b\x5c\x24\x10\x8b\x74\x24\x14\x8b\x7c\x24\x18"
             b"\x85\xdb\x74\x15"                  # loop: test ebx, ebx; jz done
             b"\xff\x36"                          # push dword [esi]
             b"\xe8" + struct.pack("<i", FUN_RANDOM - (LOOP + 26)) +
             b"\x83\xc4\x04\x89\x07\x83\xc6\x04\x83\xc7\x04\x4b\xeb\xe7"
             b"\x5f\x5e\x5b\xc3")                 # done: pop edi, esi, ebx; ret
BATCH = 1000


def _setup(ctx):
    original = ctx.original
    if not getattr(ctx.port, "random_table", False):
        ctx.port.send("rn_load", original.exe_path)
        ctx.port.reply()
        ctx.port.random_table = True
    if not getattr(original, "random_loop", False):
        original.uc.mem_map(LOOP, 0x1000)
        original.write(LOOP, LOOP_CODE)
        original.random_loop = True
    original.uc.reg_write(UC_X86_REG_FPCW, FPU_CONTROL)


def _s32(value):
    value &= 0xFFFFFFFF
    return value - (1 << 32) if value & 0x80000000 else value


def _original_state(original):
    holdrand, pointer, offset = (struct.unpack("<I", original.read(a, 4))[0] for a in (G_HOLDRAND, G_POINTER, G_OFFSET))
    end = struct.unpack("<I", original.read(G_END, 4))[0]
    return [holdrand, _s32(pointer - TABLE) // 4, -1 if end == 0 else (end - TABLE) // 4, _s32(offset)]


def _set_state(ctx, holdrand, index, offset):
    original = ctx.original
    original.write_u32(G_HOLDRAND, holdrand)
    original.write_u32(G_POINTER, TABLE + 4 * index)
    original.write_u32(G_END, END)
    original.write_u32(G_OFFSET, offset)
    ctx.port.send("rn_state", holdrand, index, TABLE_SIZE - 1, offset)


def _expect_same_state(ctx, case):
    ctx.port.send("rn_get")
    ctx.expect_equal(f"{case}: holdrand, index, end, offset", _original_state(ctx.original),
                     [int(word) for word in ctx.port.reply()])


def _count(ctx):
    roll = ctx.rng.random()
    if roll < 0.5:
        return ctx.rng.randint(1, 100)
    if roll < 0.7:
        return ctx.rng.randint(-0xFFFF, 0xFFFF)
    if roll < 0.8:
        return ctx.rng.choice([0, 1, -1, 2, 0xFFFF, -0xFFFF, 0x10000, -0x10000, 0x7FFFFFFF, -0x80000000,
                               214748364, 214748365, 0x7FFF, 0x8000])
    if roll < 0.9:
        return ctx.rng.randint(0x10000, 0x7FFFFFFF)
    return ctx.rng.randint(-0x80000000, -0x10000)


def _run(ctx, counts, case):
    """Calls the generator once for each count in both programs and compares the numbers."""
    original = ctx.original
    original.reset_heap()
    table = original.alloc_bytes(struct.pack(f"<{len(counts)}i", *counts))
    results = original.alloc(4 * len(counts))
    original.call(LOOP, len(counts), table, results)
    wanted = list(struct.unpack(f"<{len(counts)}i", original.read(results, 4 * len(counts))))
    ctx.port.send("rn_run", len(counts), *counts)
    got = [int(word) for word in ctx.port.reply()]
    if wanted != got:
        first = next(i for i in range(len(counts)) if wanted[i] != got[i])
        ctx.expect_equal(f"{case}: number {first}, count {counts[first]}", wanted[first], got[first])


def test_random_load(ctx):
    """The port reads its table from the exe, so the table equals that of the original."""
    exe = ctx.original.exe_path
    ctx.port.send("rn_load", exe)
    ctx.expect_equal("random_load: loads", b"1", ctx.port.reply()[0], source="wanted")
    ctx.port.random_table = True
    ctx.port.send("rn_table", 0, 5, TABLE_SIZE - 1)
    wanted = [struct.unpack("<i", ctx.original.read(TABLE + 4 * i, 4))[0] for i in (0, 5, TABLE_SIZE - 1)]
    ctx.expect_equal("random_load: numbers 0, 5 and the last", wanted, [int(w) for w in ctx.port.reply()])
    ctx.port.send("rn_load", exe + ".missing")
    ctx.expect_equal("random_load: a missing file fails", b"0", ctx.port.reply()[0], source="wanted")
    ctx.port.send("rn_load", os.path.join(ctx.data_dir, "arial_14.fnt"))
    ctx.expect_equal("random_load: another file fails", b"0", ctx.port.reply()[0], source="wanted")
    # An exe with one byte changed has another hash, so it fails too.
    with tempfile.TemporaryDirectory(dir=ctx.original.sandbox) as folder:
        changed = os.path.join(folder, "changed.exe")
        with open(exe, "rb") as src:
            data = bytearray(src.read())
        data[0x6a2454] ^= 1   # the first table number, at its file offset
        with open(changed, "wb") as out:
            out.write(data)
        ctx.port.send("rn_load", changed)
        ctx.expect_equal("random_load: a changed exe fails", b"0", ctx.port.reply()[0], source="wanted")
    # A failed load keeps the table.
    ctx.port.send("rn_table", 0, 5, TABLE_SIZE - 1)
    ctx.expect_equal("random_load: table kept", wanted, [int(w) for w in ctx.port.reply()])
    return 6


def test_rand(ctx):
    original = ctx.original
    cases = 0
    for case in range(200):
        seed = ctx.rng.randint(0, 0xFFFFFFFF)
        original.call(CRT_SRAND, seed)
        wanted = [original.call(CRT_RAND) for _ in range(5)]
        ctx.port.send("rn_srand", seed, 5)
        ctx.expect_equal(f"rand case {case}, seed {seed}", wanted, [int(w) for w in ctx.port.reply()])
        cases += 1
    return cases


def test_random_seed(ctx):
    original = ctx.original
    cases = 0
    for case in range(300):
        # Seed 0 takes the time, which the test cannot give the original.
        seed = ctx.rng.choice([1, -1, 12345, ctx.rng.randint(1, 0x7FFFFFFF), ctx.rng.randint(-0x80000000, -1)])
        original.write_u32(G_END, 0)
        original.call(FUN_RANDOM_SEED, seed & 0xFFFFFFFF)
        ctx.port.send("rn_seed", seed)
        ctx.port.reply()
        _expect_same_state(ctx, f"random_seed case {case}, seed {seed}")
        cases += 1
    ctx.port.send("rn_seed", 12345)
    ctx.expect_equal("random_seed: log text", [b"Random", b"Number", b"seed:", b"12345"], ctx.port.reply(),
                     source="wanted")
    return cases


def test_random_unseeded(ctx):
    """Before the seed the end pointer is 0, so every call starts the table again with a new offset."""
    original = ctx.original
    _setup(ctx)
    for case in range(20):
        holdrand = ctx.rng.randint(0, 0xFFFFFFFF)
        for address, value in ((G_HOLDRAND, holdrand), (G_POINTER, 0), (G_END, 0), (G_OFFSET, 0)):
            original.write_u32(address, value)
        ctx.port.send("rn_state", holdrand, 0, -1, 0)
        counts = [_count(ctx) or 1 for _ in range(3)]
        _run(ctx, counts, f"random_unseeded case {case}")
        state = _original_state(original)
        ctx.port.send("rn_get")
        got = [int(word) for word in ctx.port.reply()]
        # The end stays 0 in the original and -1 in the port: the same meaning.
        ctx.expect_equal(f"random_unseeded case {case}: state", state, got)
    return 20


def test_random(ctx):
    """100,000 numbers in runs of 1,000 from set states, with the state compared after each run."""
    original = ctx.original
    _setup(ctx)
    total = 0
    for run in range(100):
        roll = ctx.rng.random()
        index = ctx.rng.randint(0, TABLE_SIZE - 1) if roll < 0.8 else ctx.rng.choice([0, TABLE_SIZE - 1, TABLE_SIZE - 2])
        _set_state(ctx, ctx.rng.randint(0, 0xFFFFFFFF), index, ctx.rng.randint(0, 0xFFFF))
        counts = [_count(ctx) for _ in range(BATCH)]
        _run(ctx, counts, f"random run {run}")
        _expect_same_state(ctx, f"random run {run}")
        total += len(counts)
    return total


def test_random_wrap(ctx):
    """Long runs from the seed, which pass the end of the table and start it again."""
    original = ctx.original
    _setup(ctx)
    seed = 4242
    original.call(FUN_RANDOM_SEED, seed)
    ctx.port.send("rn_seed", seed)
    ctx.port.reply()
    total = 0
    for run in range(60):
        counts = [ctx.rng.choice([10, 100, 1000, 0x10000 + run, 0]) for _ in range(BATCH)]
        _run(ctx, counts, f"random_wrap run {run}")
        total += len(counts)
    _expect_same_state(ctx, "random_wrap")
    return total


TESTS = [
    ("random_load", test_random_load),
    ("rand", test_rand),
    ("random_seed", test_random_seed),
    ("random_unseeded", test_random_unseeded),
    ("random", test_random),
    ("random_wrap", test_random_wrap),
]
