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


def _font_file(height, glyphs):
    """A .fnt file. `glyphs` maps a character code to (width, bytes in a row, left gap, right gap)."""
    content = struct.pack("<i", height)
    for code in range(0x20, 0x100):
        width, row_bytes, left, right = glyphs.get(code, (2, 1, 0, 0))
        content += struct.pack("<4i", width, row_bytes, left, right) + bytes(max(row_bytes, 0) * max(height, 0))
    return content


def test_font_load_rejects_bad_values(ctx):
    """Port only. The original trusts every value in the file."""
    font = 7
    good = os.path.join(ctx.data_dir, FONT_FILES[font])
    files = {
        "sound": (10, {}, 1),
        "height_0": (0, {}, 0), "height_256": (256, {}, 0), "height_below_0": (-1, {}, 0),
        "row_too_short": (10, {0x41: (5, 2, 0, 0)}, 0), "row_too_long": (10, {0x41: (5, 4, 0, 0)}, 0),
        "rows_but_no_width": (10, {0x41: (0, 1, 0, 0)}, 0), "width_256": (10, {0x41: (256, 0, 0, 0)}, 0),
        "width_below_0": (10, {0x41: (-1, 0, 0, 0)}, 0), "left_gap_256": (10, {0x41: (2, 1, 256, 0)}, 0),
        "right_gap_far_below_0": (10, {0x41: (2, 1, 0, -256)}, 0),
    }
    with tempfile.TemporaryDirectory() as folder:
        for name, (height, glyphs, wanted) in files.items():
            path = os.path.join(folder, name + ".fnt")
            with open(path, "wb") as target:
                target.write(_font_file(height, glyphs))
            ctx.port.send("font_load", font, path)
            ctx.expect_equal(f"load {name}", wanted, int(ctx.port.reply()[0]), source="wanted")
    ctx.port.send("font_load", font, good)
    ctx.expect_equal("load the real font again", 1, int(ctx.port.reply()[0]), source="wanted")
    _expect_same_font(ctx, font)
    return len(files) + 1


def test_font_0_draws_nothing(ctx):
    """Port only. The original has no font 0, and the port gives text in font 0 no pixels at all."""
    ctx.port.send("fill", 30)
    before = ctx.port.framebuffer()
    ctx.port.send("draw_line", 100, 100, 0, 0xFFFF, 0, "-" + b"Text".hex())
    ctx.port.send("text", 100, 200, 400, 260, 0, 0, 0xFFFF, 0, "-" + b"Text".hex())
    ctx.expect_equal("the picture after text in font 0", before == ctx.port.framebuffer(), True, source="wanted")
    ctx.port.send("line_width", 0, "-" + b"Text".hex())
    ctx.expect_equal("width of text in font 0", 0, int(ctx.port.reply()[0]), source="wanted")
    return 3


TESTS = [
    ("font_load_rejects_bad_values", test_font_load_rejects_bad_values),
    ("font_0_draws_nothing", test_font_0_draws_nothing),
    ("font_tables", test_font_tables),
    ("font_load_fails", test_font_load_fails),
]
