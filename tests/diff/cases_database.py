"""The database load: FUN_00528540 of the original in the emulator, and the tables of the port.

The original loads the database once for each run of the test. Its tables stay below the heap floor,
so that later tests can read them.
"""
import struct

FUN_LOAD = 0x528540                  # (data folder with a trailing separator, shared, from stream): 1 when it worked.
                                     # FUN_00524d50 calls it with (folder, 1, 0). The select suite replaces that one
FUN_PROGRESS = 0x802F80              # thiscall on the screen stack: (text, step, total, channel)
G_SETTINGS = 0xAE3138                # pointer to the settings object. +0x08 is the language
G_LANGUAGE = 0x9EB2DC

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


def _u32(original, addr):
    return struct.unpack("<I", original.read(addr, 4))[0]


def original_load(ctx):
    """Runs the load of the original once and keeps its tables. Returns the progress calls."""
    original = ctx.original
    if getattr(original, "database_progress", None) is not None:
        return original.database_progress
    progress = []

    def on_progress(this, text, step, total, channel):
        progress.append((original.read_c_string(text), step, total, channel))
        return 0

    undo = original.replace_thiscall(FUN_PROGRESS, 4, on_progress)
    try:
        settings = original.alloc(0x400)
        original.write_u32(G_SETTINGS, settings)
        original.write_u32(G_LANGUAGE, 0)
        folder = original.alloc_c_string(ctx.data_dir.rstrip("/") + "/")
        result = original.call(FUN_LOAD, folder, 1, 0)
    finally:
        undo()
    ctx.expect_equal("original load", 1, result, source="wanted")
    original.heap_floor = original.heap_top
    original.database_progress = progress
    return progress


def test_original_load(ctx):
    """The harness: the load of the original runs and gives the counts of the inventory."""
    progress = original_load(ctx)
    for addr, wanted in LOAD_COUNTS.items():
        ctx.expect_equal(f"count {addr:#x}", wanted, _u32(ctx.original, addr), source="wanted")
    wanted = [("Loading database", step, 46, 0xFFFFFFFF) for step in range(47)]
    ctx.expect_equal("progress calls", wanted, progress, source="wanted")
    return len(LOAD_COUNTS) + 1


TESTS = [
    ("database_original_load", test_original_load),
]
