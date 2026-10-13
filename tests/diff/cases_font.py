"""Cases for the bitmap fonts."""
import os
import struct
import tempfile

from original import FONT_FILES, FONT_SIZE, G_FONTS


def _original_font(ctx, font):
    """Height, then width, bytes in a row, left gap, right gap and pixel rows of each of the 256 glyphs."""
    raw = ctx.original.read(G_FONTS + font * FONT_SIZE, FONT_SIZE)
    height = struct.unpack_from("<i", raw, 0)[0]
    glyphs = []
    for code in range(256):
        width, row_bytes, left, right, rows = struct.unpack_from("<4iI", raw, 4 + code * 20)
        glyphs.append((width, row_bytes, left, right, ctx.original.read(rows, row_bytes * height) if rows else b""))
    return height, glyphs


def _port_font(ctx, font):
    ctx.port.send("font_dump", font)
    words = ctx.port.reply()
    height = int(words[0])
    glyphs = []
    for code in range(256):
        width, row_bytes, left, right, rows = words[1 + code * 5:6 + code * 5]
        glyphs.append((int(width), int(row_bytes), int(left), int(right), bytes.fromhex(rows[1:].decode())))
    return height, glyphs


def _expect_same_font(ctx, font):
    original_height, original_glyphs = _original_font(ctx, font)
    port_height, port_glyphs = _port_font(ctx, font)
    ctx.expect_equal(f"font {font} height", original_height, port_height)
    for code in range(256):
        ctx.expect_equal(f"font {font} glyph {code:#04x}", original_glyphs[code], port_glyphs[code])


def test_font_tables(ctx):
    """Fonts 1 to 7 as the original and the port hold them after the load at the start."""
    for font in FONT_FILES:
        _expect_same_font(ctx, font)
    ctx.expect_equal("font 0 of the port", (0, [(0, 0, 0, 0, b"")] * 256), _port_font(ctx, 0), source="wanted")
    return len(FONT_FILES) * 256


def test_font_load_fails(ctx):
    """A file that is missing or cut short fails on both sides. The test then loads the font again."""
    font = 7
    good = os.path.join(ctx.data_dir, FONT_FILES[font])
    with open(good, "rb") as source:
        whole = source.read()
    with tempfile.TemporaryDirectory() as folder:
        paths = [os.path.join(folder, "missing.fnt")]
        for cut in (0, 3, 4, 10, 20, 21, 500, len(whole) - 1):
            paths.append(os.path.join(folder, f"cut_{cut}.fnt"))
            with open(paths[-1], "wb") as target:
                target.write(whole[:cut])
        for path in paths + [good]:
            ctx.port.send("font_load", font, path)
            ctx.expect_equal(f"load {os.path.basename(path)}", ctx.original.load_font(font, path),
                             int(ctx.port.reply()[0]))
    _expect_same_font(ctx, font)
    return len(paths) + 1


TESTS = [
    ("font_tables", test_font_tables),
    ("font_load_fails", test_font_load_fails),
]
