"""Text entry: FUN_005617c0 on its own and through the hit test, with clicks, keys and characters."""
import struct

from cases_helpers import G_RECORD_MANAGER
from cases_input import FUN_HIT_TEST, G_CURSOR_KIND, G_LEFT_HELD, G_MOUSE_X, G_MOUSE_Y, G_RIGHT_HELD
from cases_scrman import _area_arguments, _widget_arguments
from scene import ManagerScene

FUN_TEXT_ENTRY = 0x5617C0        # thiscall on the screen manager (widget index), returns a bool in AL
G_KEY_CODE, G_KEY_CHAR = 0xB67A04, 0xB67A24
G_SHOWN_DISPLAY = 0xB77C7A       # s16, the record whose screen manager shows; the test uses 0
OFFSET_TEXT_SOURCE = 0x54
SOURCE_SIZE = 512

TEXT_ENTRY = 0x8
# Cursor at the end, keeps the focus, takes : and \, takes ?, and a button.
ENTRY_BITS = [0x10, 0x20, 0x8000, 0x10000, 0x2]
# None, Backspace, Tab, Enter, Left, Right, Delete, A, Up, Space.
KEYS = [0, 0, 0, 0x08, 0x08, 0x09, 0x0D, 0x25, 0x25, 0x27, 0x27, 0x2E, 0x2E, 0x41, 0x41, 0x26, 0x20]
CHARS = (list(b"aZ09 .-'xyzMEv") * 3 + list(b'"*/<>|{}[]~:\\?') + [0, 0x1F, 0x7F, 0xE9, 0xFF])
WORDS = [b"Max", b"Evans", b"", b"Arsenal FC", b"a", b"pass word", b"Caf\xe9"]
LIMITS = [0, 3, 8, 12, 20, 60, -1, -5, -20, 255]
FONTS = [1, 2, 3, 4, 5, 6, 7]


def _text(ctx):
    return b" ".join(ctx.rng.choice(WORDS) for _ in range(ctx.rng.choice([1, 1, 2, 3])))[:40]


def _arguments(ctx, area, entry):
    arguments = _widget_arguments(ctx, area, False)
    arguments[0] = TEXT_ENTRY if entry else ctx.rng.choice([0x2, 0x2, 0x6, 0x0])
    for bit in ENTRY_BITS if entry else []:
        if ctx.rng.random() < 0.3:
            arguments[0] |= bit
    arguments[4] = ctx.rng.choice([0, 0x10, 0x20, 0x30])
    arguments[8] = ctx.rng.choice(FONTS)
    arguments[10] = _text(ctx)
    arguments[11] = ctx.rng.choice(LIMITS)
    arguments[12] = ctx.rng.choice([0, 0, 1])
    return arguments


def _screen(ctx, scene):
    """Free widgets and a few plain areas whose rows all fit. About half of the widgets are text entries."""
    rows = []
    for index in range(ctx.rng.randint(0, 2)):
        x1, y1 = ctx.rng.randint(0, 500), ctx.rng.randint(0, 400)
        arguments = _area_arguments(ctx, owner=-1)
        arguments[0] = [x1, y1, x1 + ctx.rng.randint(80, 299), y1 + ctx.rng.randint(40, 199)]
        arguments[1], arguments[3] = ctx.rng.randint(1, 3), ctx.rng.randint(1, 4)
        arguments[2] = arguments[4] = None
        arguments[5] = ctx.rng.choice([0, 0x30, 0x10])
        scene.area_create(f"entry area {index}", *arguments)
        rows.append(arguments[3])
    entries = []
    count = ctx.rng.randint(len(rows) + 1, 10)
    for index in range(count):
        area = index if index < len(rows) else ctx.rng.choice([-1, -1, -1] + list(range(len(rows))))
        entry = ctx.rng.random() < 0.55
        arguments = _arguments(ctx, area, entry)
        x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 560)
        arguments[1] = [x1, y1, x1 + ctx.rng.randint(20, 299), y1 + ctx.rng.randint(10, 39)]
        arguments[2] = ctx.rng.randint(0, 2)
        arguments[3] = ctx.rng.randint(0, rows[area] - 1) if area != -1 else 0
        result = scene.widget_create(f"entry widget {index}", *arguments)
        if entry:
            entries.append(result)
            _set_source(ctx, scene, result, arguments[10])
    for index in range(len(rows)):
        if ctx.rng.random() < 0.2:
            scene.set("area", index, {"shown": 0})
    return len(rows), count, entries


def _set_source(ctx, scene, index, text):
    """A caller's buffer of 512 bytes for the widget, in both programs."""
    original = ctx.original
    pointer = original.alloc_bytes(text + bytes(SOURCE_SIZE - len(text)))
    original.write_u32(scene.addr("widget", index) + OFFSET_TEXT_SOURCE, pointer)
    ctx.port.send("entry_buffer", index, "-" + text.hex())


def _expect_sources(ctx, scene, name, entries):
    original = ctx.original
    for index in entries:
        pointer = struct.unpack("<I", original.read(scene.addr("widget", index) + OFFSET_TEXT_SOURCE, 4))[0]
        text = original.read(pointer, SOURCE_SIZE).split(b"\0")[0]
        ctx.port.send("entry_text", index)
        ctx.expect_equal(f"{name}: caller's text of widget {index}", "-" + text.hex(),
                         ctx.port.reply()[0].decode())


def _mouse(ctx, scene, entries):
    """Mostly inside a text entry, sometimes on its edge or anywhere."""
    if entries and ctx.rng.random() < 0.75:
        box = struct.unpack("<4i", ctx.original.read(scene.addr("widget", ctx.rng.choice(entries)) + 0x10, 16))
        x = ctx.rng.choice([box[0], box[2], box[0] - 1, ctx.rng.randint(min(box[0], box[2]), max(box[0], box[2]))])
        y = ctx.rng.choice([box[1], box[3], box[3] + 1, ctx.rng.randint(min(box[1], box[3]), max(box[1], box[3]))])
        return x, y
    return ctx.rng.randint(0, 799), ctx.rng.randint(0, 599)


def _expect_all(ctx, scene, name, areas, widgets, entries):
    scene.expect_same_flips(name)
    ctx.expect_same_framebuffer(name)
    scene.expect_same(name, "manager", 0)
    for index in range(widgets):
        scene.expect_same(name, "widget", index)
        scene.expect_same_image(name, "widget", index, "saved")
    for index in range(areas):
        scene.expect_same(name, "area", index)
    _expect_sources(ctx, scene, name, entries)


def test_entry(ctx):
    steps = 0
    original = ctx.original
    for case in range(60):
        scene = ManagerScene(ctx)
        ctx.sync_framebuffer(case)
        scene.new(0)
        scene.reset(1)
        scene.set_draw_globals(0, 0)
        original.write_u32(G_RECORD_MANAGER, scene.manager)
        original.write(G_SHOWN_DISPLAY, b"\0\0")
        ctx.port.send("entry_init")
        areas, widgets, entries = _screen(ctx, scene)
        scene.draw_screen(b"\\")
        _expect_all(ctx, scene, f"entry {case}: draw", areas, widgets, entries)
        if ctx.rng.random() < 0.15:
            # The screen has not been drawn: the text changes, but nothing draws.
            scene.set("manager", 0, {"laid_out": 0})
        x, y, left = 0, 0, 0
        for step in range(ctx.rng.randint(5, 40)):
            name = f"entry {case} step {step}"
            steps += 1
            if ctx.rng.random() < 0.35:
                x, y = _mouse(ctx, scene, entries)
                left = int(ctx.rng.random() < 0.4)
            code, char = ctx.rng.choice(KEYS), ctx.rng.choice(CHARS)
            if ctx.rng.random() < 0.1 and entries:
                scene.set("manager", 0, {"passed_focus": ctx.rng.choice(entries + [-1, -1])})
            scene.set_tick(ctx.rng.getrandbits(31))
            for address, value in ((G_MOUSE_X, x), (G_MOUSE_Y, y), (G_LEFT_HELD, left), (G_RIGHT_HELD, 0),
                                   (G_CURSOR_KIND, 0), (G_KEY_CODE, code)):
                original.write_u32(address, value)
            original.write(G_KEY_CHAR, bytes([char]))
            ctx.port.send("mouse", x, y, left, 0, 0)
            ctx.port.send("entry_keys", code, char)
            original.flips = []
            scene.start_free_count()
            state = lambda: [struct.unpack("<i", original.read(G_KEY_CODE, 4))[0], original.read(G_KEY_CHAR, 1)[0],
                             struct.unpack("<i", original.read(G_CURSOR_KIND, 4))[0],
                             struct.unpack("<i", original.read(G_LEFT_HELD, 4))[0]]
            if entries and ctx.rng.random() < 0.5:
                index = ctx.rng.choice(entries)
                action = f"text entry {index}"
                result = original.call(FUN_TEXT_ENTRY, index, this=scene.manager) & 0xFF
                ctx.port.send("entry", index)
                ctx.expect_equal(f"{name}: text entry {index} (key {code:#x}, char {char:#x}): done, key, char, "
                                 f"cursor kind, button", [result] + state(),
                                 [int(word) for word in ctx.port.reply()])
            else:
                action = "hit test"
                result = original.call(FUN_HIT_TEST, this=scene.manager)
                result = (result & 0xFFFF) - (0x10000 if result & 0x8000 else 0)
                ctx.port.send("hit")
                hit = [int(word) for word in ctx.port.reply()]
                ctx.port.send("entry_state")
                ctx.expect_equal(f"{name}: hit test (key {code:#x}, char {char:#x}): widget, key, char, "
                                 f"cursor kind, button", [result] + state(),
                                 hit[:1] + [int(word) for word in ctx.port.reply()])
            _expect_all(ctx, scene, f"{name} ({action}, key {code:#x}, char {char:#x})", areas, widgets, entries)
            scene.expect_same_frees(f"{name} ({action})")
    return steps


TESTS = [
    ("entry", test_entry),
]
