"""Scrollbar input: the scrollbar click, the thumb drag, the keys and the wheel, the scroll and the scroll position."""
import struct

from cases_scrman import _widget_arguments
from scene import ManagerScene

FUN_POLL = 0x55F620              # thiscall on the screen manager, returns s16
FUN_TEXT_TIMERS = 0x569610       # reads the clock; not ported, so the test stubs it
FUN_CURSOR = 0x5E8170            # (kind): calls ShowCursor and SetCursor; the test stubs it
FUN_SCROLL_KEY = 0x5621F0        # stdcall (mode, frames)
FUN_SCROLL = 0x403EE0            # thiscall on an area (rows)
FUN_SHOWN_ROW = 0x4032E0         # thiscall on an area
FUN_SET_SCROLL_TO = 0x403320     # thiscall on an area (row)
FUN_STACK_SHOWN_ROW = 0x803320   # thiscall on the screen stack (area)
G_MOUSE_X, G_MOUSE_Y = 0xB67A20, 0xB67A14
G_LEFT_HELD, G_RIGHT_HELD = 0xB67A34, 0xB67A38
G_LEFT_CLICKED, G_RIGHT_CLICKED = 0xB67A0C, 0xB67A1C
G_CLICK_FRAMES = 0xB67A3C
G_CURSOR_KIND = 0x9C36E8
G_KEY_SCROLLBAR = 0xAD9C58
SCREEN_STACK = 0xB74450
STACK_CHANNEL, STACK_PLAYER, STACK_AREA, STACK_ROW, STACK_MANAGER = 0x382A, 0x7D4, 0x7F6, 0x7F8, 0x7FC
# The buffer that the scroll writes for network players (FUN_00800330). Without room it shows an error box.
G_SCROLL_NET_BUFFER, G_SCROLL_NET_CAPACITY = 0xAE28F8, 0xAE2900
OFFSET_SCROLLBAR = 0xB7A


def _signed16(value):
    return (value & 0xFFFF) - (0x10000 if value & 0x8000 else 0)


def _u32(ctx, address):
    return struct.unpack("<i", ctx.original.read(address, 4))[0]


def _screen(ctx, scene):
    """Areas with more rows than fit, so that each gets a scrollbar, and a few widgets with no area."""
    areas = []
    for index in range(ctx.rng.randint(1, 4)):
        x1, y1 = ctx.rng.randint(0, 560), ctx.rng.randint(0, 300)
        rows = ctx.rng.randint(1, 10)
        columns = ctx.rng.randint(1, 3)
        box = [x1, y1, x1 + ctx.rng.randint(80, 239), y1 + ctx.rng.randint(rows * 12 + 50, 299)]
        weights = [ctx.rng.randint(1, 4) for _ in range(rows)] if ctx.rng.random() < 0.3 else None
        scene.area_create(f"scroll area {index}", box, columns, None, rows, weights,
                          ctx.rng.choice([0, 0x30, 0x10, 0x1]), ctx.colour(), -1)
        # Some areas have rows that all fit, and no scrollbar.
        areas.append((columns, rows, rows + ctx.rng.choice([-1, 0, 1, 3, ctx.rng.randint(0, 40),
                                                            ctx.rng.randint(40, 90)])))
    widgets = 0
    for area, (columns, rows, last_row) in enumerate(areas):
        # Every row up to the last gets at least one widget, sometimes more, so the list has no gaps.
        for row in sorted(set(range(last_row + 1)) | {ctx.rng.randint(0, last_row) for _ in range(3)}):
            for _ in range(ctx.rng.choice([1, 1, 1, 2])):
                arguments = _widget_arguments(ctx, area, False)
                arguments[0] = ctx.rng.choice([0x2, 0x2, 0x0, 0x6])
                arguments[1] = [0, 0, 0, 0]
                arguments[2] = ctx.rng.randint(0, columns - 1)
                arguments[3] = row
                arguments[4] = ctx.rng.choice([0, 0x10, 0x30])
                arguments[8] = arguments[8] or 1
                arguments[10] = (arguments[10] or b"")[:20]
                scene.widget_create(f"scroll widget {widgets}", *arguments)
                widgets += 1
    for index in range(len(areas)):
        if ctx.rng.random() < 0.2:
            scene.set("area", index, {"shown": 0})
    for _ in range(ctx.rng.randint(0, 3)):
        arguments = _widget_arguments(ctx, -1, False)
        x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 550)
        arguments[0] = 0x2
        arguments[1] = [x1, y1, x1 + ctx.rng.randint(5, 99), y1 + ctx.rng.randint(5, 49)]
        # The ellipse style (FUN_005e7840) is not ported.
        arguments[4] = ctx.rng.choice([0, 0x10, 0x30])
        arguments[8] = arguments[8] or 1
        arguments[10] = (arguments[10] or b"")[:20]
        scene.widget_create(f"scroll widget {widgets}", *arguments)
        widgets += 1
    return len(areas), widgets


def _net_room(ctx):
    ctx.original.write_u32(G_SCROLL_NET_BUFFER, ctx.original.alloc(0x100))
    ctx.original.write_u32(G_SCROLL_NET_CAPACITY, 0x100)


def _scrollbar(ctx, scene, area):
    return struct.unpack("<11i", ctx.original.read(scene.addr("area", area) + OFFSET_SCROLLBAR, 44))


def _mouse(ctx, scene, areas):
    """A point on a part of a scrollbar, at an edge of it or anywhere."""
    x1, y1, x2, y2, track_y1, track_y2, _, _, _, thumb_y1, thumb_y2 = _scrollbar(ctx, scene, ctx.rng.randrange(areas))
    parts = [(y1, track_y1 - 1), (track_y2 + 1, y2), (track_y1, thumb_y1 - 1), (thumb_y2 + 1, track_y2),
             (thumb_y1, thumb_y2)]
    low, high = ctx.rng.choice(parts)
    if high < low:
        low, high = high, low
    x = ctx.rng.choice([x1, x2, x1 - 1, x2 + 1, ctx.rng.randint(x1, x2), ctx.rng.randint(x1, x2)])
    y = ctx.rng.choice([low, high, ctx.rng.randint(low, high), ctx.rng.randint(low, high)])
    if ctx.rng.random() < 0.1:
        x, y = ctx.rng.randint(0, 799), ctx.rng.randint(0, 599)
    return x, y


def _set_mouse(ctx, x, y, left):
    original = ctx.original
    for address, value in ((G_MOUSE_X, x), (G_MOUSE_Y, y), (G_LEFT_HELD, left), (G_RIGHT_HELD, 0),
                           (G_CURSOR_KIND, 0)):
        original.write_u32(address, value)
    ctx.port.send("mouse", x, y, left, 0, 0)


def _expect_all(ctx, scene, name, areas, widgets):
    scene.expect_same_flips(name)
    ctx.expect_same_framebuffer(name)
    scene.expect_same(name, "manager", 0)
    for index in range(areas):
        scene.expect_same(name, "area", index)
        for which in ("saved", "scrollbar"):
            scene.expect_same_image(name, "area", index, which)
    for index in range(widgets):
        scene.expect_same(name, "widget", index)


def _expect_key_scrollbar(ctx, scene, name):
    pointer = _u32(ctx, G_KEY_SCROLLBAR) & 0xFFFFFFFF
    wanted = -1
    if pointer:
        offset = pointer - scene.addr("area", 0) - OFFSET_SCROLLBAR
        wanted = offset // 0xBF1 if offset >= 0 and offset % 0xBF1 == 0 and offset // 0xBF1 < 250 else -2
    ctx.port.send("scroll_bar")
    ctx.expect_equal(f"{name}: area of the key scrollbar", wanted, int(ctx.port.reply()[0]))


def _stub_poll_helpers(original):
    """The poll calls the text timers and the cursor. Neither runs in the emulator."""
    if not getattr(original, "scroll_stubs", False):
        original._replace(FUN_TEXT_TIMERS, 0, lambda: 0)
        original._replace(FUN_CURSOR, 1, lambda kind: 0)
        original.scroll_stubs = True


def _poll(ctx, scene, name):
    """One input step: the poll runs the hit test in mode 0 and the thumb drag in mode 2."""
    original = ctx.original
    mode = _u32(ctx, scene.manager)
    original.flips = []
    result = _signed16(original.call(FUN_POLL, this=scene.manager))
    wanted = [result, _u32(ctx, scene.manager), _u32(ctx, G_CURSOR_KIND), _u32(ctx, G_LEFT_HELD)]
    ctx.port.send("scroll_poll")
    ctx.expect_equal(f"{name}: mode {mode}: result, mode, cursor kind, button", wanted,
                     [int(word) for word in ctx.port.reply()])


def _key(ctx, scene, name):
    """A key or the wheel: FUN_005621f0 puts the mouse on the key scrollbar and holds the button."""
    mode = ctx.rng.choice([0, 0, 1, 1, 2, 2, 3, 3, 4, 5, -1])
    frames = ctx.rng.choice([0, 1, 1, 1, 2, 3])
    ctx.original.call(FUN_SCROLL_KEY, mode & 0xFFFFFFFF, frames)
    ctx.port.send("scroll_key", mode, frames)
    wanted = [_u32(ctx, address) for address in (G_MOUSE_X, G_MOUSE_Y, G_LEFT_HELD, G_CLICK_FRAMES)]
    ctx.expect_equal(f"{name}: key mode {mode}, mouse, button, frames", wanted,
                     [int(word) for word in ctx.port.reply()])


def _count_frame(ctx, name):
    """The start of the pump (asm 0x68d520): the frames of a key click count down."""
    frames = _u32(ctx, G_CLICK_FRAMES)
    if frames:
        frames -= 1
        ctx.original.write_u32(G_CLICK_FRAMES, frames)
        if frames == 0:
            ctx.original.write_u32(G_LEFT_HELD, 0)
    ctx.port.send("scroll_count")
    ctx.expect_equal(f"{name}: button and frames after the pump", [_u32(ctx, G_LEFT_HELD), frames],
                     [int(word) for word in ctx.port.reply()])


def _new_screen(ctx, case):
    scene = ManagerScene(ctx)
    _net_room(ctx)
    _stub_poll_helpers(ctx.original)
    ctx.original.write_u32(G_LEFT_CLICKED, 0)
    ctx.original.write_u32(G_RIGHT_CLICKED, 0)
    ctx.sync_framebuffer(case)
    scene.new(0)
    scene.reset(1)
    scene.set_draw_globals(0, 0)
    areas, widgets = _screen(ctx, scene)
    ctx.original.write_u32(G_CLICK_FRAMES, 0)
    ctx.port.send("scroll_frames", 0)
    _set_mouse(ctx, 0, 0, 0)
    scene.draw_screen(b"\\")
    scene.expect_same_flips(f"scroll {case}: draw")
    ctx.expect_same_framebuffer(f"scroll {case}: draw")
    _expect_key_scrollbar(ctx, scene, f"scroll {case}: draw")
    return scene, areas, widgets


def test_scrollbar(ctx):
    """Clicks, drags, keys and wheel frames on areas with scrollbars, then the poll of each frame."""
    steps = 0
    for case in range(40):
        scene, areas, widgets = _new_screen(ctx, case)
        for step in range(ctx.rng.randint(5, 30)):
            name = f"scroll {case} step {step}"
            steps += 1
            mode = _u32(ctx, scene.manager)
            roll = ctx.rng.random()
            if roll < 0.25:
                if ctx.rng.random() < 0.15:
                    target = ctx.rng.choice([-1, ctx.rng.randrange(areas)])
                    address = 0 if target == -1 else scene.addr("area", target) + OFFSET_SCROLLBAR
                    ctx.original.write_u32(G_KEY_SCROLLBAR, address)
                    ctx.port.send("scroll_bar_set", target)
                _key(ctx, scene, name)
            elif mode == 2 and roll < 0.9:
                # Move the mouse while the thumb is held, often past the ends of the track.
                y = _u32(ctx, G_MOUSE_Y) + ctx.rng.choice([0, ctx.rng.randint(-30, 30), ctx.rng.randint(-400, 400)])
                _set_mouse(ctx, _u32(ctx, G_MOUSE_X), y, int(ctx.rng.random() < 0.85))
            elif roll < 0.85:
                x, y = _mouse(ctx, scene, areas)
                _set_mouse(ctx, x, y, int(ctx.rng.random() < 0.8))
            _poll(ctx, scene, name)
            _expect_all(ctx, scene, name, areas, widgets)
            _count_frame(ctx, name)
        scene.reset(1)
        _expect_key_scrollbar(ctx, scene, f"scroll {case}: after the reset")
    return steps


def test_scroll_area(ctx):
    """FUN_00403ee0 directly, with any number of rows, and the scroll position functions."""
    steps = 0
    for case in range(30):
        scene, areas, widgets = _new_screen(ctx, case)
        for step in range(ctx.rng.randint(3, 12)):
            name = f"scroll area {case} step {step}"
            steps += 1
            area = ctx.rng.randrange(areas)
            rows = ctx.rng.choice([0, 1, -1, ctx.rng.randint(-50, 50), ctx.rng.randint(-5, 5)])
            ctx.original.flips = []
            ctx.original.call(FUN_SCROLL, rows & 0xFFFFFFFF, this=scene.addr("area", area))
            ctx.port.send("scroll_area", area, rows)
            _expect_all(ctx, scene, name, areas, widgets)

            wanted = ctx.original.call(FUN_SHOWN_ROW, this=scene.addr("area", area))
            ctx.port.send("scroll_row", area)
            ctx.expect_equal(f"{name}: shown row", wanted, int(ctx.port.reply()[0]))

            player = ctx.rng.choice([0, 0, 1])
            other_area = ctx.rng.choice([area, -1, ctx.rng.randrange(areas)])
            other_row = ctx.rng.randint(-5, 50)
            for offset, size, value in ((STACK_CHANNEL, 2, 0), (STACK_PLAYER, 2, player),
                                        (STACK_AREA, 2, other_area), (STACK_ROW, 4, other_row),
                                        (STACK_MANAGER, 4, scene.manager)):
                ctx.original.write(SCREEN_STACK + offset, (value & 0xFFFFFFFF).to_bytes(4, "little")[:size])
            ctx.port.send("scroll_stack", player, other_area, other_row)
            asked = ctx.rng.choice([area, -1, ctx.rng.randrange(areas)])
            wanted = ctx.original.call(FUN_STACK_SHOWN_ROW, asked & 0xFFFF, this=SCREEN_STACK)
            ctx.port.send("scroll_srow", asked)
            ctx.expect_equal(f"{name}: shown row of the screen stack, player {player}", wanted,
                             int(ctx.port.reply()[0]))

            fields = ctx.original.read(scene.addr("area", area), 0xBAD)
            last = struct.unpack_from("<i", fields, 0x14)[0] - fields[0xBAC] + 1
            row = ctx.rng.choice([-1, 0, 1, last - 1, last, last + 1, ctx.rng.randint(-10, 60),
                                  ctx.rng.randint(-0x80000000, 0x7FFFFFFF)])
            ctx.original.call(FUN_SET_SCROLL_TO, row & 0xFFFFFFFF, this=scene.addr("area", area))
            ctx.port.send("scroll_to", area, row)
            scene.expect_same(f"{name}: scroll to {row}", "area", area)
    return steps


TESTS = [
    ("scrollbar", test_scrollbar),
    ("scroll_area", test_scroll_area),
]
