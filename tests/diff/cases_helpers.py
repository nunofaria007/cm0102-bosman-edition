"""Helpers of the screens: the text function, the heading and button helpers, the menu builder, the Back and
Next bar and the text box."""
import struct

from cases_stack import STACK, Stack
from cases_widget import G_PICTURE_CACHE
from scene import ManagerScene

FUN_TEXT_FORMAT = 0x66F4E0       # cdecl (dst, format, ...)
FUN_HEADING = 0x803F30           # thiscall on the screen stack (colour, text colour, text)
FUN_SUB_HEADING = 0x803F80       # thiscall (text, unused)
FUN_CELL_BUTTON = 0x8040A0       # thiscall (area, column, row, style, colour, text flags, font, text colour, text, id, data)
FUN_CELL_LABEL = 0x804100        # thiscall (area, column, row, style, colour, text flags, font, text colour, text)
FUN_FREE_BUTTON = 0x804030       # thiscall (x1, y1, x2, y2, style, colour, text flags, font, text colour, text, id, data)
FUN_MENU = 0x5ED300              # cdecl, 11 arguments
FUN_BACK_NEXT = 0x5EDCC0         # cdecl (back, next)
FUN_TEXT_BOX = 0x5ED070          # cdecl (x1, y1, x2, y2, flags, colour, rows, font, text colour, text, 0)

G_LANGUAGE = 0x9EB2DC            # -1 selects the plain path of the text function
G_RECORD_MANAGER = 0xB74C4C      # the screen manager of record 0 of channel 0
OFFSET_WIDGET_ID, OFFSET_WIDGET_STYLE, OFFSET_WIDGET_AREA = 0x08, 0x38, 0x7A

WORDS = [b"Game", b"Exit", b"a", b"Yes", b"Manager", b"2001/02", b"Setup", b"x", b"Start New"]


def _hex(text):
    return "null" if text is None else "-" + text.hex()


def _s16(value):
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


class Start:
    """One screen manager of the test scene as the screen manager of the screen stack, in both programs."""

    def __init__(self, ctx):
        self.ctx = ctx
        original = ctx.original
        self.stack = Stack(ctx)
        self.scene = ManagerScene(ctx)
        self.scene.new(0)
        self.scene.reset(1)
        self.scene.set_draw_globals(0, 0)
        original.write_u32(G_RECORD_MANAGER, self.scene.manager)
        original.write_u32(G_LANGUAGE, -1)
        # The picture cache of the original points into the emulated heap, which the test empties. The port
        # keeps its cache, which holds the same picture.
        original.write(G_PICTURE_CACHE, bytes(4 + 260 + 4))
        ctx.port.send("st_init")

    def call_stack(self, function, *args):
        return self.ctx.original.call(function, *args, this=STACK)

    def text(self, data):
        return self.ctx.original.alloc_bytes(data + b"\0") if data is not None else 0

    def widget_count(self):
        return struct.unpack("<h", self.ctx.original.read(self.scene.manager + 0x12E9A0, 2))[0]

    def widget(self, index, offset, fmt):
        address = self.scene.addr("widget", index) + offset
        return struct.unpack(fmt, self.ctx.original.read(address, struct.calcsize(fmt)))[0]

    def expect_same_screen(self, case, menu_area=None):
        """Every area and widget in use and the screen manager. The menu buttons of the start screen
        have stack garbage of the original as their data (widget +0x48): the test leaves it out of
        the compare by setting it to 0 in both programs."""
        scene = self.scene
        if menu_area is not None:
            for index in range(self.widget_count()):
                if self.widget(index, OFFSET_WIDGET_AREA, "<h") == menu_area:
                    scene.set("widget", index, {"unknown_48": 0})
        scene.expect_same(case, "manager", 0)
        for index in range(scene.area_count()):
            scene.expect_same(case, "area", index)
        for index in range(self.widget_count()):
            scene.expect_same(case, "widget", index)

    def expect_same_draw(self, case, title):
        self.scene.draw_screen(title)
        self.scene.expect_same_flips(case)
        self.ctx.expect_same_framebuffer(case)


def word(ctx):
    return ctx.rng.choice(WORDS + [bytes(ctx.rng.choice(b"abcXYZ019.,:!?()-") for _ in range(ctx.rng.randint(1, 8)))])


def short_text(ctx):
    return b" ".join(word(ctx) for _ in range(ctx.rng.randint(1, 3)))


def _format(ctx):
    """A format with plain text, conversions and translator notes, and the kinds of its arguments."""
    parts, kinds = [b" " * ctx.rng.choice([0, 0, 0, 1, 3])], []
    for _ in range(ctx.rng.randint(0, 7)):
        roll = ctx.rng.random()
        if roll < 0.35:
            parts.append(word(ctx))
        elif roll < 0.5:
            parts.append(ctx.rng.choice([b" ", b"  ", b"\n", b"%%"]))
        elif roll < 0.75 and len(kinds) < 4:
            kind = ctx.rng.choice("ds")
            parts.append(ctx.rng.choice([b"%", b"%5", b"%-3"]) + kind.encode())
            kinds.append(kind)
        elif len(kinds) < 4:
            # A note: "<", text, a conversion, then the text up to ">", which goes.
            kind = ctx.rng.choice("ds")
            parts.append(b"<" + word(ctx) * ctx.rng.randint(0, 1) + b"%" + kind.encode() + b" " +
                         short_text(ctx) + b">")
            kinds.append(kind)
    return b"".join(parts), kinds


def test_text(ctx):
    original = ctx.original
    for case in range(300):
        text_format, kinds = _format(ctx)
        if case == 0:
            text_format, kinds = b"Version\n<%s - version>", ["s"]
        values = [ctx.rng.choice([0, -1, 7, ctx.rng.randint(-99999, 99999)]) if kind == "d" else short_text(ctx)
                  for kind in kinds]
        original.write_u32(G_LANGUAGE, -1)
        destination = original.alloc(4096)
        arguments = [value & 0xFFFFFFFF if kind == "d" else original.alloc_bytes(value + b"\0")
                     for kind, value in zip(kinds, values)]
        end = original.call(FUN_TEXT_FORMAT, destination, original.alloc_bytes(text_format + b"\0"), *arguments)
        wanted = [str(end - destination), "-" + original.read(destination, 4096).split(b"\0")[0].hex()]
        words = [f"{kind} {value}" if kind == "d" else f"s -{value.hex()}" for kind, value in zip(kinds, values)]
        ctx.port.send("st_text", "-" + text_format.hex(), len(kinds), *words)
        ctx.expect_equal(f"text {case}: {text_format!r}", wanted, [w.decode() for w in ctx.port.reply()])
    return case + 1


def _expect_index(ctx, case, result, port):
    ctx.expect_equal(f"{case}: index", _s16(result), int(port.reply()[0]))


def test_helpers(ctx):
    calls = 0
    for case in range(25):
        start = Start(ctx)
        ctx.sync_framebuffer(case)
        scene, port = start.scene, ctx.port
        areas = []
        for index in range(ctx.rng.randint(1, 3)):
            x1, y1 = ctx.rng.randint(0, 500), ctx.rng.randint(0, 400)
            box = [x1, y1, x1 + ctx.rng.randint(60, 290), y1 + ctx.rng.randint(40, 190)]
            columns, rows = ctx.rng.randint(1, 4), ctx.rng.randint(1, 8)
            areas.append((scene.area_create(f"helpers {case} area {index}", box, columns, None, rows, None,
                                            ctx.rng.choice([0, 1, 0x30]), ctx.colour(), -1), columns, rows))
        for step in range(ctx.rng.randint(3, 12)):
            name = f"helpers {case} step {step}"
            calls += 1
            text = short_text(ctx)
            style = ctx.rng.choice([0, 0x22, 0x30, 0x1021, 0x822])
            colour, text_colour = ctx.colour(), ctx.colour()
            flags, font = ctx.rng.choice([0, 0xc, 0x2c, 1]), ctx.rng.randint(1, 7)
            identity, data = ctx.rng.randint(-5, 0x500), ctx.rng.getrandbits(31)
            area, columns, rows = ctx.rng.choice(areas)
            column, row = ctx.rng.randrange(columns), ctx.rng.randrange(rows)
            kind = ctx.rng.randrange(5)
            if kind == 0:
                result = start.call_stack(FUN_HEADING, colour, text_colour, start.text(text))
                port.send("st_heading", colour, text_colour, _hex(text))
            elif kind == 1:
                result = start.call_stack(FUN_SUB_HEADING, start.text(text), ctx.rng.getrandbits(32))
                port.send("st_sub", _hex(text))
            elif kind == 2:
                result = start.call_stack(FUN_CELL_BUTTON, area, column, row, style, colour, flags, font,
                                          text_colour, start.text(text), identity, data)
                port.send("st_cell", 1, area, column, row, style, colour, flags, font, text_colour, identity,
                          data, _hex(text))
            elif kind == 3:
                result = start.call_stack(FUN_CELL_LABEL, area, column, row, style, colour, flags, font,
                                          text_colour, start.text(text))
                port.send("st_cell", 0, area, column, row, style, colour, flags, font, text_colour, 0, 0,
                          _hex(text))
            else:
                x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 550)
                box = [x1, y1, x1 + ctx.rng.randint(10, 99), y1 + ctx.rng.randint(10, 49)]
                result = start.call_stack(FUN_FREE_BUTTON, *box, style, colour, flags, font, text_colour,
                                          start.text(text), identity, data)
                port.send("st_free", *box, style, colour, flags, font, text_colour, identity, data, _hex(text))
            _expect_index(ctx, name, result, port)
        start.expect_same_screen(f"helpers {case}")
        start.expect_same_draw(f"helpers {case}", b"\\")
    return calls


def _items(ctx, count):
    items = []
    for _ in range(count):
        text = short_text(ctx) if ctx.rng.random() < 0.8 else ctx.rng.choice([b"", b"Game", b"a"])
        items.append((ctx.rng.choice([-1, -1, ctx.rng.randint(1, 7)]), ctx.rng.choice([0, 0, ctx.colour()]),
                      ctx.rng.randint(-3, 40), ctx.rng.getrandbits(31), text))
    return items


def test_menu(ctx):
    for case in range(40):
        start = Start(ctx)
        ctx.sync_framebuffer(case)
        original, port = ctx.original, ctx.port
        count = ctx.rng.choice([1, 2, 3, 5, 9, ctx.rng.randint(1, 20)])
        columns = ctx.rng.choice([1, 2, 2, 3, 4, ctx.rng.randint(1, 6)])
        rows = min(30, -(-count // columns) + ctx.rng.choice([0, 0, 1, 3]))
        x1, y1 = ctx.rng.randint(0, 300), ctx.rng.randint(0, 200)
        box = [x1, y1, x1 + ctx.rng.randint(150, 499), y1 + ctx.rng.randint(100, 399)]
        font = ctx.rng.randint(1, 7)
        items = _items(ctx, count)
        mask = ctx.rng.choice([0, 0xFFFFFFFF, ctx.rng.getrandbits(32), 1 << ctx.rng.randrange(count)])
        sort = ctx.rng.choice([0, 1, 1])
        table = b"".join(struct.pack("<hH", f, c) + t[:199].ljust(200, b"\0") + struct.pack("<iI", i, d)
                         for f, c, i, d, t in items)
        result = original.call(FUN_MENU, *box, columns, rows, font, count, original.alloc_bytes(table), mask, sort)
        words = [w for f, c, i, d, t in items for w in (f, c, i, d, _hex(t[:199]))]
        port.send("st_menu", *box, columns, rows, font, count, mask, sort, *words)
        ctx.expect_equal(f"menu {case}: result", result, int(port.reply()[0]))
        start.expect_same_screen(f"menu {case}")
        start.expect_same_draw(f"menu {case}", b"\\")
    return case + 1


def test_bars(ctx):
    """The Back and Next bar, and the text box of the confirm screen."""
    for case in range(30):
        start = Start(ctx)
        ctx.sync_framebuffer(case)
        original, port = ctx.original, ctx.port
        back, forward = ctx.rng.choice([0, 1, 5]), ctx.rng.choice([0, 1, -1])
        result = original.call(FUN_BACK_NEXT, back, forward)
        port.send("st_back_next", back, forward)
        ctx.expect_equal(f"bar {case}: result", result, int(port.reply()[0]))
        x1, y1 = ctx.rng.randint(0, 300), ctx.rng.randint(0, 200)
        box = [x1, y1, x1 + ctx.rng.randint(300, 499), y1 + ctx.rng.randint(60, 340)]
        flags, colour = ctx.rng.choice([2, 0, 0x30]), ctx.rng.choice([0, ctx.colour()])
        rows, font, text_colour = ctx.rng.randint(1, 30), ctx.rng.randint(1, 5), ctx.colour()
        # Only a text that fits on one line is ported.
        text = ctx.rng.choice([None, short_text(ctx), b"Exit the game ?", b"Restart the game ?"])
        result = original.call(FUN_TEXT_BOX, *box, flags, colour, rows, font, text_colour, start.text(text), 0)
        port.send("st_text_box", *box, flags, colour, rows, font, text_colour, _hex(text))
        ctx.expect_equal(f"text box {case}: result", result, int(port.reply()[0]))
        start.expect_same_screen(f"bars {case}")
        start.expect_same_draw(f"bars {case}", b"\\")
    return case + 1


TESTS = [
    ("start_text", test_text),
    ("start_helpers", test_helpers),
    ("start_menu", test_menu),
    ("start_bars", test_bars),
]
