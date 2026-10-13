"""A few areas and widgets with the same contents in the original and in the port.

Each field has a name (the member of the port struct), the offset in the struct of the
original, a struct format character and a count.
"""
import struct

AREA_SIZE, WIDGET_SIZE = 0xBF1, 0x18C
AREA_COUNT, WIDGET_COUNT = 6, 24

OFFSET_AREA_AREAS = 0x204
OFFSET_AREA_WIDGETS = 0x208
OFFSET_WIDGET_AREAS = 0x00

AREA_FIELDS = [
    ("x1", 0x000, "i", 1), ("y1", 0x004, "i", 1), ("x2", 0x008, "i", 1), ("y2", 0x00C, "i", 1),
    ("row_offset", 0x010, "i", 1), ("last_row", 0x014, "i", 1), ("flags", 0x018, "I", 1),
    ("column_x1", 0x01C, "i", 30), ("row_y1", 0x094, "i", 30),
    ("column_x2", 0x10C, "i", 30), ("row_y2", 0x184, "i", 30),
    ("scroll_to", 0x1FC, "i", 1), ("colour", 0x20C, "H", 1), ("owner_widget", 0x20E, "h", 1),
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
    ("text_flags", 0x3C, "I", 1), ("text_param", 0x44, "i", 1),
    ("highlight_colour", 0x74, "H", 1), ("text_colour", 0x76, "H", 1),
    ("second_colour", 0x78, "H", 1), ("text", 0x80, "B", 260),
    ("lighten_once", 0x184, "i", 1), ("highlighted", 0x188, "i", 1),
]

# Offsets of image pointers in the structs of the original.
IMAGES = {("widget", "saved"): 0x4C, ("widget", "picture"): 0x5C}

KINDS = {"area": (AREA_FIELDS, AREA_SIZE), "widget": (WIDGET_FIELDS, WIDGET_SIZE)}


class Scene:
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
            self.ctx.port.send("set", kind, index, name, *items)

    def expect_same_image(self, case, kind, index, which):
        """Compares one image that a struct points to: size and pixels, or none on both sides."""
        original = self.ctx.original
        pointer = struct.unpack("<I", original.read(self.addr(kind, index) + IMAGES[kind, which], 4))[0]
        expected = None
        if pointer:
            width, height, size, pixels = struct.unpack("<4I", original.read(pointer, 16))
            expected = ([width, height, size], original.read(pixels, size))
        self.ctx.port.send("image", kind, index, which)
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
        self.ctx.port.send("get", kind, index)
        port_values = [int(v) for v in self.ctx.port.reply()]
        position = 0
        for name, offset, fmt, count in fields:
            original_values = list(struct.unpack_from(f"<{count}{fmt}", raw, offset))
            self.ctx.expect_equal(f"{case}: {kind} {index} {name}", original_values,
                                  port_values[position:position + count])
            position += count
