""""Start New Game" (FUN_0081ce00) and "Select League(s)" for a new game: the build 0x81d010 and the input
0x81dc30."""
import struct

import cases_start
from cases_helpers import OFFSET_WIDGET_ID, _s16
from cases_progress import BUFFER_SIZE, OFFSET_BUFFER, OFFSET_SESSION
from cases_leagues import (FUN_LEAGUE_TABLE, G_FIRST, G_NATIONS, G_SELECTED, G_TO_LOAD, NATION_SIZE, Tables)
from cases_scroll import FUN_SCROLL, G_SCROLL_NET_BUFFER, G_SCROLL_NET_CAPACITY
from cases_stack import FUN_SET_SLOT, FUN_SLOT
from cases_start import Screens, _background
from original import FB_SIZE
from scene import KINDS

FUN_START_NEW_GAME = 0x81CE00
FUN_SELECT_BUILD, FUN_SELECT_INPUT = 0x81D010, 0x81DC30
FUN_FREE_MEMORY = 0x7A1710       # free memory in MB: the test answers a large value
FUN_LOAD = 0x524D50              # (folder, 1): the database load, which the test models
FUN_SETTINGS = 0x5D8480          # thiscall: the settings apply, not ported
FUN_PUMP = 0x68D520              # the message pump: the test counts its calls
FUN_OPCODE_7 = 0x802F80          # thiscall on the screen stack (text, step, total, s16 channel), ret 0x10
STACK = 0xB74450
G_REAL_PLAYERS, G_MASKING = 0x9B5981, 0x9CC270
G_ADDING_MANAGER = 0xDD7118      # 0: no game exists yet
G_B74348 = 0xB74348              # 0 on a plain start
FREE_MB = 8190                   # the most that GlobalMemoryStatus can give

# The model of the load: a page with the code and its data.
MODEL = 0x63000000
MODEL_DATA = MODEL + 0x800       # text pointer, nation table
PROGRESS_STEPS, PROGRESS_TOTAL = 47, 46

SELECT = 3
cases_start.BUILDS[FUN_SELECT_BUILD] = SELECT
cases_start.INPUTS[FUN_SELECT_INPUT] = SELECT

# Ids of the screen.
LIST_IDS = (0xC, 0xD, 0xE, 0x38)
OPTION_IDS = (0xA, 0xB, 0x39, 0x3A, 0x3B, 0x3C)
ID_NEXT = -3
SCREEN_REBUILD = -4


def _model_code():
    """The steps of the loader as calls of opcode 7: "Loading database", steps 0 to 46 of 46 on channel -1.
    Then the nation table is in place, and the load returns 1."""
    call = MODEL + 19
    code = (b"\x53" + b"\x31\xdb" +                         # push ebx; xor ebx, ebx
            b"\x6a\xff" + b"\x6a" + bytes([PROGRESS_TOTAL]) + b"\x53" +   # push -1; push 46; push ebx
            b"\xff\x35" + struct.pack("<I", MODEL_DATA) +   # push [text]
            b"\xb9" + struct.pack("<I", STACK) +            # mov ecx, stack
            b"\xe8" + struct.pack("<i", FUN_OPCODE_7 - (call + 5)) +
            b"\x43" + b"\x83\xfb" + bytes([PROGRESS_STEPS]) + b"\x7c\xe5" +   # inc ebx; cmp ebx, 47; jl
            b"\xa1" + struct.pack("<I", MODEL_DATA + 4) +   # mov eax, [nations]
            b"\xa3" + struct.pack("<I", G_NATIONS) +        # mov [0xae23a8], eax
            b"\xb8\x01\x00\x00\x00" + b"\x5b\xc3")          # mov eax, 1; pop ebx; ret
    assert code[28:30] == b"\x7c\xe5"
    return code


def _patch(original):
    if getattr(original, "select_ready", False):
        return
    original.select_ready = True
    original.uc.mem_map(MODEL, 0x1000)
    original.write(MODEL, _model_code())
    original.write(FUN_LOAD, b"\xe9" + struct.pack("<i", MODEL - (FUN_LOAD + 5)))
    original._replace(FUN_FREE_MEMORY, 0, lambda: FREE_MB)
    original._replace(FUN_SETTINGS, 0, lambda: 0)
    if not getattr(original, "progress_ready", False):
        original.progress_ready = True
        original.pumps = 0

        def pump():
            original.pumps += 1
            return 0
        original._replace(FUN_PUMP, 0, pump)


class Select(Screens):
    """The screen stack with the start screen and "Select League(s)", and the tables, in both programs."""

    def __init__(self, ctx, load=False):
        super().__init__(ctx)
        original = ctx.original
        _patch(original)
        original.write_u32(G_ADDING_MANAGER, 0)
        original.write(G_B74348, b"\0")
        self.set_options(ctx.rng.choice([0, 1, 1, 2]), ctx.rng.choice([0, 1, 1, 7]))
        self.tables = Tables(ctx)
        self.keys = self.tables.match_keys("keys")
        original.write_u32(MODEL_DATA, original.alloc_c_string("Loading database"))
        original.write_u32(MODEL_DATA + 4, self.tables.nations)
        if load:
            # No database in memory: FUN_0081ce00 loads it.
            original.write_u32(G_NATIONS, 0)
            ctx.port.send("lg_free")
            ctx.port.send("sl_clear_keys")
        else:
            original.call(FUN_LEAGUE_TABLE)
            ctx.port.send("lg_table")
            ctx.port.reply()
        # The packet buffer of opcode 7, and no network session.
        original.write(STACK + OFFSET_BUFFER, struct.pack("<3I", original.alloc(BUFFER_SIZE), 0, BUFFER_SIZE))
        original.write_u32(STACK + OFFSET_SESSION, 0)
        original.write_u32(G_SCROLL_NET_BUFFER, original.alloc(0x100))
        original.write_u32(G_SCROLL_NET_CAPACITY, 0x100)
        original.pumps = 0
        original.flips = []
        ctx.port.send("sl_record")
        self.drop_port_flips()

    def drop_port_flips(self):
        self.ctx.port.send("flips")
        for _ in range(int(self.ctx.port.reply()[0])):
            if int(self.ctx.port.reply()[4]):
                self.ctx.port.read(FB_SIZE)

    def set_options(self, real, masking):
        self.ctx.original.write(G_REAL_PLAYERS, bytes([real]))
        self.ctx.original.write(G_MASKING, bytes([masking]))
        self.ctx.port.send("sl_options", real, masking)

    def scramble(self):
        """Random league flags of the league nations, counters and first selected nation."""
        ctx, tables = self.ctx, self.tables
        rows = [row for row in self.keys[:27] if row >= 0]
        for _ in range(ctx.rng.choice([0, 1, 2, 4, 8, 20])):
            row = ctx.rng.choice(rows)
            value = ctx.rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 0xFF])
            ctx.original.write(tables.nations + row * NATION_SIZE + 0x11C, bytes([value]))
            ctx.port.send("lg_set_flag", row, value)
        counters = [ctx.rng.choice([0, 1, 2, 14, 15, 16, ctx.rng.randint(-2, 40)]),
                    ctx.rng.choice([0, 1, 2, 14, 15, 16, ctx.rng.randint(-2, 40)]),
                    ctx.rng.choice([-1, -1] + rows)]
        selected = [row for row in rows
                    if ctx.original.read(tables.nations + row * NATION_SIZE + 0x11C, 1)[0] & 2]
        if selected and ctx.rng.random() < 0.6:
            counters[2] = selected[0] if ctx.rng.random() < 0.5 else ctx.rng.choice(selected)
        if ctx.rng.random() < 0.3:
            counters[2] = -1
        ctx.original.write_u32(G_TO_LOAD, counters[0])
        ctx.original.write_u32(G_SELECTED, counters[1])
        ctx.original.write_u32(G_FIRST, 0 if counters[2] < 0 else tables.nations + counters[2] * NATION_SIZE)
        ctx.port.send("lg_counters", *counters)

    def expect_same_state(self, case):
        original = self.ctx.original
        wanted = self.tables.flags() + [str(original.read(G_REAL_PLAYERS, 1)[0]).encode(),
                                        str(original.read(G_MASKING, 1)[0]).encode()]
        self.ctx.port.send("sl_state")
        self.ctx.expect_equal(f"{case}: league flags, counters and options", wanted, self.ctx.port.reply())

    def expect_same_pumps(self, case):
        self.ctx.port.send("sl_pumps")
        self.ctx.expect_equal(f"{case}: pumps", self.ctx.original.pumps, int(self.ctx.port.reply()[0]))
        self.ctx.original.pumps = 0

    def _data(self, value):
        tables = self.tables
        offset = value - tables.nations
        if 0 <= offset < tables.nation_count * NATION_SIZE and offset % NATION_SIZE == 0:
            return f"n{offset // NATION_SIZE}"
        return str(value)

    def expect_same_select_screen(self, case):
        """Every area and widget in use and the screen manager. The data of a widget is a nation pointer:
        the test compares the row of the nation."""
        scene, ctx = self.scene, self.ctx
        scene.expect_same(case, "manager", 0)
        for index in range(scene.area_count()):
            scene.expect_same(case, "area", index)
        fields, size = KINDS["widget"]
        for index in range(self.widget_count()):
            raw = ctx.original.read(scene.addr("widget", index), size)
            ctx.port.send("get", "manager_widget", index)
            port_values = [int(v) for v in ctx.port.reply()]
            position = 0
            for name, offset, fmt, count in fields:
                if name != "unknown_48":
                    ctx.expect_equal(f"{case}: widget {index} {name}",
                                     list(struct.unpack_from(f"<{count}{fmt}", raw, offset)),
                                     port_values[position:position + count])
                position += count
            ctx.port.send("sl_data", index)
            ctx.expect_equal(f"{case}: widget {index} data", self._data(struct.unpack_from("<I", raw, 0x48)[0]),
                             ctx.port.reply()[0].decode())

    def expect_same_flips(self, case):
        """The same boxes flipped in the same order, each with the same screen at the time."""
        port = self.ctx.port
        port.send("flips")
        flips = []
        for _ in range(int(port.reply()[0])):
            words = [int(word) for word in port.reply()]
            flips.append((words[:4], port.read(FB_SIZE) if words[4] else None))
        wanted = self.ctx.original.flips
        self.ctx.expect_equal(f"{case}: flip boxes", [box for box, _ in wanted], [box for box, _ in flips])
        for number, ((_, a), (_, b)) in enumerate(zip(wanted, flips)):
            if a != b:
                self.ctx.expect_equal(f"{case}: screen at flip {number}", "same", "different")
        self.ctx.original.flips = []

    def new_game(self, case):
        """FUN_0081ce00 in both programs, outside the start screen."""
        self.ctx.original.call(FUN_START_NEW_GAME)
        self.ctx.port.send("sl_new_game")
        self.ctx.expect_equal(f"{case}: start new game", b"1", self.ctx.port.reply()[0], source="wanted")

    def set_scroll_slot(self, value):
        self.call_stack(FUN_SET_SLOT, 0, value & 0xFFFFFFFF, 0)
        self.ctx.port.send("st_slot0", value)

    def ids(self):
        return [_s16(self.widget(i, OFFSET_WIDGET_ID, "<i")) for i in range(self.widget_count())]

    def finish(self):
        self.tables.set_language(-1)


def test_start_new_game(ctx):
    """The click on "Start New Game": the load with its progress bar, or only the push when the database
    is in memory."""
    for case in range(16):
        name = f"start new game {case}"
        load = case % 3 != 2
        select = Select(ctx, load=load)
        ctx.sync_framebuffer(case)
        if ctx.rng.random() < 0.3:
            select.push_start()
        select.push_start()
        select.build(_background(ctx))
        if not load:
            select.scramble()
        ctx.original.flips = []
        select.drop_port_flips()
        select.expect_same_pumps(f"{name}: before")
        item = select.ids().index(1)
        select.input(name, item)
        select.expect_same_flips(name)
        select.expect_same_pumps(name)
        ctx.expect_same_framebuffer(name)
        select.expect_same_stack(name)
        select.expect_same_state(name)
        select.finish()
    return case + 1


def _build(ctx, case, prefix=True):
    select = Select(ctx)
    ctx.sync_framebuffer(case)
    if prefix:
        select.push_start()
    select.scramble()
    select.new_game(f"build {case}")
    if ctx.rng.random() < 0.25:
        # A screen after it, so that ">>>" (id -3, as "Next") is a button.
        select.push_start()
        select.back()
    if ctx.rng.random() < 0.6:
        select.set_scroll_slot(ctx.rng.choice([0, 1, 5, 11, 12, 26, 40, -3, ctx.rng.randint(0, 30)]))
    select.build(_background(ctx))
    return select


def test_build(ctx):
    for case in range(30):
        name = f"select build {case}"
        select = _build(ctx, case, prefix=ctx.rng.random() < 0.8)
        select.expect_same_stack(name)
        select.expect_same_select_screen(name)
        select.expect_same_state(name)
        select.expect_same_draw(name, _background(ctx))
        select.finish()
    return case + 1


def _click(ctx, select):
    ids = select.ids()
    roll = ctx.rng.random()
    if roll < 0.05:
        return -1
    if roll < 0.2:
        choices = [i for i, value in enumerate(ids) if value in (0xA, 0xB, ID_NEXT)]
        return ctx.rng.choice(choices) if choices else -1
    if roll < 0.45:
        # The buttons of the first selected nation, which a change moves on.
        first = struct.unpack("<I", ctx.original.read(G_FIRST, 4))[0]
        choices = [i for i, value in enumerate(ids) if value in LIST_IDS and first and
                   select.widget(i, 0x48, "<I") == first]
        if choices:
            return ctx.rng.choice(choices)
    if roll < 0.55:
        # Only six nations have an extra division.
        choices = [i for i, value in enumerate(ids) if value == 0x38]
        if choices:
            return ctx.rng.choice(choices)
    wanted = LIST_IDS if roll < 0.7 else OPTION_IDS if roll < 0.88 else None
    choices = [i for i, value in enumerate(ids) if wanted is None or value in wanted]
    return ctx.rng.choice(choices) if choices else -1


def test_input(ctx):
    steps = 0
    for case in range(60):
        select = _build(ctx, case)
        name = f"select input {case}"
        title = _background(ctx)
        select.expect_same_draw(name, title)
        # Each draw takes about 1.3 MB of the 16 MB emulated heap for its saved screens, which the test
        # never frees.
        for step in range(ctx.rng.randint(1, 8)):
            name = f"select input {case} step {step}"
            if ctx.rng.random() < 0.3:
                area = select.call_stack(FUN_SLOT, 1)
                rows = ctx.rng.choice([1, -1, 3, 16, -16, ctx.rng.randint(-30, 30)])
                ctx.original.call(FUN_SCROLL, rows & 0xFFFFFFFF, this=select.scene.addr("area", area))
                ctx.port.send("scroll_area", area, rows)
                select.expect_same_flips(f"{name}: scroll {rows}")
            clicked = _click(ctx, select)
            steps += 1
            if ctx.rng.random() < 0.04:
                # No nation table: the screen goes back.
                ctx.original.write_u32(G_NATIONS, 0)
                ctx.port.send("lg_free")
                select.input(f"{name}: no table, click {clicked}", clicked)
                break
            selected = struct.unpack("<i", ctx.original.read(G_SELECTED, 4))[0]
            if clicked >= 0 and select.ids()[clicked] == ID_NEXT and selected != 0:
                # The original goes on to "Select Start Season" or creates the game. The port rebuilds.
                ctx.port.send("st_input", clicked)
                ctx.expect_equal(f"{name}: Next", SCREEN_REBUILD, int(ctx.port.reply()[0]), source="wanted")
                break
            result = select.input(f"{name}: click {clicked}", clicked)
            select.expect_same_state(name)
            select.expect_same_stack(name)
            if select.current_build() != FUN_SELECT_BUILD or result not in (-11, -4):
                break
            title = _background(ctx)
            select.build(title)
            select.expect_same_select_screen(name)
            select.expect_same_draw(name, title)
        select.finish()
    return steps


TESTS = [
    ("select_start_new_game", test_start_new_game),
    ("select_build", test_build),
    ("select_input", test_input),
]
