"""The database load: FUN_00528540 of the original in the emulator, and the tables of the port.

The original loads the database once for each run of the test, with code hooks that copy every table
at the steps of the load. Its tables stay below the heap floor, so that later tests can read them.
"""
import array
import struct

from unicorn import UC_HOOK_CODE

FUN_LOAD = 0x528540                  # (data folder with a trailing separator, shared, from stream): 1 when it worked.
                                     # FUN_00524d50 calls it with (folder, 1, 0). The select suite replaces that one
FUN_PROGRESS = 0x802F80              # thiscall on the screen stack: (text, step, total, channel)
FUN_RANDOM = 0x90CFC0                # the load must not draw a random number with the shipped data
G_SETTINGS = 0xAE3138                # pointer to the settings object. +0x08 is the language
G_LANGUAGE = 0x9EB2DC
G_KEY_IDS = (0x9CF178, 0x9D0528)     # all -1 when the original starts
G_EMPTY_COMMON_NAME = 0x9B59B4       # -1 when the original starts
# Code that other suites patch and the load runs: the date constructor (cases_leagues).
PATCHED_CODE = [(0x549F70, 5)]

# Steps of the load where the test copies the tables: the call of the id pass, the instruction after
# the delete of duff.out (the id pass is done), and the call of the key ids (the staff pass and the
# national clubs are done).
AT_READ, AT_RESOLVED, AT_STAFF_PASS = 0x52D1AA, 0x52D20A, 0x52D8FA

# Count globals after the load, from the game creation inventory, section 2b.
LOAD_COUNTS = {
    0xAE2340: 1,          # shared
    0xAE2344: 127,        # reserve competition rows
    0xAE2348: 16,         # manager rows
    0xAE234C: 6,          # continents
    0xAE2350: 213,        # nations
    0xAE2354: 6682,       # cities
    0xAE2358: 8917,       # stadiums
    0xAE235C: 13640,      # clubs with the national clubs
    0xAE2360: 426,        # national clubs
    0xAE2364: 192358,     # staff
    0xAE2368: 53881,      # non-player rows
    0xAE236C: 139769,     # player rows
    0xAE2370: 169358,     # preferences
    0xAE2374: 385846,     # staff history
    0xAE2378: 579,        # club comps with the nation comps and the reserve rows
    0xAE237C: 11039,      # comp history with the nation comp history
    0xAE2380: 19,         # nation comps
    0xAE2384: 273,        # nation comp history
    0xAE2388: 543,        # staff comps
    0xAE238C: 2609,       # staff comp history
    0xAE2390: 4567,       # officials
    0xAE2394: 34,         # colours
    0xAE2398: 44297,      # first names
    0xAE239C: 104411,     # second names
    0xAE23A0: 13272,      # common names
}
# The globals that the port prints with db_counts, in its order.
COUNTS = [0xAE2340, 0xAE2344, 0xAE2348, 0xAE234C, 0xAE2350, 0xAE2354, 0xAE2358, 0xAE235C, 0xAE2360,
          0xAE2364, 0xAE2368, 0xAE236C, 0xAE2370, 0xAE2374, 0xAE2378, 0xAE237C, 0xAE2380, 0xAE2384,
          0xAE2388, 0xAE238C, 0xAE2390, 0xAE2394, 0xAE2398, 0xAE239C, 0xAE23A0]

# Table id -> (table global, count global, record size). The tables that the test compares.
TABLES = {
    0: (0xAE23B4, 0xAE235C, 0x245),      # clubs with the national clubs
    2: (0xAE23EC, 0xAE2394, 0x3A),       # colours
    3: (0xAE23A4, 0xAE234C, 0xC6),       # continents
    4: (0xAE23A8, 0xAE2350, 0x122),      # nations
    5: (0xAE23B0, 0xAE2358, 0x4E),       # stadiums
    6: (0xAE23BC, 0xAE2364, 0x6E),       # staff
    7: (0xAE23E8, 0xAE2390, 0x2B),       # officials
    9: (0xAE23C0, 0xAE2368, 0x44),       # non-player rows
    10: (0xAE23C4, 0xAE236C, 0x46),      # player rows
    11: (0xAE23E0, 0xAE2388, 0x65),      # staff comps
    12: (0xAE23D0, 0xAE2378, 0x6B),      # club comps with the nation comps and the reserve rows
    13: (0xAE23F0, 0xAE2398, 0x3C),      # first names
    14: (0xAE23F4, 0xAE239C, 0x3C),      # second names
    15: (0xAE23F8, 0xAE23A0, 0x3C),      # common names
    17: (0xAE23CC, 0xAE2374, 0x11),      # staff history
    18: (0xAE23E4, 0xAE238C, 0x3A),      # staff comp history
    19: (0xAE23D4, 0xAE237C, 0x1A),      # comp history with the nation comp history
    21: (0xAE23AC, 0xAE2354, 0x38),      # cities
    22: (0xAE23C8, 0xAE2370, 0x34),      # preferences
}
# The shared arrays: (table global of the part, table global of the whole array, size).
PARTS = [(0xAE23B8, 0xAE23B4, 0x245), (0xAE23D8, 0xAE23D0, 0x6B), (0xAE23DC, 0xAE23D4, 0x1A)]


def _u32(original, addr):
    return struct.unpack("<I", original.read(addr, 4))[0]


class Snapshot:
    """The counts and every table of the original at one step of the load."""

    def __init__(self, original):
        self.counts = [struct.unpack("<i", original.read(a, 4))[0] for a in COUNTS]
        self.parts = []
        for part, whole, size in PARTS:
            base = _u32(original, whole)
            self.parts.append((_u32(original, part) - base) // size if base else -1)
        self.bases = {}
        self.tables = {}
        for table, (pointer, count, size) in TABLES.items():
            base = _u32(original, pointer)
            rows = _u32(original, count) if base else 0
            self.bases[table] = (base, rows, size)
            self.tables[table] = original.read(base, rows * size) if rows else b""


def original_load(ctx):
    """Runs the load of the original once, from the state of its start, and keeps its tables.
    Returns the progress calls."""
    original = ctx.original
    if getattr(original, "database_progress", None) is not None:
        return original.database_progress
    progress = []
    snapshots = {}
    randoms = []

    def on_progress(this, text, step, total, channel):
        progress.append((original.read_c_string(text), step, total, channel))
        return 0

    def snapshot(name):
        return lambda uc, address, size, user_data: snapshots.setdefault(name, Snapshot(original))

    hooks = [original.uc.hook_add(UC_HOOK_CODE, snapshot(name), begin=addr, end=addr)
             for name, addr in (("read", AT_READ), ("resolved", AT_RESOLVED), ("staff", AT_STAFF_PASS))]
    hooks.append(original.uc.hook_add(UC_HOOK_CODE, lambda *a: randoms.append(1), begin=FUN_RANDOM,
                                      end=FUN_RANDOM))
    patched = [(addr, original.read(addr, size)) for addr, size in PATCHED_CODE]
    for addr, size in PATCHED_CODE:
        original.write(addr, original.pristine(addr, size))
    undo = original.replace_thiscall(FUN_PROGRESS, 4, on_progress)    # flushes the translated code
    try:
        settings = original.alloc(0x400)
        original.write_u32(G_SETTINGS, settings)
        original.write_u32(G_LANGUAGE, 0)
        for address in range(*G_KEY_IDS, 4):
            original.write_u32(address, -1)
        original.write_u32(G_EMPTY_COMMON_NAME, -1)
        folder = original.alloc_c_string(ctx.data_dir.rstrip("/") + "/")
        result = original.call(FUN_LOAD, folder, 1, 0)
    finally:
        undo()
        for addr, code in patched:
            original.write(addr, code)
        original.uc.ctl_flush_tb()
        for hook in hooks:
            original.uc.hook_del(hook)
    ctx.expect_equal("original load", 1, result, source="wanted")
    ctx.expect_equal("random numbers in the load", 0, len(randoms), source="wanted")
    snapshots["keys"] = Snapshot(original)
    original.heap_floor = original.heap_top
    original.database_progress = progress
    original.database_snapshots = snapshots
    return progress


def test_original_load(ctx):
    """The harness: the load of the original runs and gives the counts of the inventory."""
    progress = original_load(ctx)
    for addr, wanted in LOAD_COUNTS.items():
        ctx.expect_equal(f"count {addr:#x}", wanted, _u32(ctx.original, addr), source="wanted")
    wanted = [("Loading database", step, 46, 0xFFFFFFFF) for step in range(47)]
    ctx.expect_equal("progress calls", wanted, progress, source="wanted")
    return len(LOAD_COUNTS) + 1


# Comparison

def _column(data, offset, size, rows):
    """The ints at one offset of every record, as an array."""
    raw = bytearray(4 * rows)
    for byte in range(4):
        raw[byte::4] = data[offset + byte::size][:rows]
    return array.array("I", bytes(raw))


def _put_column(data, offset, size, rows, values):
    raw = array.array("I", values).tobytes()
    for byte in range(4):
        data[offset + byte::size] = raw[byte::4]


def _pointers_to_rows(snapshot, table, links):
    """The table of the original with the row behind each pointer, as the port writes it."""
    base, rows, size = snapshot.bases[table]
    data = bytearray(snapshot.tables[table])
    for offset, count, target in links:
        target_base, target_rows, target_size = snapshot.bases[target]
        end = target_base + target_rows * target_size
        for k in range(count):
            column = _column(data, offset + 4 * k, size, rows)
            converted = [0xFFFFFFFF if v == 0 else
                         (v - target_base) // target_size
                         if target_base <= v < end and (v - target_base) % target_size == 0 else v
                         for v in column]
            _put_column(data, offset + 4 * k, size, rows, converted)
    return bytes(data)


def _port_table(ctx, table):
    ctx.port.send("db_info", table)
    words = [int(w) for w in ctx.port.reply()]
    rows, size, count = words[:3]
    links = [tuple(words[3 + 3 * i:6 + 3 * i]) for i in range(count)]
    ctx.port.send("db_dump", table)
    length = int(ctx.port.reply()[0])
    return rows, size, links, ctx.port.read(length)


def _first_difference(a, b, size):
    at = next(i for i in range(min(len(a), len(b))) if a[i] != b[i])
    row, offset = divmod(at, size)
    return f"row {row} byte {offset:#x}: original {a[row * size:row * size + size].hex()}, " \
           f"port {b[row * size:row * size + size].hex()}"


def expect_tables(ctx, stage, linked):
    """Every table of the port against the copy of the original at one step of the load. `linked`:
    the pointer fields hold pointers."""
    snapshot = ctx.original.database_snapshots[stage]
    ctx.port.send("db_counts")
    words = [int(w) for w in ctx.port.reply()]
    ctx.expect_equal(f"{stage}: counts", snapshot.counts + snapshot.parts, words)
    cases = 1
    for table in TABLES:
        base, rows, size = snapshot.bases[table]
        port_rows, port_size, links, port_data = _port_table(ctx, table)
        ctx.expect_equal(f"{stage}: table {table} rows", (rows, size), (port_rows, port_size))
        wanted = _pointers_to_rows(snapshot, table, links) if linked else snapshot.tables[table]
        if wanted != port_data:
            ctx.expect_equal(f"{stage}: table {table}", "", _first_difference(wanted, port_data, size))
        cases += 1
    return cases


def test_read(ctx):
    """The loader up to the id pass: index.dat, the sizes, the reserve rows, the manager rows, the
    readers and the staff conversion with the preferences table."""
    original_load(ctx)
    ctx.port.send("db_fresh")
    ctx.port.send("db_read", ctx.data_dir)
    ctx.expect_equal("port read", [b"1"], ctx.port.reply(), source="wanted")
    return expect_tables(ctx, "read", linked=False)


def test_resolve(ctx):
    """The id pass FUN_005315a0: every id field becomes a pointer, or NULL, or keeps its id."""
    test_read(ctx)
    ctx.port.send("db_resolve")
    return expect_tables(ctx, "resolved", linked=True)


def test_staff_pass(ctx):
    """The staff pass and the national clubs of the loader."""
    test_resolve(ctx)
    ctx.port.send("db_staff_pass")
    return expect_tables(ctx, "staff", linked=True)


TESTS = [
    ("database_original_load", test_original_load),
    ("database_read", test_read),
    ("database_resolve", test_resolve),
    ("database_staff_pass", test_staff_pass),
]
