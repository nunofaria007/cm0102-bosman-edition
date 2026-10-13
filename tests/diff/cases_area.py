"""Cases for area layout, scrollbar setup and popup position."""
import struct

from scene import Scene

FUN_AREA_LAYOUT = 0x403350
FUN_POPUP_POSITION = 0x403760

NO_MARGIN, FILL, POPUP_BELOW, GRID_ONLY = 0x1, 0x10, 0x100, 0x80000
POPUP_SIDES = [0x100000, 0x200000, 0x400000, 0x800000]

LINE_TOO_MANY_COLUMNS, LINE_TOO_MANY_ROWS, LINE_POPUP_NO_OWNER = 0x1F1, 0x1FB, 0x2CE


# Set to noise before each layout, so a field the port forgets to write shows up.
STALE_SCROLLBAR_FIELDS = ["x1", "y1", "x2", "y2", "track_y1", "track_y2", "max_position", "position",
                          "thumb_height", "thumb_y1", "thumb_y2"]


def _weights(ctx, count):
    """Weights that do not all add up to 0, because the original divides by the sum."""
    weights = [ctx.rng.choice([0, 1, 1, 2, 3, ctx.rng.randint(1, 255)]) for _ in range(count)]
    if count and not any(weights):
        weights[ctx.rng.randrange(count)] = 1
    return weights


def _random_area(ctx):
    x1, y1 = ctx.rng.randint(0, 600), ctx.rng.randint(0, 400)
    columns = ctx.rng.choice([0, 1, 2, 3, 5, 30, ctx.rng.randint(1, 30)])
    rows = ctx.rng.choice([0, 1, 2, 5, 30, ctx.rng.randint(1, 30)])
    oversize = ctx.rng.random()
    if oversize < 0.04:
        columns = ctx.rng.choice([31, 255])
    elif oversize < 0.08:
        rows = ctx.rng.choice([31, 255])
    scrolls = ctx.rng.random() < 0.5
    last_row = rows + ctx.rng.randint(0, 60) if scrolls else rows - 1 - ctx.rng.randint(0, 2)
    flags = 0
    for flag, chance in ((NO_MARGIN, 0.25), (FILL, 0.3), (GRID_ONLY, 0.08), (0x20, 0.3)):
        if ctx.rng.random() < chance:
            flags |= flag
    return {
        "x1": x1, "y1": y1,
        "x2": x1 + ctx.rng.choice([ctx.rng.randint(30, 700), ctx.rng.randint(0, 30)]),
        "y2": y1 + ctx.rng.choice([ctx.rng.randint(30, 500), ctx.rng.randint(0, 50)]),
        "first_row": ctx.rng.randint(0, 5), "last_row": last_row, "flags": flags,
        "scroll_to": ctx.rng.choice([-1, -1, ctx.rng.randint(-3, max(last_row, 0) + 3)]),
        "index": ctx.rng.randint(0, 249), "first_visible": ctx.rng.randint(0, 9), "shown": 1,
        "has_scrollbar": ctx.rng.randint(0, 1), "scrollbar.pressed": ctx.rng.randint(0, 2),
        **{f"scrollbar.{name}": ctx.rng.randint(-50, 900) for name in STALE_SCROLLBAR_FIELDS},
        "column_count": columns, "row_count": rows,
        "column_weight": _weights(ctx, min(columns, 30)), "row_weight": _weights(ctx, min(rows, 30)),
    }


def test_area_layout(ctx):
    Scene.check_tables(ctx)
    errors = set()
    for case in range(1500):
        scene = Scene(ctx)
        area = _random_area(ctx)
        members = sorted(ctx.rng.sample(range(24), ctx.rng.randint(0, 20)))
        rows = sorted(ctx.rng.randint(0, max(area["last_row"], 0)) for _ in members)
        for widget, row in zip(members, rows):
            scene.set("widget", widget, {"row": row, "popup_area": -1})
        area["widget_index"] = members
        area["widget_count"] = len(members)
        area["owner_widget"] = ctx.rng.choice([-1, -1, ctx.rng.randrange(24)])
        scene.set("area", 1, area)
        # The layout clears the saved screen of the scrollbar and leaves that of the area.
        for which in ("saved", "scrollbar"):
            if ctx.rng.random() < 0.5:
                scene.capture_to("area", 1, which, (0, 0, 7, 5))

        ctx.original.error_lines.clear()
        ctx.original.call(FUN_AREA_LAYOUT, this=scene.addr("area", 1))
        ctx.port.send("layout", 1)
        errors.update(ctx.original.error_lines)
        scene.expect_same(f"layout case {case}", "area", 1)
        for which in ("saved", "scrollbar"):
            scene.expect_same_image(f"layout case {case}", "area", 1, which)
        if area["owner_widget"] != -1:
            scene.expect_same(f"layout case {case}", "widget", area["owner_widget"])
    ctx.expect_equal("error boxes seen in the original", {LINE_TOO_MANY_COLUMNS, LINE_TOO_MANY_ROWS},
                     errors, source="wanted")
    return case + 1


def _box(ctx, max_width, max_height):
    x1, y1 = ctx.rng.randint(0, 799), ctx.rng.randint(0, 599)
    return {"x1": x1, "y1": y1, "x2": x1 + ctx.rng.randint(0, max_width), "y2": y1 + ctx.rng.randint(0, max_height)}


def _owner(ctx, popup):
    """A widget box that often sits on a limit of the popup rules for this popup area."""
    height = popup["y2"] - popup["y1"]
    box = _box(ctx, 150, 40)
    roll = ctx.rng.random()
    if roll < 0.2:
        box["y1"] = 600 - height + ctx.rng.choice([-1, 0, 1])
        box["y2"] = box["y1"] + 20
    elif roll < 0.4:
        box["y2"] = 597 - height + ctx.rng.choice([-1, 0, 1])
        box["y1"] = box["y2"] - 20
    elif roll < 0.5:
        box["y2"] = height + ctx.rng.choice([-1, 0, 1, 2])
        box["y1"] = box["y2"] - 20
    if ctx.rng.random() < 0.3:
        width = box["x2"] - box["x1"]
        box["x1"] = ctx.rng.choice([399, 400, 401, 402])
        box["x2"] = box["x1"] + width
    return box


def _popup_flags(ctx):
    flags = POPUP_BELOW if ctx.rng.random() < 0.3 else 0
    for side in POPUP_SIDES:
        if ctx.rng.random() < 0.2:
            flags |= side
    return flags


def test_popup_position(ctx):
    """Area 1 is a popup area of widget 0. Area 2 is a popup area of widget 1, which sits in area 1."""
    out = None
    for case in range(1500):
        scene = Scene(ctx)
        out = ctx.original.alloc(8)
        popup1, popup2 = _box(ctx, 300, 400), _box(ctx, 300, 400)
        widget0, widget1 = _owner(ctx, popup1), _owner(ctx, popup2)
        if ctx.rng.random() < 0.3:
            width = popup1["x2"] - popup1["x1"]
            popup1["x1"] = widget0["x2"] + ctx.rng.choice([-1, 0, 1])
            popup1["x2"] = popup1["x1"] + width
        scene.set("area", 0, {**_box(ctx, 400, 300), "owner_widget": -1})
        scene.set("widget", 0, {**widget0, "area": ctx.rng.choice([0, 0, -1])})
        scene.set("area", 1, {**popup1, "owner_widget": 0, "flags": _popup_flags(ctx)})
        scene.set("widget", 1, {**widget1, "area": 1})
        scene.set("area", 2, {**popup2, "owner_widget": 1, "flags": _popup_flags(ctx)})
        scene.set("area", 3, {**_box(ctx, 300, 400), "owner_widget": -1})

        ctx.original.error_lines.clear()
        for area in (1, 2, 3):
            ctx.original.write(out, struct.pack("<2i", -9999, -9999))
            ctx.original.call(FUN_POPUP_POSITION, out, out + 4, this=scene.addr("area", area))
            ctx.port.send("popup", area)
            ctx.expect_equal(f"popup case {case} area {area}",
                             list(struct.unpack("<2i", ctx.original.read(out, 8))),
                             [int(v) for v in ctx.port.reply()])
        ctx.expect_equal("error boxes seen in the original", [LINE_POPUP_NO_OWNER],
                         ctx.original.error_lines, source="wanted")
    return (case + 1) * 3


TESTS = [
    ("area_layout", test_area_layout),
    ("popup_position", test_popup_position),
]
