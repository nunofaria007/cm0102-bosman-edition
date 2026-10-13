"""Cases for a line of text and for text in a box."""
import re

from original import HEIGHT, WIDTH

FUN_LINE_WIDTH = 0x5E5AD0   # (font, text)
FUN_DRAW_LINE = 0x5E5160    # (x, y, font, colour, text, cursor)
FUN_TEXT = 0x5E6D80         # (x1, y1, x2, y2, flags, font, colour, text, cursor)

LEFT, TOP, EMBOSS, RIGHT, PASSWORD, WRAP = 0x1, 0x2, 0x20, 0x40, 0x80, 0x100

FONTS = [1, 2, 3, 4, 5, 6, 7]
CODE = re.compile(rb"#([0-9a-fA-F]{4})#")

# Bytes above 0x7f that the digit test of the original accepts in a character code, and some it does not.
ALSO_DIGITS = [0x83, 0x87, 0x8B, 0x9F, 0xAF, 0xB5, 0xC1, 0xD7, 0xE7, 0xEA, 0xFC, 0xFE]
NOT_DIGITS = [0x80, 0x84, 0xA0, 0xE9, 0xFF]


def odd_code(ctx):
    """A code group with a byte above 0x7f in it. The code is then the hex digits before that byte."""
    digits = b"0123456789abcdefABCDEF"
    place = ctx.rng.randint(1, 4)
    high = ctx.rng.choice(ALSO_DIGITS + ALSO_DIGITS + NOT_DIGITS)
    group = bytearray(ctx.rng.choice(digits) for _ in range(4))
    for later in range(place, 4):
        group[later] = ctx.rng.choice(list(digits) + ALSO_DIGITS)
    group[place - 1] = high
    if place == 4:
        group[0] = ord("0")
    return b"#" + bytes(group) + b"#"


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
            elif roll < 0.95:
                parts.append(ctx.rng.choice([b"#%04x#", b"#%04X#"]) % ctx.rng.choice([0, 0x41, ctx.rng.randint(1, 0xFF)]))
            elif roll < 0.97:
                parts.append(odd_code(ctx))
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
        text = random_text(ctx, 40 if case % 50 else 3000)
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
        cursor = ctx.rng.choice([-1, -1, -2, ctx.rng.randint(0, len(text) + 2)])
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


def random_words(ctx, max_words):
    """Words with spaces between them, and now and then a line break, a bar, a hash or a character code."""
    letters = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPW il.,'-0123456789"
    words = []
    for _ in range(ctx.rng.randint(0, max_words)):
        word = bytes(ctx.rng.choice(letters) for _ in range(ctx.rng.randint(1, 9)))
        roll = ctx.rng.random()
        if roll < 0.06:
            word += b"\n"
        elif roll < 0.10:
            word += b"#00e9#"
        elif roll < 0.13:
            word = b"#" + word
        elif roll < 0.16:
            word += b"|"
        words.append(word)
    return ctx.rng.choice([b" ", b" ", b" ", b"  "]).join(words)[:251]


def _flags(ctx):
    flags = ctx.rng.getrandbits(32) & ctx.rng.getrandbits(32)
    for flag, chance in ((LEFT, 0.3), (TOP, 0.3), (EMBOSS, 0.15), (RIGHT, 0.3), (PASSWORD, 0.1), (WRAP, 0.6)):
        flags = flags | flag if ctx.rng.random() < chance else flags & ~flag
    return flags


def test_text_box(ctx):
    ctx.sync_framebuffer(22)
    for case in range(2500):
        ctx.original.reset_heap()
        x1, y1 = ctx.rng.randint(0, 500), ctx.rng.randint(0, 420)
        box = (x1, y1, x1 + ctx.rng.choice([ctx.rng.randint(20, 290), ctx.rng.randint(1, 60)]),
               y1 + ctx.rng.randint(5, 170))
        font = ctx.rng.choice(FONTS + FONTS + [-1])
        text = ctx.rng.choice([random_words(ctx, 14), random_words(ctx, 40), random_text(ctx, 30),
                               random_words(ctx, 90)])
        flags, colour = _flags(ctx), ctx.colour()
        cursor = ctx.rng.choice([-1, -1, -1, -2, -9, ctx.rng.randint(0, 20)])
        ctx.original.call(FUN_TEXT, *box, flags, font, colour, ctx.original.alloc_bytes(text + b"\0"), cursor)
        ctx.port.send("text", *box, flags, font, colour, cursor, as_word(text))
        ctx.expect_same_framebuffer(f"text {text!r} in {box} flags {flags:#x} font {font} cursor {cursor}")
    return case + 1


def test_text_box_long_text(ctx):
    """Texts at the limit of 251 characters, with and without spaces, in boxes that the end of the text
    crosses. A longer text writes past the line buffer of the original, so no case has one."""
    ctx.sync_framebuffer(24)
    count = 0
    for length in (240, 249, 250, 251):
        for spaced in (False, True):
            # Narrow characters, so that the whole line fits on the screen and its last character shows.
            text = (b"!. !. " * 50 if spaced else b"!" * 300)[:length]
            for font in (1,):
                width = ctx.original.call(FUN_LINE_WIDTH, font, ctx.original.alloc_bytes(text + b"\0"))
                ctx.expect_equal(f"{length} characters in font {font} fit on the screen", True, width < 780,
                                 source="wanted")
                # A box of width + slack + 1 pixels. At -2 only the last character does not fit.
                for slack in (-40, -3, -2, -1, 0):
                    for flags in (LEFT | TOP, LEFT | TOP | WRAP):
                        box = (5, 100 + 60 * (count % 6), 5 + width + slack, 150 + 60 * (count % 6))
                        ctx.original.reset_heap()
                        ctx.original.call(FUN_TEXT, *box, flags, font, 0xFFFF,
                                          ctx.original.alloc_bytes(text + b"\0"), -1)
                        ctx.port.send("text", *box, flags, font, 0xFFFF, -1, as_word(text))
                        ctx.expect_same_framebuffer(f"{length} characters, spaced {spaced}, font {font}, "
                                                    f"box {slack} wider than the text, flags {flags:#x}")
                        count += 1
    return count


def test_text_box_wrap_quirk(ctx):
    """A third line whose text is shorter than the first. The original then finds a space of the first
    line beyond the end of the current line, leaves the line too wide and repeats words."""
    ctx.sync_framebuffer(23)
    texts = [b"iiii iiii iiii iiii iiii WWWWWW WWWWWW WWWWWW WWWWWW ii ii ii",
             b"one two three four five six SEVENSEVEN EIGHTEIGHT NINENINE ten eleven twelve",
             b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
             b"a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R",
             b"word #00e9##00e9##00e9##00e9##00e9##00e9##00e9##00e9##00e9##00e9##00e9##00e9# end of it"]
    count = 0
    for text in texts:
        for font in (1, 3, 5):
            for width in (60, 110, 170):
                box = (100, 50, 100 + width, 450)
                ctx.sync_framebuffer(23)
                ctx.original.call(FUN_TEXT, *box, WRAP | TOP | LEFT, font, 0xFFFF,
                                  ctx.original.alloc_bytes(text + b"\0"), -1)
                ctx.port.send("text", *box, WRAP | TOP | LEFT, font, 0xFFFF, -1, as_word(text))
                ctx.expect_same_framebuffer(f"wrap of {text[:20]!r} in font {font}, width {width}")
                count += 1
    return count


TESTS = [
    ("text_box", test_text_box),
    ("text_box_long_text", test_text_box_long_text),
    ("text_box_wrap_quirk", test_text_box_wrap_quirk),
    ("line_width", test_line_width),
    ("draw_line", test_draw_line),
    ("draw_line_blend_reuse", test_draw_line_blend_reuse),
]
