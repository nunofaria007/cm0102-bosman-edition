"""Cases for the panel draw and the colour constants."""
import struct

FUN_PANEL = 0x5E5DF0

DARKEN, GRADIENT_H, GRADIENT_V, FILL, BORDER = 0x2, 0x4, 0x8, 0x10, 0x20
ELLIPSE = 0x10000        # not ported
SHRINK, INSET = 0x20000, 0x40000

# Colour globals of the original, in the order the port driver prints its constants.
PALETTE = [0xAEBE04, 0xAEBDEC, 0xAEBDEA, 0xAE3184, 0xAEBDD4, 0xAE31A8, 0xAE317E]


def test_palette(ctx):
    expected = [struct.unpack("<H", ctx.original.read(addr, 2))[0] for addr in PALETTE]
    ctx.port.send("palette")
    ctx.expect_equal("palette", expected, [int(v) for v in ctx.port.reply()])
    return len(PALETTE)


def divides_by_zero(x1, y1, x2, y2, flags):
    """The original divides by the gradient span, so a one-pixel gradient faults."""
    if flags & SHRINK:
        x2, y2 = x2 - 2, y2 - 2
    if flags & INSET:
        x1, y1 = x1 + 2, y1 + 2
    if flags & (DARKEN | FILL):
        return False
    if flags & GRADIENT_H:
        return x1 == x2
    return bool(flags & GRADIENT_V) and y1 == y2


def _draw(ctx, box, flags, colour):
    args = (*box, flags, colour)
    ctx.original.call(FUN_PANEL, *args, 0)
    ctx.port.send("panel", *args)
    ctx.expect_same_framebuffer(f"panel{box} flags {flags:#x} colour {colour:#06x}")


def test_panel_each_flag(ctx):
    """Each flag alone and with fill and border, on a panel that gets a thick bevel and on two that do not."""
    ctx.sync_framebuffer(8)
    boxes = [(100, 20, 300, 90), (350, 150, 600, 400), (20, 500, 50, 520)]
    count = 0
    for bit in range(31):
        if 1 << bit == ELLIPSE:
            continue
        for extra in (0, FILL, BORDER, FILL | BORDER):
            for box in boxes:
                for colour in (0, ctx.rng.randint(1, 0xFFFF)):
                    _draw(ctx, box, 1 << bit | extra, colour)
                    count += 1
    return count


def test_panel_bevel_limits(ctx):
    """The bevel is thick only for a filled panel of 50 or more each way that ends at or above y 99."""
    ctx.sync_framebuffer(10)
    count = 0
    for width in (49, 50):
        for height in (49, 50):
            for bottom in (99, 100):
                for flags in (BORDER, FILL | BORDER, FILL | BORDER | 0x40):
                    for moved in (0, SHRINK, INSET, SHRINK | INSET):
                        # The rule applies to the box after the two flags move its sides.
                        far, near = (2 if moved & SHRINK else 0), (2 if moved & INSET else 0)
                        box = (200 - near, bottom - height - near, 200 + width + far, bottom + far)
                        _draw(ctx, box, flags | moved, ctx.rng.randint(1, 0xFFFF))
                        count += 1
    return count


def test_panel_random(ctx):
    ctx.sync_framebuffer(9)
    count = 0
    while count < 1500:
        flags = ctx.rng.getrandbits(31)
        for _ in range(ctx.rng.randint(0, 3)):
            flags &= ctx.rng.getrandbits(31)
        flags &= ~ELLIPSE
        box = (ctx.x(), ctx.y(), ctx.x(), ctx.y())
        if divides_by_zero(*box, flags):
            continue
        _draw(ctx, box, flags, ctx.colour())
        count += 1
    return count


TESTS = [
    ("palette", test_palette),
    ("panel_each_flag", test_panel_each_flag),
    ("panel_bevel_limits", test_panel_bevel_limits),
    ("panel_random", test_panel_random),
]
