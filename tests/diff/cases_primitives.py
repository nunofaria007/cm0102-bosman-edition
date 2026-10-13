"""Cases for the basic drawing primitives (gui_utils)."""
import struct

from original import HEIGHT, WIDTH

FUN_CLIP = 0x5E3680
FUN_LINE = 0x5E3730
FUN_RECT = 0x5E3B50
FUN_DRAW_GLYPH = 0x5E3CE0
FUN_CAPTURE = 0x5E3DD0
FUN_DARKEN = 0x5E42E0
FUN_SCALE_COLOUR = 0x5E4890

# Glyph tables of the original: scroll arrows, popup arrows and widget marks.
GLYPH_TABLES = [0x9CD67C, 0x9CD69C, 0x9CD6BC, 0x9CD6DC, 0x9CD6FC, 0x9CD730, 0x9CD774, 0x9CD7BC]


def test_clip(ctx):
    out = ctx.original.alloc(16)
    for case in range(2000):
        box = (ctx.x(), ctx.y(), ctx.x(), ctx.y())
        ok = ctx.original.call(FUN_CLIP, *box, out)
        expected = [ok, *struct.unpack("<4i", ctx.original.read(out, 16))]
        ctx.port.send("clip", *box)
        ctx.expect_equal(f"clip{box}", expected, [int(v) for v in ctx.port.reply()])
    box = (10, 10, 20, 20)
    ctx.port.send("clip_null", *box)
    ctx.expect_equal(f"clip{box} with no output box", ctx.original.call(FUN_CLIP, *box, 0),
                     int(ctx.port.reply()[0]))
    return case + 2


def test_scale_colour(ctx):
    for case in range(4000):
        colour, percent = ctx.colour(), ctx.rng.randint(0, 255)
        expected = ctx.original.call(FUN_SCALE_COLOUR, colour, percent, 0) & 0xFFFF
        ctx.port.send("scale", colour, percent)
        ctx.expect_equal(f"scale({colour:#06x}, {percent})", expected, int(ctx.port.reply()[0]))
    return case + 1


def _style(ctx, named):
    """A named style, or any value, because the original tests single bits."""
    return ctx.rng.choice(named + [ctx.rng.randint(0, 0x7FFFFFFF)])


def test_line(ctx):
    ctx.sync_framebuffer(1)
    for case in range(1000):
        x1, y1, x2, y2 = ctx.x(), ctx.y(), ctx.x(), ctx.y()
        shape = ctx.rng.random()
        if shape < 0.2:
            y2 = y1
        elif shape < 0.4:
            x2 = x1
        elif shape < 0.45:
            x2, y2 = x1, y1
        elif shape < 0.6:
            length = ctx.rng.randint(1, 300)
            x2 = x1 + ctx.rng.choice([-length, length])
            y2 = y1 + ctx.rng.choice([-length, length])
        elif shape < 0.75:
            x2 = x1 + ctx.rng.randint(-3, 3)
            y2 = y1 + ctx.rng.randint(-3, 3)
        args = (x1, y1, x2, y2, _style(ctx, [0, 1, 2, 3]), ctx.colour())
        ctx.original.call(FUN_LINE, *args)
        ctx.port.send("line", *args)
        ctx.expect_same_framebuffer(f"line{args}")
    return case + 1


def test_rect(ctx):
    ctx.sync_framebuffer(2)
    for case in range(300):
        args = (ctx.x(), ctx.y(), ctx.x(), ctx.y(), _style(ctx, [0, 1, 2, 3, 4, 5, 6]), ctx.colour())
        ctx.original.call(FUN_RECT, *args)
        ctx.port.send("rect", *args)
        ctx.expect_same_framebuffer(f"rect{args}")
    return case + 1


def test_darken(ctx):
    ctx.sync_framebuffer(3)
    boxes = [(0, 0, WIDTH - 1, HEIGHT - 1), (0, 0, WIDTH, HEIGHT), (10, 10, 10, 10), (300, 200, 100, 50)]
    boxes += [(ctx.x(), ctx.y(), ctx.x(), ctx.y()) for _ in range(40)]
    for box in boxes:
        ctx.original.call(FUN_DARKEN, *box)
        ctx.port.send("darken", *box)
        ctx.expect_same_framebuffer(f"darken{box}")
    return len(boxes)


def test_draw_glyph(ctx):
    ctx.sync_framebuffer(4)
    tables = []
    for addr in GLYPH_TABLES:
        width, height = ctx.original.read(addr, 2)
        tables.append(ctx.original.read(addr, 2 + width * height))
    for _ in range(400):
        width, height = ctx.rng.randint(0, 24), ctx.rng.randint(0, 24)
        body = bytes(ctx.rng.choice([0, ctx.rng.randint(0, 255)]) for _ in range(width * height))
        tables.append(bytes([width, height]) + body)
    scratch = ctx.original.alloc(2 + 24 * 24)
    for table in tables:
        x = ctx.origin(table[0], WIDTH)
        y = ctx.origin(table[1], HEIGHT)
        colour = ctx.colour()
        ctx.original.write(scratch, table)
        ctx.original.call(FUN_DRAW_GLYPH, x, y, colour, scratch)
        ctx.port.send("glyph", x, y, colour, table.hex())
        ctx.expect_same_framebuffer(f"glyph {table[0]}x{table[1]} at ({x}, {y})")
    ctx.original.call(FUN_DRAW_GLYPH, 100, 100, 0xFFFF, 0)
    ctx.port.send("glyph", 100, 100, 0xFFFF, "null")
    ctx.expect_same_framebuffer("no glyph table")
    return len(tables) + 1


def _original_image(ctx, addr):
    if not addr:
        return None
    header = struct.unpack("<12I", ctx.original.read(addr, 48))
    width, height, size, pixels = header[:4]
    return [width, height, size, *header[4:]], ctx.original.read(pixels, size)


def _port_image(ctx, words):
    if words == [b"0"]:
        return None
    fields = [int(v) for v in words]
    return fields, ctx.port.read(fields[2])


def test_capture(ctx):
    ctx.sync_framebuffer(5)
    for case in range(60):
        ctx.original.reset_heap()
        box = (ctx.x(), ctx.y(), ctx.x(), ctx.y())
        ctx.port.send("capture", *box)
        ctx.expect_equal(f"capture{box}", _original_image(ctx, ctx.original.call(FUN_CAPTURE, *box, 0)),
                         _port_image(ctx, ctx.port.reply()))
    return case + 1


def test_capture_reuse(ctx):
    """A second capture keeps the first image only when the size is the same."""
    ctx.sync_framebuffer(6)
    for case in range(120):
        ctx.original.reset_heap()
        x, y = ctx.rng.randint(0, 600), ctx.rng.randint(0, 400)
        w, h = ctx.rng.randint(1, 100), ctx.rng.randint(1, 100)
        first = (x, y, x + w - 1, y + h - 1)
        second = ctx.rng.choice([
            (x + 50, y + 50, x + w + 49, y + h + 49),
            (x, y, x + w - 1, y + h),
            (x, y, x + w, y + h - 1),
            (ctx.x(), ctx.y(), ctx.x(), ctx.y()),
        ])
        image1 = ctx.original.call(FUN_CAPTURE, *first, 0)
        image2 = ctx.original.call(FUN_CAPTURE, *second, image1)
        expected = [int(image2 == image1), _original_image(ctx, image2)]
        ctx.port.send("capture2", *first, *second)
        words = ctx.port.reply()
        ctx.expect_equal(f"capture{first} then capture{second}", expected,
                         [int(words[0]), _port_image(ctx, words[1:])])
    return case + 1


TESTS = [
    ("clip", test_clip),
    ("scale_colour", test_scale_colour),
    ("line", test_line),
    ("rect", test_rect),
    ("darken", test_darken),
    ("draw_glyph", test_draw_glyph),
    ("capture", test_capture),
    ("capture_reuse", test_capture_reuse),
]
