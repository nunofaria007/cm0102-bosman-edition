"""Screen manager: the constructor, the reset and area create."""
from scene import KINDS, ManagerScene

SPARE_AREAS = 3                  # areas past those in use that the reset test also fills
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


TESTS = [
    ("manager_new", test_manager_new),
    ("manager_areas", test_manager_areas),
    ("manager_area_limit", test_manager_area_limit),
]
