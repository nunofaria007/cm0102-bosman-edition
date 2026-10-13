"""Cases for widget draw."""
import os
import struct
import tempfile

from cases_images import _pixels, _rgn, _write
from cases_panel import ELLIPSE, divides_by_zero
from scene import Scene

FUN_WIDGET_DRAW = 0x5EE590

G_PICTURE_CACHE = 0xAE291C  # use count, then the file name (260 bytes), then the image

PICTURE = 0x400
OTHER_FLAGS = [0x4, 0x80, 0x800, 0x2000, 0x4000, 0x20000, 0x40000]
POPUP_FLAGS = [0x100, 0x100000, 0x200000, 0x400000, 0x800000]
OFFSET_PICTURE = 0x5C


# The original has no font 0, so no case uses it.
TEXT_FONTS = [-1, 1, 2, 3, 4, 5, 6, 7]
TEXTS = [b"", b"Arsenal", b"1\x01hidden", b"\x01", b"a\x01b\x01c", b"Manchester United v Liverpool",
         b"Yes\nNo", b"Caf#00e9# au lait", b"Goals|For", b"the quick brown fox jumps over the lazy dog"]


def text_flags(ctx):
    """Any flags, with the ones the text function knows more often set than not."""
    flags = ctx.rng.getrandbits(32) & ctx.rng.getrandbits(32)
    for flag in (0x1, 0x2, 0x20, 0x40, 0x80, 0x100):
        flags = flags | flag if ctx.rng.random() < 0.3 else flags & ~flag
    return flags


def _text(value):
    return list(value) + [0]


def _some(ctx, flags, chance):
    return sum(flag for flag in flags if ctx.rng.random() < chance)


def _style(ctx, box):
    while True:
        style = ctx.rng.getrandbits(26)
        for _ in range(ctx.rng.randint(1, 3)):
            style &= ctx.rng.getrandbits(26)
        style &= ~ELLIPSE
        if not divides_by_zero(*box, style):
            return style


def _random_widget(ctx):
    x1, y1 = ctx.x(), ctx.y()
    box = (x1, y1, x1 + ctx.rng.randint(8, 300), y1 + ctx.rng.randint(6, 60))
    return {
        "x1": box[0], "y1": box[1], "x2": box[2], "y2": box[3],
        "flags": _some(ctx, OTHER_FLAGS, 0.25), "style": _style(ctx, box),
        "colour": ctx.colour(), "highlight_colour": ctx.colour(),
        "font": ctx.rng.choice(TEXT_FONTS), "text_colour": ctx.colour(),
        "text_flags": text_flags(ctx), "cursor": ctx.rng.choice([-1, -1, ctx.rng.randint(0, 12)]),
        "text": _text(ctx.rng.choice(TEXTS)),
        "lighten_once": ctx.rng.choice([0, 0, 1]), "highlighted": ctx.rng.choice([0, 0, 1]),
        "area": ctx.rng.choice([0, -1]), "popup_area": -1,
    }


def _draw(ctx, scene, index, redraw, case):
    ctx.original.call(FUN_WIDGET_DRAW, redraw, this=scene.addr("widget", index))
    ctx.port.send("draw_widget", index, redraw)
    ctx.expect_same_framebuffer(case)
    scene.expect_same(case, "widget", index)
    scene.expect_same_image(case, "widget", index, "saved")


def test_widget_draw(ctx):
    """Widget 0, drawn twice. Half the time it opens area 2 as a popup area."""
    ctx.sync_framebuffer(11)
    for case in range(1200):
        scene = Scene(ctx)
        widget = _random_widget(ctx)
        scene.set("area", 0, {"x1": 10, "y1": 10, "x2": 500, "y2": 400, "owner_widget": -1})
        if ctx.rng.random() < 0.5:
            widget["popup_area"] = 2
            # A popup area with its sides the wrong way round makes more than one arrow rule true.
            x1, y1 = ctx.rng.randint(0, 700), ctx.rng.randint(0, 550)
            width = ctx.rng.choice([ctx.rng.randint(5, 300), ctx.rng.randint(5, 300), ctx.rng.randint(-200, -1)])
            height = ctx.rng.choice([ctx.rng.randint(5, 300), ctx.rng.randint(5, 300), ctx.rng.randint(-200, -1)])
            scene.set("area", 2, {"x1": x1, "y1": y1, "x2": x1 + width, "y2": y1 + height, "owner_widget": 0,
                                  "flags": _some(ctx, POPUP_FLAGS, 0.3)})
        scene.set("widget", 0, widget)
        _draw(ctx, scene, 0, ctx.rng.randint(0, 1), f"widget case {case}")
        scene.set("widget", 0, {"highlighted": ctx.rng.randint(0, 1), "lighten_once": ctx.rng.choice([0, 1])})
        _draw(ctx, scene, 0, ctx.rng.randint(0, 1), f"widget case {case}, second draw")
    return (case + 1) * 2


def test_widget_popup_arrows(ctx):
    """The four sides a popup area can open on. The side follows from where the widget is and the popup flags."""
    ctx.sync_framebuffer(12)
    below = 0x100
    tall, flat = 100, 0
    sides = {
        "right": ((300, 280, 380, 300), 0, tall),
        "left": ((450, 280, 530, 300), 0, tall),
        "below": ((300, 280, 380, 300), below, tall),
        "above": ((300, 560, 380, 580), below, tall),
        # A popup area of no height sits exactly on the limits of the above and below tests.
        "right, level with the top": ((300, 280, 380, 300), 0, flat),
        "right, level with the bottom": ((300, 280, 380, 300), 0x400000, flat),
    }
    count = 0
    for side, (box, flags, height) in sides.items():
        for text in (b"", b"Tactics"):
            for colour in (0x8410, 0x0000):
                for style in (0x30, 0x70):
                    scene = Scene(ctx)
                    scene.set("widget", 0, {"x1": box[0], "y1": box[1], "x2": box[2], "y2": box[3], "style": style,
                                            "colour": colour, "text": _text(text), "area": -1, "popup_area": 2})
                    scene.set("area", 2, {"x1": 0, "y1": 0, "x2": 120, "y2": height, "owner_widget": 0, "flags": flags})
                    _draw(ctx, scene, 0, 1, f"popup arrow {side}, text {text!r}, colour {colour:#06x}, style {style:#x}")
                    count += 1
    return count


def test_widget_picture(ctx):
    """Widgets that name the first file share its image. Each widget that names the second file loads its own,
    because the one shared image stays with the first file while widgets use it.

    The test clears the picture cache of the original. The port has no way to clear its own, so this test
    can run once for each start of the port driver.
    """
    ctx.sync_framebuffer(13)
    original = ctx.original
    scene = Scene(ctx)
    original.write(G_PICTURE_CACHE, bytes(4 + 260 + 4))
    # A picture widget still gets its tick, marks, highlight and popup arrow.
    extra_flags = [0x2000 | 0x4000, 0x20000 | 0x40000, 0x2000, 0x4000, 0, 0x20000]
    highlighted = [1, 0, 0, 1, 0, 1]
    popup_areas = [-1, 2, -1, -1, 3, -1]
    scene.set("area", 2, {"x1": 0, "y1": 0, "x2": 90, "y2": 70, "owner_widget": 1})
    scene.set("area", 3, {"x1": 0, "y1": 0, "x2": 90, "y2": 70, "owner_widget": 4, "flags": 0x100})
    with tempfile.TemporaryDirectory() as folder:
        first = _write(folder, "first.rgn", _rgn(40, 30, _pixels(ctx, 40, 30)))
        second = _write(folder, "second.rgn", _rgn(25, 25, _pixels(ctx, 25, 25)))
        names = [first, first, second, os.path.join(folder, "missing.rgn"), second, first]
        for index, name in enumerate(names):
            x, y = 50 + index % 4 * 150, 100 + index // 4 * 100
            scene.set("widget", index, {"x1": x, "y1": y, "x2": x + 100, "y2": y + 60,
                                        "flags": PICTURE | extra_flags[index], "style": 0x20, "colour": 0x8410,
                                        "highlight_colour": 0xF81F, "text_colour": 0x4321,
                                        "highlighted": highlighted[index], "text": _text(name.encode()),
                                        "popup_area": popup_areas[index], "area": -1})
        for step, index in enumerate([0, 1, 2, 3, 4, 5, 0, 2]):
            case = f"picture step {step}, widget {index}"
            _draw(ctx, scene, index, 1, case)
            scene.expect_same_image(case, "widget", index, "picture")
            pointers = [struct.unpack("<I", original.read(scene.addr("widget", i) + OFFSET_PICTURE, 4))[0]
                        for i in range(len(names))]
            for other in range(len(names)):
                ctx.port.send("same_picture", index, other)
                ctx.expect_equal(f"{case}: shares its image with widget {other}",
                                 int(pointers[index] == pointers[other]), int(ctx.port.reply()[0]))
    return step + 1


TESTS = [
    ("widget_draw", test_widget_draw),
    ("widget_popup_arrows", test_widget_popup_arrows),
    ("widget_picture", test_widget_picture),
]
