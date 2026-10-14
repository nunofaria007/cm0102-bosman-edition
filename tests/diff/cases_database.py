"""The database load: FUN_00528540 of the original in the emulator, and the tables of the port.

The original loads the database once for each run of the test, with code hooks that copy every table
at the steps of the load. Its tables stay below the heap floor, so that later tests can read them.
"""
import array
import os
import struct
import tempfile

from unicorn import UC_HOOK_CODE

import cases_leagues
from original import Original

FUN_LOAD = 0x528540                  # (data folder with a trailing separator, shared, from stream): 1 when it worked.
                                     # FUN_00524d50 calls it with (folder, 1, 0). The select suite replaces that one
FUN_PROGRESS = 0x802F80              # thiscall on the screen stack: (text, step, total, channel)
FUN_RANDOM = 0x90CFC0                # the load must not draw a random number with the shipped data
G_SETTINGS = 0xAE3138                # pointer to the settings object. +0x08 is the language
G_LANGUAGE = 0x9EB2DC
G_KEY_IDS = (0x9CF178, 0x9D0528)     # all -1 when the original starts
G_EMPTY_COMMON_NAME = 0x9B59B4       # -1 when the original starts
# The globals of the key ids: FUN_00601a40 sets them to -1, then from the names of the rows.
KEY_RANGES = [G_KEY_IDS, (0xB63C9C, 0xB63D04)]
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


class OriginalLoad:
    """One run of the load of the original from the state of its start, with a copy of every table at
    each step, the progress calls, the key ids and the error lines."""

    def __init__(self, original, folder):
        self.progress = []
        self.snapshots = {}
        randoms = []

        def on_progress(this, text, step, total, channel):
            self.progress.append((original.read_c_string(text), step, total, channel))
            return 0

        def snapshot(name):
            return lambda uc, address, size, user_data: self.snapshots.setdefault(name, Snapshot(original))

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
            path = original.alloc_c_string(folder.rstrip("/") + "/")
            errors_before = len(original.error_lines)
            self.result = original.call(FUN_LOAD, path, 1, 0)
            self.errors = original.error_lines[errors_before:]
        finally:
            undo()
            for addr, code in patched:
                original.write(addr, code)
            original.uc.ctl_flush_tb()
            for hook in hooks:
                original.uc.hook_del(hook)
        self.randoms = len(randoms)
        self.snapshots["keys"] = Snapshot(original)
        self.keys = {a: struct.unpack("<i", original.read(a, 4))[0]
                     for start, end in KEY_RANGES for a in range(start, end, 4)}

    def check(self, ctx):
        ctx.expect_equal("original load", 1, self.result, source="wanted")
        ctx.expect_equal("random numbers in the load", 0, self.randoms, source="wanted")
        ctx.expect_equal("error boxes in the load", [], self.errors, source="wanted")


def original_load(ctx):
    """Runs the load of the original on the shipped data once, and keeps its tables below the heap
    floor."""
    original = ctx.original
    if getattr(original, "database_load", None) is None:
        load = OriginalLoad(original, ctx.data_dir)
        load.check(ctx)
        original.heap_floor = original.heap_top
        original.database_load = load
    return original.database_load


def test_original_load(ctx):
    """The harness: the load of the original runs and gives the counts of the inventory."""
    progress = original_load(ctx).progress
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

    def bounds(target):
        target_base, target_rows, target_size = snapshot.bases[target]
        # The staff and player tables have 1,325 rows past their count (DB_SPARE_ROWS of the port).
        return target_base, target_base + (target_rows + (0x52D if target in (6, 10) else 0)) * target_size, \
            target_size

    def elsewhere(v):
        """A pointer into another table: 0x80000000 | table << 24 | row, as the port writes it. The port
        writes an id that the id pass left in a pointer field as 0x40000000 | id."""
        for other in TABLES:
            low, high, step = bounds(other)
            if low <= v < high and (v - low) % step == 0:
                return 0x80000000 | other << 24 | (v - low) // step
        return v

    for offset, count, target in links:
        low, high, step = bounds(target)
        for k in range(count):
            column = _column(data, offset + 4 * k, size, rows)
            converted = [0xFFFFFFFF if v == 0 else
                         (v - low) // step if low <= v < high and (v - low) % step == 0 else
                         elsewhere(v) if v >= 0x10000000 else 0x40000000 | v
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


def expect_tables(ctx, load, stage, linked):
    """Every table of the port against the copy of the original at one step of the load. `linked`:
    the pointer fields hold pointers."""
    snapshot = load.snapshots[stage]
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


def test_layouts(ctx):
    """The fields of each record of the port cover the record of the original once each, in order.
    The byte compare cannot see a field at a wrong offset, because the port reads and writes the
    record with the same layout."""
    ctx.port.send("db_layouts")
    ctx.expect_equal("tables with a wrong layout", [b"0"], ctx.port.reply(), source="wanted")
    return 1


def test_read(ctx):
    """The loader up to the id pass: index.dat, the sizes, the reserve rows, the manager rows, the
    readers and the staff conversion with the preferences table."""
    original_load(ctx)
    ctx.port.send("db_fresh")
    ctx.port.send("db_read", ctx.data_dir)
    ctx.expect_equal("port read", [b"1"], ctx.port.reply(), source="wanted")
    return expect_tables(ctx, original_load(ctx), "read", linked=False)


def test_resolve(ctx):
    """The id pass FUN_005315a0: every id field becomes a pointer, or NULL, or keeps its id."""
    test_read(ctx)
    ctx.port.send("db_resolve")
    return expect_tables(ctx, original_load(ctx), "resolved", linked=True)


def test_staff_pass(ctx):
    """The staff pass and the national clubs of the loader."""
    test_resolve(ctx)
    ctx.port.send("db_staff_pass")
    return expect_tables(ctx, original_load(ctx), "staff", linked=True)


def expect_keys(ctx, wanted):
    """The key ids of the port after FUN_00601a40 against those of the original."""
    ctx.port.send("db_keys")
    words = [int(w) for w in ctx.port.reply()]
    port = dict(zip(words[0::2], words[1::2]))
    ctx.expect_equal("key ids outside the ranges", [], sorted(set(port) - set(wanted)), source="wanted")
    # Two globals of the ranges (0x9d0300, 0x9d034c) are never used and stay -1. The port has none.
    wrong = [f"{a:#x}: original {wanted[a]}, port {port.get(a, 'none')}" for a in sorted(wanted)
             if wanted[a] != port.get(a, -1)]
    ctx.expect_equal("key ids", [], wrong[:5], source="wanted")
    named = [wanted[a] for a in cases_leagues.KEY_NATIONS + cases_leagues.KEY_CONTINENTS]
    ctx.port.send("db_named_keys")
    ctx.expect_equal("named key ids", named, [int(w) for w in ctx.port.reply()])
    return len(wanted) + 1


def test_keys(ctx):
    """FUN_00601a40: every key id after the load, and the copies of the named ones."""
    test_staff_pass(ctx)
    return expect_keys(ctx, original_load(ctx).keys)


def _s32_at(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


def _put(data, offset, value, fmt="<i"):
    struct.pack_into(fmt, data, offset, value)


def _synthetic_folder(ctx, folder):
    """The shipped data with changes that reach the branches of the load that the shipped data does
    not: ids at and past the ends of tables, and staff rows for each case of the staff pass."""
    files = {e.lower(): os.path.join(ctx.data_dir, e) for e in os.listdir(ctx.data_dir)}
    changed = {}

    def data(name):
        if name not in changed:
            with open(files[name], "rb") as source:
                changed[name] = bytearray(source.read())
        return changed[name]

    index = data("index.dat")
    _put(index, 4, 2)                                  # 14 manager rows
    entries = {_s32_at(index, 8 + i * 0x43 + 0x33): 8 + i * 0x43 for i in range(22)}
    staff_count = _s32_at(index, entries[6] + 0x37)
    nonplayer_offset = _s32_at(index, entries[9] + 0x3B)
    clubs, nat_clubs = data("club.dat"), data("nat_club.dat")
    club_rows = len(clubs) // 0x245
    staff_rows = staff_count + 23000 + 14              # the staff count after the load
    staff = data("staff.dat")

    def row(i):
        return i * 0x9D

    # Rows with player and non-player data, a nation and no jobs, from which the cases start.
    plain = [i for i in range(1000, staff_count)
             if _s32_at(staff, row(i) + 0x91) >= 0 and _s32_at(staff, row(i) + 0x99) >= 0 and
             _s32_at(staff, row(i) + 0x1A) >= 0][:12]
    for i in plain:
        _put(staff, row(i) + 0x39, -1)
        _put(staff, row(i) + 0x24, -1)
        staff[row(i) + 0x3D] = 0
        staff[row(i) + 0x28] = 0
    youth, b23, manager, both, player, names, coach, assistant, job15, prefs = plain[:10]
    # A club with a nation for the player case: a club without one is extinct.
    squad_club = next(c for c in range(100, club_rows) if _s32_at(clubs, c * 0x245 + 0x53) >= 0)
    _put(staff, row(youth) + 0x39, club_rows + len(nat_clubs) // 0x245 - 1)   # the last national club
    staff[row(youth) + 0x3D] = 11
    staff[row(youth) + 0x5E] = 2
    staff[row(b23) + 0x22], staff[row(b23) + 0x23] = 0, 1
    _put(staff, row(manager) + 0x24, club_rows + 3)
    staff[row(manager) + 0x28] = 5
    _put(staff, row(manager) + 0x99, -1)
    _put(nat_clubs, 3 * 0x245 + 0xCF, 5)
    staff[row(both) + 0x3D] = 12
    _put(staff, row(both) + 0x99, -1)
    _put(staff, row(player) + 0x39, squad_club)
    staff[row(player) + 0x3D] = 11
    _put(staff, row(player) + 0x91, -1)
    _put(clubs, squad_club * 0x245 + 0xD7, player)
    _put(clubs, squad_club * 0x245 + 0xDB, player)
    for offset in (0x04, 0x08, 0x0C):
        _put(staff, row(names) + offset, -1)
    _put(staff, row(coach) + 0x24, club_rows + 5)
    staff[row(coach) + 0x28] = 8
    _put(nat_clubs, 5 * 0x245 + 0x19F + 16, coach)          # the last of the five coaches
    _put(staff, row(assistant) + 0x24, club_rows + 6)
    staff[row(assistant) + 0x28] = 6
    _put(nat_clubs, 6 * 0x245 + 0xD3, assistant)
    staff[row(job15) + 0x28] = 15
    _put(staff, row(prefs) + 0x79, 5)
    # Ids at and past the ends of tables.
    _put(clubs, 200 * 0x245 + 0x5B, 9999)                      # last division past the club comps
    _put(clubs, 201 * 0x245 + 0xD7 + 8, staff_rows)            # squad id equal to the staff count
    _put(clubs, 202 * 0x245 + 0x9B, staff_rows)                # an id that the pass keeps
    _put(data("stadium.dat"), 10 * 0x4E + 0x48, 99999)         # near stadium past the stadiums
    _put(staff, nonplayer_offset + 0x23, staff_rows - 1)       # the last staff row
    for name, path in files.items():
        if name.endswith(".dat") and name not in changed:
            os.symlink(path, os.path.join(folder, name))
    for name, content in changed.items():
        with open(os.path.join(folder, name), "wb") as target:
            target.write(content)


def test_synthetic(ctx):
    """The load of both programs on changed data: every table and key id after the load."""
    with tempfile.TemporaryDirectory(dir=ctx.original.sandbox) as folder:
        _synthetic_folder(ctx, folder)
        original = Original(ctx.original.exe_path, ctx.data_dir)
        load = OriginalLoad(original, folder)
        load.check(ctx)
        ctx.port.send("db_fresh")
        ctx.port.send("db_read", folder)
        ctx.expect_equal("port read", [b"1"], ctx.port.reply(), source="wanted")
        ctx.port.send("db_resolve")
        ctx.port.send("db_staff_pass")
        cases = expect_tables(ctx, load, "staff", linked=True)
        return cases + expect_keys(ctx, load.keys)


TESTS = [
    ("database_original_load", test_original_load),
    ("database_layouts", test_layouts),
    ("database_read", test_read),
    ("database_resolve", test_resolve),
    ("database_staff_pass", test_staff_pass),
    ("database_keys", test_keys),
    ("database_synthetic", test_synthetic),
]
