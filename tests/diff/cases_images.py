"""Cases for RGN load and image blit."""
import os
import struct
import tempfile

from original import HEIGHT, WIDTH

FUN_BLIT = 0x5E3FD0
FUN_LOAD_RGN = 0x5E40D0

RGB565, RGB555 = 0x07E0, 0x03E0


def _pixels(ctx, width, height):
    """Random pixels with runs of zero, because zero is not transparent in the original."""
    data = bytearray(ctx.rng.randbytes(width * height * 2))
    for _ in range(width * height // 8):
        start = ctx.rng.randrange(width * height) * 2
        data[start:start + 6] = bytes(len(data[start:start + 6]))
    return bytes(data)


def _rgn(width, height, pixels, g_mask=RGB565, data_size=None):
    r_mask = 0xF800 if g_mask == RGB565 else 0x7C00
    size = len(pixels) if data_size is None else data_size
    return struct.pack("<12I", width, height, size, 0, 32, 0x40, 0, 16, r_mask, g_mask, 0x1F, 0) + pixels


def _write(folder, name, content):
    path = os.path.join(folder, name)
    with open(path, "wb") as rgn:
        rgn.write(content)
    return path


def _original_load(ctx, path):
    return ctx.original.call(FUN_LOAD_RGN, ctx.original.alloc_c_string(path), 0)


GREEN_MASK = 8  # place in the header fields below


def _original_image(ctx, addr):
    if not addr:
        return None
    header = struct.unpack("<12I", ctx.original.read(addr, 48))
    fields = [*header[:3], *header[4:]]
    fields[GREEN_MASK] = RGB565
    return fields, ctx.original.read(header[3], header[2])


def _port_image(ctx):
    """Every header field but the pixel pointer. The original keeps the green mask of the file
    after it converts the pixels. The port records RGB565, so the test does not compare that field."""
    words = ctx.port.reply()
    if words == [b"0"]:
        return None
    fields = [int(v) for v in words]
    fields[GREEN_MASK] = RGB565
    return fields, ctx.port.read(fields[2])


def test_load(ctx):
    with tempfile.TemporaryDirectory() as folder:
        files = [os.path.join(folder, "missing.rgn"), _write(folder, "short_header.rgn", bytes(40))]
        for case in range(60):
            width, height = ctx.rng.randint(1, 120), ctx.rng.randint(1, 90)
            # The original converts for any green mask but that of the display.
            g_mask = ctx.rng.choice([RGB565, RGB565, RGB555, 0, 0x07C0])
            content = _rgn(width, height, _pixels(ctx, width, height), g_mask)
            if case % 10 == 9:
                content = content[:-ctx.rng.randint(1, width * 2)]
            files.append(_write(folder, f"{case}.rgn", content))
        for path in files:
            ctx.original.reset_heap()
            ctx.port.send("load", path)
            ctx.expect_equal(f"load {os.path.basename(path)}",
                             _original_image(ctx, _original_load(ctx, path)), _port_image(ctx))
    return len(files)


def test_load_rejects_wrong_data_size(ctx):
    """Port only. The original trusts data_size and then reads width * height pixels."""
    with tempfile.TemporaryDirectory() as folder:
        pixels = _pixels(ctx, 20, 10)
        files = [
            _write(folder, "small.rgn", _rgn(20, 10, pixels[:200])),
            _write(folder, "large.rgn", _rgn(20, 10, pixels + bytes(64))),
            _write(folder, "zero.rgn", _rgn(20, 10, b"")),
        ]
        for path in files:
            ctx.port.send("load", path)
            ctx.expect_equal(f"load {os.path.basename(path)}", None, _port_image(ctx), source="wanted")
    return len(files)


def test_blit(ctx):
    ctx.sync_framebuffer(7)
    with tempfile.TemporaryDirectory() as folder:
        sizes = [(WIDTH, HEIGHT), (WIDTH + 1, 10), (10, HEIGHT + 1), (1, 1)]
        sizes += [(ctx.rng.randint(1, 200), ctx.rng.randint(1, 150)) for _ in range(120)]
        for case, (width, height) in enumerate(sizes):
            ctx.original.reset_heap()
            path = _write(folder, f"{case}.rgn", _rgn(width, height, _pixels(ctx, width, height)))
            x, y = (0, 0) if case < 3 else (ctx.origin(width, WIDTH), ctx.origin(height, HEIGHT))
            ctx.original.call(FUN_BLIT, x, y, _original_load(ctx, path))
            ctx.port.send("blit", x, y, path)
            ctx.expect_same_framebuffer(f"blit {width}x{height} at ({x}, {y})")
        missing = os.path.join(folder, "missing.rgn")
        ctx.original.call(FUN_BLIT, 5, 5, _original_load(ctx, missing))
        ctx.port.send("blit", 5, 5, missing)
        ctx.expect_same_framebuffer("blit with no image")
    return len(sizes) + 1


TESTS = [
    ("load", test_load),
    ("load_rejects_wrong_data_size", test_load_rejects_wrong_data_size),
    ("blit", test_blit),
]
