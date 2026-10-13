"""Cases for area draw and scrollbar draw."""
from cases_area import FUN_AREA_LAYOUT, _weights
from cases_panel import ELLIPSE, divides_by_zero
from cases_widget import OTHER_FLAGS, TEXT_FONTS, TEXTS, _some, _text, text_flags
from scene import Scene

FUN_AREA_DRAW = 0x402E70

NO_MARGIN, FILL, BORDER, GRID_ONLY = 0x1, 0x10, 0x20, 0x80000
GRADIENTS = 0xC
LINE_NO_FIRST_WIDGET = 0xF5

AREA, POPUP_OWNER = 1, 23


def _area_flags(ctx, box):
    while True:
        flags = _some(ctx, [NO_MARGIN, FILL, BORDER], 0.4)
        extra = ctx.rng.getrandbits(26) & ctx.rng.getrandbits(26) & ctx.rng.getrandbits(26)
        flags |= extra & ~ELLIPSE & ~GRID_ONLY
        if ctx.rng.random() < 0.12:
            flags |= GRID_ONLY
        if not divides_by_zero(*box, flags):
            return flags


def _member(ctx, columns, total_rows):
    """A widget of the area. Its place on screen comes from the grid unless it has a fixed place."""
    style = ctx.rng.getrandbits(26) & ctx.rng.getrandbits(26) & ~ELLIPSE & ~GRADIENTS
    widget = {
        "column": ctx.rng.randrange(columns), "row": ctx.rng.randrange(total_rows),
        "flags": _some(ctx, OTHER_FLAGS, 0.2), "style": style | ctx.rng.choice([0, 0x20, 0x30]),
        "colour": ctx.colour(), "highlight_colour": ctx.colour(),
        "font": ctx.rng.choice(TEXT_FONTS), "text_colour": ctx.colour(),
        "text_flags": text_flags(ctx), "cursor": ctx.rng.choice([-1, -1, ctx.rng.randint(0, 12)]),
        "text": _text(ctx.rng.choice(TEXTS)),
        "lighten_once": ctx.rng.choice([0, 0, 0, 1]), "highlighted": ctx.rng.choice([0, 0, 0, 1]),
        "area": AREA, "popup_area": -1,
    }
    x1, y1 = ctx.rng.randint(1, 700), ctx.rng.randint(1, 550)
    if ctx.rng.random() < 0.15:
        widget.update({"fixed_x1": x1, "fixed_x2": x1 + ctx.rng.randint(5, 90)})
    if ctx.rng.random() < 0.15:
        widget.update({"fixed_y1": y1, "fixed_y2": y1 + ctx.rng.randint(5, 40)})
    return widget


def _out_of_order(ctx, columns, total_rows):
    """A widget for the end of the list whose row can be above the rows before it. Its y place is fixed,
    because a row above the first row in view has no entry in the row table."""
    y1 = ctx.rng.randint(1, 550)
    return {**_member(ctx, columns, total_rows), "fixed_y1": y1, "fixed_y2": y1 + ctx.rng.randint(5, 40)}


def _layout(ctx, scene):
    ctx.original.call(FUN_AREA_LAYOUT, this=scene.addr("area", AREA))
    ctx.port.send("layout", AREA)


def _draw(ctx, scene, members, redraw, case):
    ctx.original.call(FUN_AREA_DRAW, redraw, this=scene.addr("area", AREA))
    ctx.port.send("draw_area", AREA, redraw)
    ctx.expect_same_framebuffer(case)
    scene.expect_same(case, "area", AREA)
    scene.expect_same_image(case, "area", AREA, "saved")
    scene.expect_same_image(case, "area", AREA, "scrollbar")
    for widget in members:
        scene.expect_same(case, "widget", widget)
        scene.expect_same_image(case, "widget", widget, "saved")


def test_area_draw(ctx):
    """One area, laid out and drawn, then scrolled, laid out again and drawn again."""
    ctx.sync_framebuffer(14)
    errors = []
    for case in range(500):
        scene = Scene(ctx)

        columns, rows = ctx.rng.randint(1, 6), ctx.rng.randint(1, 10)
        total_rows = rows + (ctx.rng.randint(1, 12) if ctx.rng.random() < 0.5 else 0)
        x1, y1 = ctx.rng.randint(0, 250), ctx.rng.randint(0, 150)
        box = (x1, y1, x1 + ctx.rng.randint(200, 500), y1 + ctx.rng.randint(120, 400))
        widgets = sorted((_member(ctx, columns, total_rows) for _ in range(ctx.rng.randint(0, 20))),
                         key=lambda w: w["row"])
        if ctx.rng.random() < 0.4:
            widgets.append(_out_of_order(ctx, columns, total_rows))
        members = list(range(len(widgets)))
        for index, widget in zip(members, widgets):
            scene.set("widget", index, widget)
        scene.set("area", AREA, {
            "x1": box[0], "y1": box[1], "x2": box[2], "y2": box[3], "flags": _area_flags(ctx, box),
            # A first row above 0 moves the widgets down the row table. The table has 30 rows.
            "colour": ctx.colour(), "first_row": ctx.rng.choice([0, 0, ctx.rng.randint(0, 30 - rows)]),
            "last_row": total_rows - 1,
            "scroll_to": ctx.rng.choice([-1, ctx.rng.randrange(total_rows)]),
            "owner_widget": ctx.rng.choice([-1, -1, -1, POPUP_OWNER]), "index": AREA, "shown": 1,
            "column_count": columns, "row_count": rows,
            "column_weight": [max(w, 1) for w in _weights(ctx, columns)],
            "row_weight": [max(w, 1) for w in _weights(ctx, rows)],
            "widget_index": members, "widget_count": len(members),
        })
        _layout(ctx, scene)
        scene.set("area", AREA, {"scrollbar.pressed": ctx.rng.choice([0, 0, 1, 2])})
        if ctx.rng.random() < 0.05:
            scene.set("area", AREA, {"first_visible": -1})

        ctx.original.error_lines.clear()
        _draw(ctx, scene, members, 1, f"area case {case}")
        errors += ctx.original.error_lines

        scene.set("area", AREA, {"scroll_to": ctx.rng.randrange(total_rows)})
        _layout(ctx, scene)
        scene.set("area", AREA, {"scrollbar.pressed": ctx.rng.choice([0, 1, 2])})
        _draw(ctx, scene, members, ctx.rng.randint(0, 1), f"area case {case}, after a scroll")

        # No layout before this draw, so the saved screen of the scrollbar stays in use.
        scene.set("area", AREA, {"scrollbar.pressed": ctx.rng.choice([0, 1, 2])})
        _draw(ctx, scene, members, ctx.rng.randint(0, 1), f"area case {case}, drawn again")
    ctx.expect_equal("the original met an area with no first widget", True,
                     LINE_NO_FIRST_WIDGET in errors, source="wanted")
    return (case + 1) * 3


TESTS = [
    ("area_draw", test_area_draw),
]
