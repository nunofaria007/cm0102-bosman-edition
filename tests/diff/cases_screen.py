"""Screen manager: the screen draw and the fade."""
import os
import struct
import tempfile

from cases_images import _pixels, _rgn, _write
from cases_scrman import _area_arguments, _widget_arguments
from scene import FUN_FADE, ManagerScene

OFFSET_BACKGROUND = 0x12F525
OFFSET_BACKGROUND_NAME = 0x12F55D


def _screen(ctx, scene):
    """Plain areas with widgets, popup areas, and widgets with no area, on screen most of the time."""
    areas = []
    for index in range(ctx.rng.randint(0, 4)):
        popup = ctx.rng.random() < 0.5
        x1, y1 = ctx.rng.randint(0, 600), ctx.rng.randint(0, 450)
        arguments = _area_arguments(ctx, owner=ctx.rng.randint(0, 20) if popup else -1)
        arguments[0] = [x1, y1, x1 + ctx.rng.randint(40, 199), y1 + ctx.rng.randint(30, 149)]
        arguments[1], arguments[3] = ctx.rng.randint(1, 4), ctx.rng.randint(1, 6)
        arguments[2] = arguments[4] = None
        arguments[5] &= 0x1 | 0x100 | 0x100000 | 0x200000 | 0x400000 | 0x800000
        arguments[5] |= ctx.rng.choice([0, 0x30, 0x30, 0x10])
        scene.area_create(f"screen area {index}", *arguments)
        areas.append(popup)
    for index in range(ctx.rng.randint(0, 25)):
        # The first widgets go into the areas, so that each popup area has some.
        area = index if index < len(areas) else ctx.rng.choice([-1, -1] + list(range(len(areas))) * 2)
        arguments = _widget_arguments(ctx, area, area != -1 and areas[area])
        x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 550)
        arguments[1] = [x1, y1, x1 + ctx.rng.randint(10, 99), y1 + ctx.rng.randint(10, 49)]
        # Flags and style from the widget draw test would need its own checks. Plain ones do here.
        arguments[0] &= 0x4 | 0x80 | 0x2000 | 0x4000 | 0x20000 | 0x40000
        arguments[4] = ctx.rng.choice([0, 0x10, 0x20, 0x30, 0x50])
        arguments[3] = ctx.rng.randint(0, 8)
        arguments[2] = ctx.rng.randint(0, 3)
        # The original has no font 0, and its text there is undefined.
        if arguments[8] == 0:
            arguments[8] = 1
        # The line buffer of the original's text function holds 251 characters (text spec).
        if arguments[10] is not None:
            # A popup area wider than the screen hides itself, so the texts in a popup area stay short.
            arguments[10] = arguments[10][:20 if area != -1 and areas[area] else 251]
        scene.widget_create(f"screen widget {index}", *arguments)
    for index, popup in enumerate(areas):
        if ctx.rng.random() < 0.2:
            scene.set("area", index, {"shown": 0})
    return areas


def _background(original, scene):
    pointer = struct.unpack("<I", original.read(scene.manager + OFFSET_BACKGROUND, 4))[0]
    size = struct.unpack("<2i", original.read(pointer, 8)) if pointer else None
    name = original.read_c_string(scene.manager + OFFSET_BACKGROUND_NAME)
    return size, name


def test_screen_draw(ctx):
    draws = 0
    with tempfile.TemporaryDirectory(dir="/tmp") as folder:
        full = [_write(folder, f"full{n}.rgn", _rgn(800, 600, _pixels(ctx, 800, 600))) for n in range(2)]
        small = _write(folder, "small.rgn", _rgn(300, 200, _pixels(ctx, 300, 200)))
        # Same width as the full pictures, so only the height tells the original not to reuse.
        short = _write(folder, "short.rgn", _rgn(800, 500, _pixels(ctx, 800, 500)))
        # The original reads RGB555 pictures too, and converts them to RGB565 as it loads them.
        old = _write(folder, "old.rgn", _rgn(800, 600, _pixels(ctx, 800, 600), g_mask=0x3E0))
        empty = tempfile.mkdtemp(dir=folder)
        names = [name.encode() for name in full + [small, short, old, os.path.join(folder, "missing.rgn")]]
        names += [b"", full[0].upper().encode(), b"\\"]
        for case in range(30):
            scene = ManagerScene(ctx)
            ctx.sync_framebuffer(case)
            scene.new(0)
            scene.reset(1)
            popups = _screen(ctx, scene)
            areas = len(popups)
            for step in range(ctx.rng.randint(1, 4)):
                name = f"screen {case} draw {step}"
                draws += 1
                scene.set_draw_globals(int(ctx.rng.random() < 0.25), int(ctx.rng.random() < 0.5))
                scene.set("manager", 0, {"darken_screen": int(ctx.rng.random() < 0.3),
                                         "skip_draw": int(ctx.rng.random() < 0.08),
                                         "fade_in": int(case in (3, 17) and step == 0)})
                if step and ctx.rng.random() < 0.3:
                    scene.set("manager", 0, {"laid_out": 0})
                # The layout closes the popup areas. Input opens one, so the test opens some.
                for index, popup in enumerate(popups):
                    if step and popup and ctx.rng.random() < 0.6:
                        scene.set("area", index, {"shown": 1})
                box = (-1, -1, -1, -1)
                if ctx.rng.random() < 0.5:
                    box = [ctx.rng.randint(0, 799), ctx.rng.randint(0, 599), ctx.rng.randint(0, 799),
                           ctx.rng.randint(0, 599)]
                    if ctx.rng.random() < 0.3:
                        box[ctx.rng.randrange(4)] = -1
                background = ctx.rng.choice(names)
                # Without DEFAULT_PIC.RGN, a name that does not load keeps the old background.
                scene.hide_data_files(ctx.rng.random() < 0.15, empty)
                size, stored = _background(ctx.original, scene)
                # With an empty name and a background of another size, the original loses the old
                # background. The port frees it.
                extra_free = int(background == b"" and stored != "" and size not in (None, (800, 600)))
                scene.start_free_count()
                scene.draw_screen(background, box)
                # A fade also frees the shade table of each colour in the original, so the count differs there.
                if not case in (3, 17) or step:
                    ctx.port.send("frees")
                    ctx.expect_equal(f"{name}: images freed", ctx.original.frees / 2 + extra_free,
                                     int(ctx.port.reply()[0]))
                ctx.expect_same_framebuffer(name)
                scene.expect_same_flips(name)
                scene.expect_same(name, "manager", 0)
                scene.expect_same_image(name, "manager", 0, "background")
            for index in range(areas):
                scene.expect_same(f"screen {case}", "area", index)
            scene.hide_data_files(False, empty)
    return draws


def test_fade(ctx):
    for case in range(30):
        ctx.sync_framebuffer(case)
        if ctx.rng.random() < 0.15:
            box = [ctx.x(), ctx.y(), ctx.x(), ctx.y()]
        else:
            x1, y1 = ctx.rng.randint(0, 780), ctx.rng.randint(0, 580)
            box = [x1, y1, x1 + ctx.rng.randint(0, 60), y1 + ctx.rng.randint(0, 40)]
        steps = ctx.rng.choice([0, 1, -1, 2, 20, -20, ctx.rng.randint(-25, 25)])
        scene = ManagerScene(ctx)
        ctx.original.flips = []
        ctx.original.call(FUN_FADE, *box, steps)
        ctx.port.send("fade", *box, steps)
        scene.expect_same_flips(f"fade {case}, box {box}, steps {steps}")
        ctx.expect_same_framebuffer(f"fade {case}")
    return case + 1


TESTS = [
    ("fade", test_fade),
    ("screen_draw", test_screen_draw),
]
