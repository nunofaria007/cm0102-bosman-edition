"""Cases for a line of text and for text in a box."""
import re

from original import HEIGHT, WIDTH

FUN_LINE_WIDTH = 0x5E5AD0   # (font, text)
FUN_DRAW_LINE = 0x5E5160    # (x, y, font, colour, text, cursor)

FONTS = [1, 2, 3, 4, 5, 6, 7]
CODE = re.compile(rb"#([0-9a-fA-F]{4})#")


def random_text(ctx, max_length):
    """Printable and accented characters, bars, control characters and character codes.

    A character code above 0xff reads past the glyphs of the font in the original, so no text has one.
    """
    while True:
        parts = []
        for _ in range(ctx.rng.randint(0, max_length)):
            roll = ctx.rng.random()
            if roll < 0.62:
                parts.append(bytes([ctx.rng.randint(0x21, 0x7E)]))
            elif roll < 0.74:
                parts.append(b" ")
            elif roll < 0.84:
                parts.append(bytes([ctx.rng.randint(0x80, 0xFF)]))
            elif roll < 0.88:
                parts.append(b"|")
            elif roll < 0.91:
                parts.append(bytes([ctx.rng.randint(1, 0x1F)]))
            elif roll < 0.96:
                parts.append(ctx.rng.choice([b"#%04x#", b"#%04X#"]) % ctx.rng.choice([0, 0x41, ctx.rng.randint(1, 0xFF)]))
            else:
                parts.append(ctx.rng.choice([b"#", b"#12#", b"#zz00#", b"##", b"#12345#"]))
        text = b"".join(parts)
        if all(int(code, 16) <= 0xFF for code in CODE.findall(text)):
            return text


def as_word(text):
    """A text as one word for the port driver."""
    return "-" + text.hex()


def test_line_width(ctx):
    for case in range(3000):
        font = ctx.rng.choice(FONTS + [-1])
        text = random_text(ctx, 40)
        expected = ctx.original.call(FUN_LINE_WIDTH, font, ctx.original.alloc_bytes(text + b"\0"))
        ctx.port.send("line_width", font, as_word(text))
        ctx.expect_equal(f"width of {text!r} in font {font}", expected, int(ctx.port.reply()[0]))
        if case % 200 == 0:
            ctx.original.reset_heap()
    return case + 1


def _draw_line(ctx, x, y, font, colour, text, cursor):
    ctx.original.call(FUN_DRAW_LINE, x, y, font, colour, ctx.original.alloc_bytes(text + b"\0"), cursor)
    ctx.port.send("draw_line", x, y, font, colour, cursor, as_word(text))


def test_draw_line(ctx):
    ctx.sync_framebuffer(20)
    for case in range(1500):
        ctx.original.reset_heap()
        font = ctx.rng.choice(FONTS + FONTS + [-1])
        text = random_text(ctx, 24)
        x = ctx.rng.choice([ctx.rng.randint(0, 600), ctx.rng.randint(0, 600), ctx.x()])
        y = ctx.rng.choice([ctx.rng.randint(0, 540), ctx.rng.randint(0, 540), ctx.y()])
        cursor = ctx.rng.choice([-1, -1, ctx.rng.randint(0, len(text) + 2)])
        _draw_line(ctx, x, y, font, ctx.colour(), text, cursor)
        ctx.expect_same_framebuffer(f"line {text!r} at ({x}, {y}) font {font} cursor {cursor}")
    return case + 1


def test_draw_line_blend_reuse(ctx):
    """The original keeps the colour of the screen pixel under its last blended pixel. It reads the
    screen again only when the next screen pixel differs from the last pixel it wrote.

    The first draw of a character shows which pixels it writes. The test then gives every second one
    of them the value that the draw wrote to the one before, and draws again.
    """
    count = 0
    seed = 21
    for font in (3, 5, 7):
        for char in b"W@8S%g":
            text, x, y, colour = bytes([char]), 300, 200, 0xFFFF
            ctx.sync_framebuffer(seed)
            before = ctx.original.framebuffer()
            _draw_line(ctx, x, y, font, colour, text, -1)
            ctx.expect_same_framebuffer(f"first draw of {text!r} in font {font}")
            after = ctx.original.framebuffer()
            written = [i for i in range(0, len(before), 2) if before[i:i + 2] != after[i:i + 2]]

            ctx.sync_framebuffer(seed)
            for earlier, later in zip(written[0::2], written[1::2]):
                value = int.from_bytes(after[earlier:earlier + 2], "little")
                pixel = later // 2
                ctx.original.put_pixel(pixel % WIDTH, pixel // WIDTH, value)
                ctx.port.send("put", pixel % WIDTH, pixel // WIDTH, value)
            _draw_line(ctx, x, y, font, colour, text, -1)
            ctx.expect_same_framebuffer(f"second draw of {text!r} in font {font}")
            count += 2
    return count


TESTS = [
    ("line_width", test_line_width),
    ("draw_line", test_draw_line),
    ("draw_line_blend_reuse", test_draw_line_blend_reuse),
]
