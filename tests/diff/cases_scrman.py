"""Screen manager: the constructor, the reset, area create and widget create."""
import os
import tempfile

from cases_images import _pixels, _rgn, _write
from scene import KINDS, ManagerScene

SPARE_AREAS = 3                  # areas past those in use that the reset test also fills
SPARE_WIDGETS = 3                # the same for widgets
MAX_AREAS = 249                  # the original never uses the last of its 250 slots
LINE_ORDER_FULL = 0x302          # display.cpp: no room in the draw order

FLAG_BITS = [0x1, 0x100, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000]

# Fields that a create or a reset reads. A test that scrambles them would send the original out
# of its arrays.
COUNTERS = {"area_slots", "widget_slots", "area_count", "widget_count", "area_order", "widget_order"}

# Fields of an area that the test scrambles before a create takes its slot.
AREA_LEFTOVERS = ["first_row", "last_row", "column_weight", "row_weight", "shown", "has_scrollbar",
                  "widget_count", "first_visible", "scroll_to", "unknown_210", "scrollbar.pressed",
                  "scrollbar.position", "column_x1", "widget_index"]


def _box(ctx):
    roll = ctx.rng.random()
    if roll < 0.05:
        return [ctx.rng.randint(-60, 1) for _ in range(4)]
    if roll < 0.1:
        return [ctx.rng.randint(-0x80000000, 0x7FFFFFFF) for _ in range(4)]
    return [ctx.x(), ctx.y(), ctx.x(), ctx.y()]


def _count(ctx):
    return ctx.rng.choice([0, 1, 1, 2, 3, 5, 12, 29, 30, ctx.rng.randint(0, 30)])


def _weights(ctx, count):
    """No table, or one weight for each column or row."""
    if ctx.rng.random() < 0.4:
        return None
    return [ctx.rng.choice([0, 1, 1, 2, ctx.rng.randint(0, 255)]) for _ in range(count)]


def _area_arguments(ctx, owner=None):
    columns, rows = _count(ctx), _count(ctx)
    flags = ctx.rng.getrandbits(32) if ctx.rng.random() < 0.2 else 0
    for bit in FLAG_BITS:
        if ctx.rng.random() < 0.2:
            flags |= bit
    if owner is None:
        owner = -1 if ctx.rng.random() < 0.7 else ctx.rng.choice([0, 1199, -2, -0x8000, ctx.rng.randint(0, 1199)])
    return [_box(ctx), columns, _weights(ctx, columns), rows, _weights(ctx, rows), flags, ctx.colour(), owner]


def _capture(ctx, scene, kind, index, which):
    x, y = ctx.rng.randint(0, 700), ctx.rng.randint(0, 500)
    scene.capture_to(kind, index, which, [x, y, x + ctx.rng.randint(0, 40), y + ctx.rng.randint(0, 40)])


def _scramble_manager(ctx, scene):
    scene.scramble("manager", 0, {name for name, _, _, _ in KINDS["manager"][0]} - COUNTERS)


def test_manager_new(ctx):
    ManagerScene.check_tables(ctx)
    for case in range(40):
        scene = ManagerScene(ctx)
        scene.scramble("manager", 0)
        ctx.sync_framebuffer(case)
        for which in ("unknown_12f521", "background"):
            if ctx.rng.random() < 0.7:
                _capture(ctx, scene, "manager", 0, which)
        if ctx.rng.random() < 0.7:
            scene.set_pointer()
        scene.start_free_count()
        scene.new(ctx.rng.choice([0, 1, ctx.rng.getrandbits(32)]))
        scene.expect_same_frees(f"new {case}")
        scene.expect_same_pointer(f"new {case}")
        scene.expect_same(f"new {case}", "manager", 0)
        for which in ("unknown_12f521", "background"):
            scene.expect_same_image(f"new {case}", "manager", 0, which)
    return case + 1


def _reset(ctx, scene, case, slots):
    """Resets after it gives saved screens to the screen manager and to some areas, in use or not."""
    slots += SPARE_AREAS
    for index in range(slots):
        for which in ("saved", "scrollbar"):
            if ctx.rng.random() < 0.5:
                _capture(ctx, scene, "area", index, which)
    _scramble_manager(ctx, scene)
    scene.set("manager", 0, {"unknown_12f555": ctx.rng.choice([0, 0, 0, 1, ctx.rng.getrandbits(31)]),
                             "widget_count": ctx.rng.randint(0, 1200)})
    if ctx.rng.random() < 0.6:
        _capture(ctx, scene, "manager", 0, "unknown_12f521")
    scene.start_free_count()
    scene.reset(ctx.rng.choice([0, 1, 1, ctx.rng.getrandbits(31)]), net_room=ctx.rng.random() < 0.9)
    scene.expect_same_frees(case)
    scene.expect_same(case, "manager", 0)
    scene.expect_same_image(case, "manager", 0, "unknown_12f521")
    for index in range(slots):
        for which in ("saved", "scrollbar"):
            scene.expect_same_image(case, "area", index, which)


def test_manager_areas(ctx):
    calls = 0
    for case in range(60):
        scene = ManagerScene(ctx)
        ctx.sync_framebuffer(case)
        scene.new(0)
        scene.reset(1)
        slots = most = 0
        for step in range(ctx.rng.randint(1, 16)):
            name = f"areas {case} step {step}"
            calls += 1
            if ctx.rng.random() < 0.2:
                _reset(ctx, scene, name, slots)
                slots = 0
                continue
            if ctx.rng.random() < 0.7:
                scene.scramble("area", slots, AREA_LEFTOVERS)
            if ctx.rng.random() < 0.5:
                _scramble_manager(ctx, scene)
            scene.start_free_count()
            index = scene.area_create(name, *_area_arguments(ctx), net_room=ctx.rng.random() < 0.9)
            scene.expect_same_frees(name)
            ctx.expect_equal(f"{name}: area index", slots, index, source="wanted")
            slots += 1
            most = max(most, slots)
            scene.expect_same(name, "manager", 0)
            scene.expect_same(name, "area", index)
            scene.expect_links(name, index)
        for index in range(most + SPARE_AREAS):
            scene.expect_same(f"areas {case} at the end", "area", index)
    return calls


def test_manager_area_limit(ctx):
    scene = ManagerScene(ctx)
    scene.new(0)
    scene.reset(1)
    for count in range(MAX_AREAS + 3):
        name = f"area limit, create {count}"
        index = scene.area_create(name, *_area_arguments(ctx))
        ctx.expect_equal(name, count if count < MAX_AREAS else -1, index, source="wanted")
        if count % 40 == 0 or count >= MAX_AREAS - 2:
            scene.expect_same(name, "manager", 0)
    for index in (0, 1, MAX_AREAS - 2, MAX_AREAS - 1, MAX_AREAS):
        scene.expect_same("area limit", "area", index)

    # A draw order with one place left takes the area. A full one leaves it out.
    for room, error in ((1, []), (0, [LINE_ORDER_FULL])):
        name = f"order limit, room for {room}"
        scene.reset(1)
        scene.expect_same(f"{name}: reset", "manager", 0)
        scene.area_create(f"{name}: first", *_area_arguments(ctx))
        scene.set("manager", 0, {"area_count": 250 - room})
        ctx.original.error_lines.clear()
        index = scene.area_create(name, *_area_arguments(ctx))
        ctx.expect_equal(f"{name}: area index", 1, index, source="wanted")
        ctx.expect_equal(f"{name}: error of the original", error, ctx.original.error_lines, source="wanted")
        scene.expect_same(name, "manager", 0)
        scene.expect_same(name, "area", 1)
    return MAX_AREAS + 7


MAX_WIDGETS = 1199               # the original never uses the last of its 1200 slots
LINE_WIDGET_ORDER_FULL = 0x2EA   # display.cpp: no room for a widget with no area

GRID_ONLY = 0x80000
PICTURE = 0x400
WIDGET_FLAG_BITS = [0x4, 0x8, 0x10, 0x40, 0x80, 0x100, 0x400, 0x800, 0x1000, 0x2000, 0x4000]
STYLE_BITS = [0x400, 0x4000]     # widget init keeps a reversed box when only the first is set
# The original has no font 0. A widget in a popup area reads the size of font 0, so no such case uses it.
FONTS = [-1, 1, 2, 3, 4, 5, 6, 7]
WORDS = [b"Arsenal", b"Manchester United", b"v", b"Caf#00e9#", b"Goals|For", b"1\x01hidden", b"Yes\nNo",
         b"%s %d", b"\xe9\xfc\xdf", b"W", b"the quick brown fox"]


def _text(ctx, limit):
    roll = ctx.rng.random()
    if roll < 0.1:
        return None
    if roll < 0.2:
        return b""
    text = b" ".join(ctx.rng.choice(WORDS) for _ in range(ctx.rng.choice([1, 1, 2, 3, 8, 30])))
    return text[:ctx.rng.choice([limit, limit, ctx.rng.randint(1, limit)])]


def _widget_arguments(ctx, area, popup):
    flags = sum(bit for bit in WIDGET_FLAG_BITS if ctx.rng.random() < 0.2)
    style = sum(bit for bit in STYLE_BITS if ctx.rng.random() < 0.3)
    if ctx.rng.random() < 0.3:
        flags |= ctx.rng.getrandbits(32) & ctx.rng.getrandbits(32)
        style |= ctx.rng.getrandbits(32) & ctx.rng.getrandbits(32)
    row = ctx.rng.choice([0, 1, 2, 3, ctx.rng.randint(0, 40), ctx.rng.randint(-5, 5),
                          ctx.rng.randint(-0x80000000, 0x7FFFFFFF)])
    font = ctx.rng.choice(FONTS if popup else FONTS + [0])
    # In a popup area the text gets five spaces in front. The port keeps 255 characters in all.
    return [flags, _box(ctx), ctx.rng.randint(-2, 31), row, style, ctx.colour(), ctx.colour(),
            ctx.rng.getrandbits(32), font, ctx.colour(), _text(ctx, 250 if popup else 255),
            ctx.rng.randint(-0x80000000, 0x7FFFFFFF), ctx.rng.randint(-0x80000000, 0x7FFFFFFF),
            ctx.rng.getrandbits(32), area]


def _build_areas(ctx, scene, name):
    """A few areas of each sort. Returns the area indexes that are popup areas."""
    popups = set()
    for index in range(ctx.rng.randint(1, 5)):
        sort = ctx.rng.choice(["plain", "plain", "popup", "popup", "grid"])
        arguments = _area_arguments(ctx, owner=-1 if sort != "popup" else ctx.rng.choice([-2, -0x8000, 0, ctx.rng.randint(0, 1198)]))
        arguments[5] = arguments[5] & ~GRID_ONLY | (GRID_ONLY if sort == "grid" else 0)
        scene.area_create(f"{name}: area {index}", *arguments)
        if sort == "popup":
            popups.add(index)
            if ctx.rng.random() < 0.4:
                scene.set("area", index, {"column_count": ctx.rng.choice([2, 3, 5, 30])})
    return popups


def _reset_with_widgets(ctx, scene, case, areas, widgets):
    for index in range(widgets + SPARE_WIDGETS):
        if ctx.rng.random() < 0.4:
            _capture(ctx, scene, "widget", index, "saved")
    _reset(ctx, scene, case, areas)
    for index in range(widgets + SPARE_WIDGETS):
        scene.expect_same_image(case, "widget", index, "saved")
        scene.expect_same_image(case, "widget", index, "picture")


def test_manager_widgets(ctx):
    ManagerScene.check_tables(ctx)
    calls = 0
    for case in range(50):
        scene = ManagerScene(ctx)
        ctx.sync_framebuffer(case)
        scene.new(0)
        scene.reset(1)
        popups = _build_areas(ctx, scene, f"widgets {case}")
        areas = most_areas = scene.area_count()
        widgets = most = 0
        held = {}
        for step in range(ctx.rng.randint(1, 30)):
            name = f"widgets {case} step {step}"
            calls += 1
            if ctx.rng.random() < 0.06:
                _reset_with_widgets(ctx, scene, name, areas, widgets)
                popups = _build_areas(ctx, scene, name)
                areas = scene.area_count()
                most_areas = max(most_areas, areas)
                widgets = 0
                held = {}
                continue
            # An area that no create made has no widget array, and the original reads address 0
            # for it. No case uses one.
            area = ctx.rng.choice([-1, -1] + list(range(areas)) * 3)
            if ctx.rng.random() < 0.7:
                scene.scramble("widget", widgets)
            if ctx.rng.random() < 0.3:
                _scramble_manager(ctx, scene)
            if area in popups and ctx.rng.random() < 0.3:
                scene.set("area", area, {"unknown_b78": ctx.rng.randint(0, 40)})
            # A popup area gives each widget the next row, so add to area looks at the last entry
            # only. The first entry can then be -1, where the search for the last column stops.
            if area in popups and held.get(area, 0) >= 2 and ctx.rng.random() < 0.2:
                scene.set("area", area, {"widget_index": [-1]})
            held[area] = held.get(area, 0) + 1
            scene.set_tick(ctx.rng.getrandbits(31))
            scene.start_free_count()
            index = scene.widget_create(name, *_widget_arguments(ctx, area, area in popups))
            scene.expect_same_frees(name)
            ctx.expect_equal(f"{name}: widget index", widgets, index, source="wanted")
            widgets += 1
            most = max(most, widgets)
            scene.expect_same(name, "manager", 0)
            scene.expect_same(name, "widget", index)
            if area != -1:
                scene.expect_same(name, "area", area)
        for index in range(most + SPARE_WIDGETS):
            scene.expect_same(f"widgets {case} at the end", "widget", index)
        for index in range(most_areas + SPARE_AREAS):
            scene.expect_same(f"widgets {case} at the end", "area", index)
    return calls


def test_manager_widget_limit(ctx):
    scene = ManagerScene(ctx)
    scene.new(0)
    scene.reset(1)
    scene.area_create("widget limit: area", *_area_arguments(ctx, owner=-1)[:5], 0, 0x1234, -1)
    for count in range(MAX_WIDGETS + 3):
        name = f"widget limit, create {count}"
        area = ctx.rng.choice([-1, 0])
        index = scene.widget_create(name, *_widget_arguments(ctx, area, False))
        ctx.expect_equal(name, count if count < MAX_WIDGETS else -1, index, source="wanted")
        if count % 150 == 0 or count >= MAX_WIDGETS - 2:
            scene.expect_same(name, "manager", 0)
            scene.expect_same(name, "area", 0)
    for index in (0, 1, MAX_WIDGETS - 1, MAX_WIDGETS):
        scene.expect_same("widget limit", "widget", index)

    # A draw order with one place left takes the widget. A full one leaves it out.
    for room, error in ((1, []), (0, [LINE_WIDGET_ORDER_FULL])):
        name = f"widget order limit, room for {room}"
        scene.reset(1)
        scene.widget_create(f"{name}: first", *_widget_arguments(ctx, -1, False))
        scene.set("manager", 0, {"widget_count": 1200 - room})
        ctx.original.error_lines.clear()
        index = scene.widget_create(name, *_widget_arguments(ctx, -1, False))
        ctx.expect_equal(f"{name}: widget index", 1, index, source="wanted")
        ctx.expect_equal(f"{name}: error of the original", error, ctx.original.error_lines, source="wanted")
        scene.expect_same(name, "manager", 0)

    # An area with 1199 widgets takes one more. An area with 1200 takes none.
    for held in (1198, 1199, 1200):
        name = f"area with {held} widgets"
        scene.reset(1)
        scene.area_create(f"{name}: area", *_area_arguments(ctx, owner=-1)[:5], 0, 0x1234, -1)
        scene.widget_create(f"{name}: first", *_widget_arguments(ctx, 0, False))
        scene.set("area", 0, {"widget_count": held, "widget_index": [0] * 1200})
        scene.widget_create(name, *_widget_arguments(ctx, 0, False))
        scene.expect_same(name, "area", 0)
    return MAX_WIDGETS + 7


def test_manager_widget_release(ctx):
    """The reset releases the pictures of its widgets. Widgets that name the same file share one image."""
    scene = ManagerScene(ctx)
    ctx.sync_framebuffer(21)
    scene.new(0)
    scene.reset(1)
    steps = 0
    with tempfile.TemporaryDirectory(dir="/tmp") as folder:
        files = [_write(folder, name, _rgn(w, h, _pixels(ctx, w, h)))
                 for name, w, h in (("one.rgn", 40, 30), ("two.rgn", 25, 25), ("three.rgn", 31, 17))]
        files.append(os.path.join(folder, "missing.rgn"))
        for round_number, release in enumerate([1, 1, 0, 1, 1, 1]):
            name = f"release round {round_number}"
            count = ctx.rng.randint(3, 9)
            for index in range(count):
                x, y = 5 + index % 4 * 197, 20 + index // 4 * 100
                picture = ctx.rng.random() < 0.7
                text = ctx.rng.choice(files).encode() if picture else b"Arsenal"
                scene.widget_create(f"{name}: widget {index}", PICTURE if picture else 0,
                                    [x, y, x + 190, y + 60], 0, 0, 0x20, 0x8410, 0xF81F, 0, 1, 0x4321, text,
                                    0, 0, 0, -1)
            for index in range(count):
                if ctx.rng.random() < 0.85:
                    scene.draw_widget(index, 1)
                    scene.expect_same_image(f"{name}: widget {index}", "widget", index, "picture")
            ctx.expect_same_framebuffer(name)
            if round_number == 4:
                scene.set("manager", 0, {"unknown_12f555": 1})
            scene.start_free_count()
            scene.reset(release)
            scene.expect_same_frees(name)
            scene.expect_same(name, "manager", 0)
            for index in range(count):
                scene.expect_same_image(f"{name}: after the reset, widget {index}", "widget", index, "picture")
                scene.expect_same_image(f"{name}: after the reset, widget {index}", "widget", index, "saved")
            steps += count
    return steps


TESTS = [
    ("manager_new", test_manager_new),
    ("manager_areas", test_manager_areas),
    ("manager_area_limit", test_manager_area_limit),
    ("manager_widgets", test_manager_widgets),
    ("manager_widget_limit", test_manager_widget_limit),
    ("manager_widget_release", test_manager_widget_release),
]
