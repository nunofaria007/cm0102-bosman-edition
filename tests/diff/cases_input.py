"""Screen manager: the hit test of mode 0."""
import struct

from cases_scrman import _area_arguments, _widget_arguments
from scene import ManagerScene

FUN_HIT_TEST = 0x560490          # thiscall on the screen manager, returns s16
G_MOUSE_X, G_MOUSE_Y = 0xB67A20, 0xB67A14
G_LEFT_HELD, G_RIGHT_HELD = 0xB67A34, 0xB67A38
G_CURSOR_KIND = 0x9C36E8

# Button (more often), the right button presses it, drag, no sunken look. Text entry (0x08) is not ported.
FLAG_BITS = [0x2, 0x2, 0x2, 0x100, 0x40, 0x4]


def _screen(ctx, scene):
    """Plain areas whose rows all fit, so no area gets a scrollbar, and widgets with no area."""
    rows = []
    for index in range(ctx.rng.randint(0, 3)):
        x1, y1 = ctx.rng.randint(0, 600), ctx.rng.randint(0, 450)
        arguments = _area_arguments(ctx, owner=-1)
        arguments[0] = [x1, y1, x1 + ctx.rng.randint(40, 199), y1 + ctx.rng.randint(30, 149)]
        arguments[1], arguments[3] = ctx.rng.randint(1, 4), ctx.rng.randint(1, 6)
        arguments[2] = arguments[4] = None
        arguments[5] = ctx.rng.choice([0, 0x30, 0x10])
        scene.area_create(f"hit area {index}", *arguments)
        rows.append(arguments[3])
    widgets = []
    for index in range(ctx.rng.randint(len(rows) + 1, 20)):
        # An area with no widget makes the original read a widget before the array. Each area gets one.
        area = index if index < len(rows) else ctx.rng.choice([-1, -1] + list(range(len(rows))) * 2)
        arguments = _widget_arguments(ctx, area, False)
        x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 550)
        if widgets and ctx.rng.random() < 0.4:
            # Over the last widget, so that two buttons can be under the mouse at once.
            x1, y1 = last[0] + ctx.rng.randint(-5, 5), last[1] + ctx.rng.randint(-5, 5)
            x1, y1 = max(0, min(700, x1)), max(0, min(550, y1))
        arguments[1] = [x1, y1, x1 + ctx.rng.randint(5, 99), y1 + ctx.rng.randint(5, 49)]
        last = (x1, y1)
        arguments[0] = 0
        for bit in FLAG_BITS:
            if ctx.rng.random() < 0.35:
                arguments[0] |= bit
        arguments[4] = ctx.rng.choice([0, 0x10, 0x20, 0x30, 0x50])
        arguments[2] = ctx.rng.randint(0, 3)
        arguments[3] = ctx.rng.randint(0, rows[area] - 1) if area != -1 else 0
        arguments[8] = arguments[8] or 1
        if arguments[10] is not None:
            arguments[10] = arguments[10][:60]
        arguments[12] = ctx.rng.choice([0, 0, 1, ctx.rng.randint(-5, 50)])
        scene.widget_create(f"hit widget {index}", *arguments)
        widgets.append(index)
    for index in range(len(rows)):
        if ctx.rng.random() < 0.25:
            scene.set("area", index, {"shown": 0})
    return len(rows), len(widgets)


def _mouse(ctx, scene, widgets):
    """Mostly inside a widget, sometimes on its edge or anywhere."""
    original = ctx.original
    roll = ctx.rng.random()
    if roll < 0.7 and widgets:
        box = struct.unpack("<4i", original.read(scene.addr("widget", ctx.rng.randrange(widgets)) + 0x10, 16))
        x = ctx.rng.choice([box[0], box[2], ctx.rng.randint(min(box[0], box[2]), max(box[0], box[2]))])
        y = ctx.rng.choice([box[1], box[3], ctx.rng.randint(min(box[1], box[3]), max(box[1], box[3]))])
    else:
        x, y = ctx.rng.randint(0, 799), ctx.rng.randint(0, 599)
    return x, y


def test_hit(ctx):
    steps = 0
    for case in range(40):
        scene = ManagerScene(ctx)
        ctx.sync_framebuffer(case)
        scene.new(0)
        scene.reset(1)
        scene.set_draw_globals(0, 0)
        areas, widgets = _screen(ctx, scene)
        scene.draw_screen(b"\\")
        scene.expect_same_flips(f"hit {case}: draw")
        ctx.expect_same_framebuffer(f"hit {case}: draw")
        for step in range(ctx.rng.randint(1, 10)):
            name = f"hit {case} step {step}"
            steps += 1
            x, y = _mouse(ctx, scene, widgets)
            left, right = int(ctx.rng.random() < 0.7), int(ctx.rng.random() < 0.3)
            cursor = ctx.rng.choice([0, 2])
            scene.set_tick(ctx.rng.getrandbits(31))
            original = ctx.original
            for address, value in ((G_MOUSE_X, x), (G_MOUSE_Y, y), (G_LEFT_HELD, left),
                                   (G_RIGHT_HELD, right), (G_CURSOR_KIND, cursor)):
                original.write_u32(address, value)
            ctx.port.send("mouse", x, y, left, right, cursor)
            original.flips = []
            result = original.call(FUN_HIT_TEST, this=scene.manager)
            result = (result & 0xFFFF) - (0x10000 if result & 0x8000 else 0)
            wanted = [result, struct.unpack("<i", original.read(G_CURSOR_KIND, 4))[0],
                      struct.unpack("<i", original.read(G_LEFT_HELD, 4))[0]]
            ctx.port.send("hit")
            ctx.expect_equal(f"{name}: widget, cursor kind, left button", wanted,
                             [int(word) for word in ctx.port.reply()])
            scene.expect_same_flips(name)
            ctx.expect_same_framebuffer(name)
            scene.expect_same(name, "manager", 0)
            for index in range(widgets):
                scene.expect_same(name, "widget", index)
            for index in range(areas):
                scene.expect_same(name, "area", index)
    return steps


TESTS = [
    ("hit", test_hit),
]
