"""A few areas and widgets with the same contents in the original and in the port.

Each field has a name (the member of the port struct), the offset in the struct of the
original, a struct format character and a count.
"""
import struct

from original import FB_SIZE

AREA_SIZE, WIDGET_SIZE = 0xBF1, 0x18C
AREA_COUNT, WIDGET_COUNT = 6, 24

OFFSET_AREA_AREAS = 0x204
OFFSET_AREA_WIDGETS = 0x208
OFFSET_WIDGET_AREAS = 0x00

AREA_FIELDS = [
    ("x1", 0x000, "i", 1), ("y1", 0x004, "i", 1), ("x2", 0x008, "i", 1), ("y2", 0x00C, "i", 1),
    ("first_row", 0x010, "i", 1), ("last_row", 0x014, "i", 1), ("flags", 0x018, "I", 1),
    ("column_x1", 0x01C, "i", 30), ("row_y1", 0x094, "i", 30),
    ("column_x2", 0x10C, "i", 30), ("row_y2", 0x184, "i", 30),
    ("scroll_to", 0x1FC, "i", 1), ("colour", 0x20C, "H", 1), ("owner_widget", 0x20E, "h", 1),
    ("unknown_210", 0x210, "h", 1), ("unknown_b78", 0xB78, "h", 1),
    ("widget_index", 0x212, "h", 1200), ("index", 0xB72, "h", 1), ("widget_count", 0xB74, "h", 1),
    ("first_visible", 0xB76, "h", 1),
    ("scrollbar.x1", 0xB7A, "i", 1), ("scrollbar.y1", 0xB7E, "i", 1),
    ("scrollbar.x2", 0xB82, "i", 1), ("scrollbar.y2", 0xB86, "i", 1),
    ("scrollbar.track_y1", 0xB8A, "i", 1), ("scrollbar.track_y2", 0xB8E, "i", 1),
    ("scrollbar.max_position", 0xB92, "i", 1), ("scrollbar.position", 0xB96, "i", 1),
    ("scrollbar.thumb_height", 0xB9A, "i", 1), ("scrollbar.thumb_y1", 0xB9E, "i", 1),
    ("scrollbar.thumb_y2", 0xBA2, "i", 1), ("scrollbar.pressed", 0xBAA, "B", 1),
    ("column_count", 0xBAB, "B", 1), ("row_count", 0xBAC, "B", 1),
    ("column_weight", 0xBAD, "B", 30), ("row_weight", 0xBCB, "B", 30),
    ("shown", 0xBE9, "i", 1), ("has_scrollbar", 0xBED, "i", 1),
]

WIDGET_FIELDS = [
    ("flags", 0x0C, "I", 1),
    ("x1", 0x10, "i", 1), ("y1", 0x14, "i", 1), ("x2", 0x18, "i", 1), ("y2", 0x1C, "i", 1),
    ("column", 0x20, "i", 1), ("row", 0x24, "i", 1),
    ("fixed_x1", 0x28, "i", 1), ("fixed_x2", 0x2C, "i", 1),
    ("fixed_y1", 0x30, "i", 1), ("fixed_y2", 0x34, "i", 1),
    ("style", 0x38, "I", 1), ("colour", 0x72, "H", 1),
    ("area", 0x7A, "h", 1), ("popup_area", 0x7C, "h", 1),
    ("text_flags", 0x3C, "I", 1), ("cursor", 0x44, "i", 1),
    ("highlight_colour", 0x74, "H", 1), ("font", 0x76, "h", 1),
    ("text_colour", 0x78, "H", 1), ("text", 0x80, "B", 256),
    ("lighten_once", 0x184, "i", 1), ("highlighted", 0x188, "i", 1),
    ("unknown_08", 0x08, "i", 1), ("unknown_40", 0x40, "i", 1), ("unknown_48", 0x48, "I", 1),
    ("tick", 0x50, "i", 1), ("index", 0x70, "h", 1), ("unknown_7e", 0x7E, "h", 1),
    ("unknown_180", 0x180, "i", 1),
]

OFFSET_WIDGET_WIDGETS = 0x04
OFFSET_WIDGET_TEXT_SOURCE = 0x54

# The screen manager without its areas and widgets. Three pointers are not listed: the two images
# in IMAGES and one at 0x12F529.
MANAGER_SIZE = 0x12F661
OFFSET_MANAGER_AREAS, OFFSET_MANAGER_WIDGETS = 0x4, 0xBA95E
OFFSET_MANAGER_POINTER = 0x12F529
MANAGER_FIELDS = [
    ("mode", 0x0, "i", 1),
    ("area_slots", 0x12E99E, "h", 1), ("widget_slots", 0x12E9A0, "h", 1),
    ("area_order", 0x12E9A2, "h", 250), ("widget_order", 0x12EB96, "h", 1200),
    ("area_count", 0x12F4F6, "h", 1), ("widget_count", 0x12F4F8, "h", 1),
    ("darken_screen", 0x12F4FA, "i", 1),
    ("unknown_12f4fe", 0x12F4FE, "B", 1), ("unknown_12f4ff", 0x12F4FF, "h", 1),
    ("unknown_12f501", 0x12F501, "h", 1), ("unknown_12f503", 0x12F503, "h", 1),
    ("unknown_12f505", 0x12F505, "i", 1), ("unknown_12f509", 0x12F509, "i", 6),
    ("unknown_12f52d", 0x12F52D, "h", 1), ("unknown_12f52f", 0x12F52F, "h", 1),
    ("unknown_12f531", 0x12F531, "h", 1), ("unknown_12f533", 0x12F533, "h", 1),
    ("fade_in", 0x12F535, "i", 1),
    ("unknown_12f539", 0x12F539, "h", 1), ("unknown_12f53b", 0x12F53B, "h", 1),
    ("skip_draw", 0x12F53D, "i", 1), ("unknown_12f541", 0x12F541, "I", 1),
    ("laid_out", 0x12F545, "i", 1),
    ("widget_overflow", 0x12F549, "i", 1), ("area_overflow", 0x12F54D, "i", 1),
    ("unknown_12f551", 0x12F551, "i", 1), ("unknown_12f555", 0x12F555, "i", 1),
    ("background_name", 0x12F55D, "B", 260),
]

# Offsets of image pointers in the structs of the original.
IMAGES = {("widget", "saved"): 0x4C, ("widget", "picture"): 0x5C,
          ("area", "saved"): 0x200, ("area", "scrollbar"): 0xBA6,
          ("manager", "unknown_12f521"): 0x12F521, ("manager", "background"): 0x12F525}

# The buffer for network games that the reset and area create of the original write to.
G_NET_BUFFER, G_NET_POSITION, G_NET_CAPACITY = 0xAE2908, 0xAE290C, 0xAE2910
NET_CAPACITY = 0x1000

FUN_MANAGER_NEW = 0x55EF50       # thiscall (argument)
FUN_MANAGER_RESET = 0x55F7C0     # thiscall (release)
FUN_AREA_CREATE = 0x55FBA0       # thiscall, 11 arguments
FUN_WIDGET_CREATE = 0x55F990     # thiscall, 18 arguments
FUN_WIDGET_DRAW = 0x5EE590       # thiscall (redraw)
G_TICK = 0xB67A08                # widget init copies it into each widget
FUN_SCREEN_DRAW = 0x55F300       # thiscall (x1, y1, x2, y2)
FUN_FADE = 0x5E30E0              # (x1, y1, x2, y2, steps)
G_NO_BACKGROUNDS = 0xDD6BDA      # not 0: every background name becomes a backslash
G_DARKEN_SCREENS = 0xB77C8C      # not 0: a screen manager with darken_screen set darkens the screen

RANGES = {"B": (0, 0xFF), "h": (-0x8000, 0x7FFF), "H": (0, 0xFFFF),
          "i": (-0x80000000, 0x7FFFFFFF), "I": (0, 0xFFFFFFFF)}

FUN_CAPTURE = 0x5E3DD0

KINDS = {"area": (AREA_FIELDS, AREA_SIZE), "widget": (WIDGET_FIELDS, WIDGET_SIZE),
         "manager": (MANAGER_FIELDS, MANAGER_SIZE)}


class Scene:
    # The names of the port driver for the structs of this scene.
    PORT_KIND = {"area": "area", "widget": "widget"}

    def __init__(self, ctx):
        self.ctx = ctx
        original = ctx.original
        original.reset_heap()
        self.base = {"area": original.alloc(AREA_COUNT * AREA_SIZE),
                     "widget": original.alloc(WIDGET_COUNT * WIDGET_SIZE)}
        for i in range(AREA_COUNT):
            original.write_u32(self.addr("area", i) + OFFSET_AREA_AREAS, self.base["area"])
            original.write_u32(self.addr("area", i) + OFFSET_AREA_WIDGETS, self.base["widget"])
        for i in range(WIDGET_COUNT):
            original.write_u32(self.addr("widget", i) + OFFSET_WIDGET_AREAS, self.base["area"])
        ctx.port.send("scene_reset")

    @staticmethod
    def check_tables(ctx):
        """The port driver must list the same fields in the same order."""
        for kind, (fields, _) in KINDS.items():
            ctx.port.send("fields", kind)
            ctx.expect_equal(f"{kind} fields", [f"{name}:{count}" for name, _, _, count in fields],
                             [word.decode() for word in ctx.port.reply()], source="scene.py")

    def addr(self, kind, index):
        return self.base[kind] + index * KINDS[kind][1]

    def set(self, kind, index, values):
        """Writes fields in both programs. A list sets an array from its start."""
        table = {name: (offset, fmt) for name, offset, fmt, _ in KINDS[kind][0]}
        for name, value in values.items():
            offset, fmt = table[name]
            items = list(value) if isinstance(value, (list, tuple)) else [value]
            if not items:
                continue
            self.ctx.original.write(self.addr(kind, index) + offset, struct.pack(f"<{len(items)}{fmt}", *items))
            self.ctx.port.send("set", self.PORT_KIND[kind], index, name, *items)

    def capture_to(self, kind, index, which, box):
        """Points an image pointer of a struct at a new capture of the screen, in both programs."""
        original = self.ctx.original
        original.write_u32(self.addr(kind, index) + IMAGES[kind, which], original.call(FUN_CAPTURE, *box, 0))
        self.ctx.port.send("capture_to", self.PORT_KIND[kind], index, which, *box)

    def expect_same_image(self, case, kind, index, which):
        """Compares one image that a struct points to: size and pixels, or none on both sides."""
        original = self.ctx.original
        pointer = struct.unpack("<I", original.read(self.addr(kind, index) + IMAGES[kind, which], 4))[0]
        expected = None
        if pointer:
            width, height, size, pixels = struct.unpack("<4I", original.read(pointer, 16))
            expected = ([width, height, size], original.read(pixels, size))
        self.ctx.port.send("image", self.PORT_KIND[kind], index, which)
        words = self.ctx.port.reply()
        got = None
        if words != [b"0"]:
            fields = [int(v) for v in words]
            got = (fields[:3], self.ctx.port.read(fields[2]))
        self.ctx.expect_equal(f"{case}: {kind} {index} {which} image", expected, got)

    def expect_same(self, case, kind, index):
        """Compares every field of one struct."""
        fields, size = KINDS[kind]
        raw = self.ctx.original.read(self.addr(kind, index), size)
        self.ctx.port.send("get", self.PORT_KIND[kind], index)
        port_values = [int(v) for v in self.ctx.port.reply()]
        position = 0
        for name, offset, fmt, count in fields:
            original_values = list(struct.unpack_from(f"<{count}{fmt}", raw, offset))
            self.ctx.expect_equal(f"{case}: {kind} {index} {name}", original_values,
                                  port_values[position:position + count])
            position += count


class ManagerScene(Scene):
    """One screen manager with its 250 areas and 1200 widgets, in both programs."""

    PORT_KIND = {"area": "manager_area", "widget": "manager_widget", "manager": "manager"}

    def __init__(self, ctx):
        self.ctx = ctx
        original = ctx.original
        original.reset_heap()
        self.manager = original.alloc(MANAGER_SIZE)
        self.base = {"area": self.manager + OFFSET_MANAGER_AREAS,
                     "widget": self.manager + OFFSET_MANAGER_WIDGETS, "manager": self.manager}
        original.write_u32(G_NET_BUFFER, original.alloc(NET_CAPACITY))
        original.write_u32(G_NET_CAPACITY, NET_CAPACITY)
        ctx.port.send("manager_clear")

    def scramble(self, kind, index, names=None):
        """Sets fields to random values in both programs: all of them, or the named ones."""
        rng = self.ctx.rng
        self.set(kind, index, {name: [rng.randint(*RANGES[fmt]) for _ in range(count)]
                               for name, _, fmt, count in KINDS[kind][0] if names is None or name in names})

    def new(self, argument):
        self.ctx.original.call(FUN_MANAGER_NEW, argument, this=self.manager)
        self.ctx.port.send("manager_new", argument)

    def reset(self, release, net_room=True):
        self._rewind_net(net_room)
        self.ctx.original.call(FUN_MANAGER_RESET, release, this=self.manager)
        self.ctx.port.send("manager_reset", release)

    def _rewind_net(self, room):
        """Empties the network buffer. Without room, each write of the original shows its error box."""
        self.ctx.original.write_u32(G_NET_POSITION, 0)
        self.ctx.original.write_u32(G_NET_CAPACITY, NET_CAPACITY if room else 0)

    def start_free_count(self):
        self.ctx.original.frees = 0
        self.ctx.port.send("frees")
        self.ctx.port.reply()

    def expect_same_frees(self, case):
        """Both programs freed the same number of images. The original frees two blocks for each."""
        self.ctx.port.send("frees")
        self.ctx.expect_equal(f"{case}: images freed", self.ctx.original.frees / 2,
                              int(self.ctx.port.reply()[0]))

    def expect_links(self, case, index):
        """The area points to the areas and widgets of its screen manager."""
        original = self.ctx.original
        area = self.addr("area", index)
        links = struct.unpack("<2I", original.read(area + OFFSET_AREA_AREAS, 8))
        self.ctx.port.send("manager_links", index)
        self.ctx.expect_equal(f"{case}: area {index} links",
                              [int(links[0] == self.base["area"]), int(links[1] == self.base["widget"])],
                              [int(v) for v in self.ctx.port.reply()])

    def set_pointer(self):
        """Sets the pointer at 0x12F529, whose purpose is not known."""
        self.ctx.original.write_u32(self.manager + OFFSET_MANAGER_POINTER, 0x12345678)
        self.ctx.port.send("manager_pointer", 1)
        self.ctx.port.reply()

    def expect_same_pointer(self, case):
        value = struct.unpack("<I", self.ctx.original.read(self.manager + OFFSET_MANAGER_POINTER, 4))[0]
        self.ctx.port.send("manager_pointer", 0)
        self.ctx.expect_equal(f"{case}: pointer at 0x12f529 is set", int(value != 0),
                              int(self.ctx.port.reply()[0]))

    def area_create(self, case, box, columns, column_weights, rows, row_weights, flags, colour, owner,
                    net_room=True):
        """Creates an area in both programs and returns its index. A weight table is a list or None."""
        original = self.ctx.original
        self._rewind_net(net_room)
        tables = [original.alloc_bytes(bytes(table) + b"\0") if table is not None else 0
                  for table in (column_weights, row_weights)]
        result = original.call(FUN_AREA_CREATE, *box, columns, tables[0], rows, tables[1], flags, colour,
                               owner, this=self.manager)
        # The original returns the index in AX and leaves the rest of EAX as it was.
        result = (result & 0xFFFF) - (0x10000 if result & 0x8000 else 0)
        words = [*box, columns, rows, flags, colour, owner]
        for table in (column_weights, row_weights):
            words += [-1] if table is None else [len(table), *table]
        self.ctx.port.send("manager_area", *words)
        self.ctx.expect_equal(f"{case}: area index", result, int(self.ctx.port.reply()[0]))
        return result

    def set_tick(self, tick):
        self.ctx.original.write_u32(G_TICK, tick)
        self.ctx.port.send("tick", tick)

    def widget_create(self, case, flags, box, column, row, style, colour, highlight, text_flags, font,
                      text_colour, text, unknown_40, unknown_08, unknown_48, area):
        """Creates a widget in both programs and returns its index. The text is bytes or None."""
        original = self.ctx.original
        pointer = original.alloc_bytes(text + b"\0") if text is not None else 0
        result = original.call(FUN_WIDGET_CREATE, flags, *box, column, row, style, colour, highlight,
                               text_flags, font, text_colour, pointer, unknown_40, unknown_08, unknown_48,
                               area, this=self.manager)
        result = (result & 0xFFFF) - (0x10000 if result & 0x8000 else 0)
        self.ctx.port.send("manager_widget", flags, *box, column, row, style, colour, highlight, text_flags,
                           font, text_colour, unknown_40, unknown_08, unknown_48, area,
                           "null" if text is None else "-" + text.hex())
        self.ctx.expect_equal(f"{case}: widget index", result, int(self.ctx.port.reply()[0]))
        if result != -1:
            widget = self.addr("widget", result)
            links = struct.unpack("<2I", original.read(widget, 8))
            source = struct.unpack("<I", original.read(widget + OFFSET_WIDGET_TEXT_SOURCE, 4))[0]
            self.ctx.expect_equal(f"{case}: text source of the original", True, source in (0, pointer),
                                  source="wanted")
            self.ctx.port.send("manager_wlinks", result)
            self.ctx.expect_equal(f"{case}: widget {result} links and text source",
                                  [int(links[0] == self.base["area"]), int(links[1] == self.base["widget"]),
                                   int(source != 0)], [int(v) for v in self.ctx.port.reply()])
        return result

    def draw_widget(self, index, redraw):
        self.ctx.original.call(FUN_WIDGET_DRAW, redraw, this=self.addr("widget", index))
        self.ctx.port.send("manager_draw", index, redraw)

    def area_count(self):
        """The areas in use, as the original counts them."""
        return struct.unpack("<h", self.ctx.original.read(self.manager + 0x12E99E, 2))[0]

    def hide_data_files(self, hidden, empty_folder):
        """With `hidden`, DEFAULT_PIC.RGN does not load in either program."""
        self.ctx.original.no_data_files = hidden
        self.ctx.port.send("data_dir", empty_folder if hidden else self.ctx.data_dir)

    def set_draw_globals(self, no_backgrounds, darken_screens):
        self.ctx.original.write_u32(G_NO_BACKGROUNDS, no_backgrounds)
        self.ctx.original.write_u32(G_DARKEN_SCREENS, darken_screens)
        self.ctx.port.send("draw_globals", no_backgrounds, darken_screens)

    def draw_screen(self, name, box=(-1, -1, -1, -1)):
        """Draws the screen in both programs. `name` is the background file name, as bytes."""
        original = self.ctx.original
        original.title = original.alloc_bytes(name + b"\0")
        original.flips = []
        original.call(FUN_SCREEN_DRAW, *box, this=self.manager)
        self.ctx.port.send("manager_screen", *box, "-" + name.hex())

    def expect_same_flips(self, case):
        """The same boxes flipped in the same order, each with the same screen at the time."""
        port = self.ctx.port
        port.send("flips")
        flips = []
        for _ in range(int(port.reply()[0])):
            words = [int(word) for word in port.reply()]
            flips.append((words[:4], port.read(FB_SIZE) if words[4] else None))
        wanted = self.ctx.original.flips
        self.ctx.expect_equal(f"{case}: flip boxes", [box for box, _ in wanted], [box for box, _ in flips])
        for number, ((_, a), (_, b)) in enumerate(zip(wanted, flips)):
            if a != b:
                pixels = sum(a[i:i + 2] != b[i:i + 2] for i in range(0, FB_SIZE, 2))
                self.ctx.expect_equal(f"{case}: pixels that differ at flip {number}", 0, pixels, source="wanted")
        self.ctx.original.flips = []
