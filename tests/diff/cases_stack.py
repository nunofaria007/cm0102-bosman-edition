"""Screen stack: push, remove, free all, slots, title and the small queries, for one local player."""
import struct

from unicorn import UC_HOOK_CODE

STACK = 0xB74450                 # the screen stack object of the original
OFFSET_RECORD = 0x7D4            # channel 0, slot 0
OFFSET_DEFAULTS = 0x37D4
OFFSET_RECORD_COUNT = 0x380A     # s16 for each of 16 channels
OFFSET_CHANNEL = 0x382A
RECORD_SIZE = 0x30
NODE_SLOTS = 60

FUN_PUSH = 0x7FE600              # thiscall (build, input, flag, leave, exit)
FUN_REMOVE = 0x7FF990            # thiscall (key, channel)
FUN_FREE_ALL = 0x7FEF10
FUN_SET_DEFAULT = 0x7FEB20       # thiscall (build, input, flag, leave, exit, values, count)
FUN_SET_SLOT = 0x7FF270          # thiscall (slot, value, copy)
FUN_SLOT = 0x7FF010              # thiscall (slot)
FUN_SET_TITLE = 0x7FEEB0
FUN_TITLE = 0x7FEE80            # replaced in the emulator by the screen draw tests
FUN_AT_END = 0x7FEBB0
FUN_CANNOT_GO_BACK = 0x7FEC60    # thiscall (any)
FUN_FLAGGED = 0x803D50
FUN_SNAPSHOT_FLAG = 0x55F290     # stdcall (channel): clears a flag of a network snapshot pool

# Fake screen functions: 1 to 8 for build, leave and exit, 101 to 108 for input.
FAKE_BASE = 0x62000000
FAKES = 8


class Stack:
    """The screen stack of the original and of the port, with the same fake screen functions."""

    def __init__(self, ctx):
        self.ctx = ctx
        original = ctx.original
        original.reset_heap()
        if not getattr(original, "stack_ready", False):
            original.uc.mem_map(FAKE_BASE, 0x1000)
            for number in list(range(1, FAKES + 1)) + list(range(101, 101 + FAKES)):
                # Each fake returns 0, so an input function gives 0.
                original.write(FAKE_BASE + number * 8, b"\x31\xc0\xc3")
            original.write(FUN_SNAPSHOT_FLAG, b"\xc2\x04\x00")
            original.uc.hook_add(UC_HOOK_CODE, self._on_fake, begin=FAKE_BASE, end=FAKE_BASE + 0xFFF)
            original.stack_ready = True
        Stack.CALLS.clear()
        original.write(STACK + OFFSET_RECORD, bytes(RECORD_SIZE))
        original.write(STACK + OFFSET_DEFAULTS, bytes(0x1A))
        original.write(STACK + OFFSET_RECORD_COUNT, struct.pack("<h", 1))
        original.write(STACK + OFFSET_CHANNEL, struct.pack("<h", 0))
        record = STACK + OFFSET_RECORD
        original.write(record + 0x16, struct.pack("<i", 1))
        original.write(record + 0x22, struct.pack("<h", -1))
        ctx.port.send("stack_init")

    # The fake screen functions that the original called, in order.
    CALLS = []

    @staticmethod
    def _on_fake(uc, address, size, user_data):
        if (address - FAKE_BASE) % 8 == 0:
            Stack.CALLS.append((address - FAKE_BASE) // 8)

    def address(self, number):
        return FAKE_BASE + number * 8 if number else 0

    def call(self, function, *args):
        return self.ctx.original.call(function, *args, this=STACK)

    def push(self, case, build, input_, flag, leave, exit_):
        result = self.call(FUN_PUSH, *(self.address(n) for n in (build, input_)), flag,
                           *(self.address(n) for n in (leave, exit_)))
        self.ctx.port.send("stack_push", build, input_, flag, leave, exit_)
        self.ctx.expect_equal(f"{case}: push result", int(result != 0), int(self.ctx.port.reply()[0]))

    def set_default(self, build, input_, flag, leave, exit_, values):
        original = self.ctx.original
        table = original.alloc_bytes(struct.pack(f"<{len(values)}I", *values) + b"\0" * 4)
        self.call(FUN_SET_DEFAULT, *(self.address(n) for n in (build, input_)), flag,
                  *(self.address(n) for n in (leave, exit_)), table, len(values))
        self.ctx.port.send("stack_default", build, input_, flag, leave, exit_, len(values), *values)

    def set_record(self, key, rebuild, away, done):
        """Sets fields of the record that only the main loop and other screens change."""
        record = STACK + OFFSET_RECORD
        self.ctx.original.write(record + 0x02, struct.pack("<I", 0x12345678 if key else 0))
        self.ctx.original.write(record + 0x16, struct.pack("<3i", rebuild, away, done))
        self.ctx.port.send("stack_record", int(key), rebuild, away, done)

    def remove(self):
        self.call(FUN_REMOVE, 0, 0)
        self.ctx.port.send("stack_remove")

    def free_all(self):
        self.call(FUN_FREE_ALL)
        self.ctx.port.send("stack_free")

    def set_slot(self, slot, value, copy):
        """`value` is bytes or None with `copy`, else a number."""
        if copy and value is None:
            self.call(FUN_SET_SLOT, slot, 0, 1)
            self.ctx.port.send("stack_slot_set", slot, 1, "null")
        elif copy:
            self.call(FUN_SET_SLOT, slot, self.ctx.original.alloc_bytes(value + b"\0"), 1)
            self.ctx.port.send("stack_slot_set", slot, 1, "-" + value.hex())
        else:
            self.call(FUN_SET_SLOT, slot, value, 0)
            self.ctx.port.send("stack_slot_set", slot, 0, value)

    def set_title(self, title):
        pointer = self.ctx.original.alloc_bytes(title + b"\0") if title is not None else 0
        self.call(FUN_SET_TITLE, pointer)
        self.ctx.port.send("stack_title_set", "null" if title is None else "-" + title.hex())

    def expect_same(self, case):
        ctx, original = self.ctx, self.ctx.original
        record = STACK + OFFSET_RECORD
        player, key, first, last, current, flagged, count, rebuild, away, done, f22, f24 = struct.unpack(
            "<hIIIIhhiiihi", original.read(record, 0x28))
        nodes = []
        node = first
        while node and len(nodes) < 200:
            nodes.append(node)
            node = struct.unpack("<I", original.read(node + 0x1F8, 4))[0]
        position = lambda pointer: nodes.index(pointer) if pointer in nodes else (-1 if not pointer else -2)
        record_count = struct.unpack("<h", original.read(STACK + OFFSET_RECORD_COUNT, 2))[0]
        wanted = [player, int(key != 0), flagged, count, rebuild, away, done, f22, f24, record_count,
                  position(first), position(last), position(current), len(nodes)]
        ctx.port.send("stack_dump")
        got = [int(word) for word in ctx.port.reply()]
        ctx.expect_equal(f"{case}: record", wanted, got)
        fake = lambda pointer: (pointer - FAKE_BASE) // 8 if pointer else 0
        for index, node in enumerate(nodes):
            build, leave, input_, exit_, flag = struct.unpack("<5I", original.read(node, 20))
            slots = struct.unpack(f"<{NODE_SLOTS * 2}I", original.read(node + 0x14, NODE_SLOTS * 8))
            prev = struct.unpack("<I", original.read(node + 0x1F4, 4))[0]
            title = original.read(node + 0x1FC, 260).split(b"\0")[0]
            words = [str(fake(build)), str(fake(leave)), str(fake(input_)), str(fake(exit_)), str(flag),
                     str(position(prev)), "-" + title.hex()]
            for slot in range(NODE_SLOTS):
                value, owned = slots[slot * 2], slots[slot * 2 + 1]
                if owned:
                    words.append("s-" + original.read(value, 300).split(b"\0")[0].hex())
                else:
                    words.append(str(value))
            ctx.expect_equal(f"{case}: screen {index}", words, [w.decode() for w in ctx.port.reply()])
        ctx.port.send("stack_calls")
        calls = [int(word) for word in ctx.port.reply()][1:]
        ctx.expect_equal(f"{case}: calls of screen functions", Stack.CALLS, calls)
        Stack.CALLS.clear()

    def expect_same_reads(self, case):
        """Title, slots and queries. The original ends the program for these with no screen."""
        ctx = self.ctx
        # The screen draw tests answer FUN_TITLE themselves, so the test reads the title of the node.
        current = struct.unpack("<I", ctx.original.read(STACK + OFFSET_RECORD + 0x0E, 4))[0]
        title = ctx.original.read(current + 0x1FC, 260).split(b"\0")[0]
        ctx.port.send("stack_title")
        ctx.expect_equal(f"{case}: title", "-" + title.hex(), ctx.port.reply()[0].decode())
        for slot in (0, 1, 59, ctx.rng.randint(0, 59), -1, 60):
            value = self.call(FUN_SLOT, slot) & 0xFFFFFFFF
            ctx.port.send("stack_slot", slot)
            port_value = int(ctx.port.reply()[0])
            if value and port_value and value != port_value:
                # An owned text sits at a different address in each program. stack_dump compares it.
                continue
            ctx.expect_equal(f"{case}: slot {slot}", value, port_value)
        wanted = [self.call(FUN_AT_END) & 0xFF, self.call(FUN_CANNOT_GO_BACK, 0) & 0xFF,
                  self.call(FUN_CANNOT_GO_BACK, 1) & 0xFF, self.call(FUN_FLAGGED)]
        ctx.port.send("stack_queries")
        ctx.expect_equal(f"{case}: queries", wanted, [int(word) for word in ctx.port.reply()])


def _function(ctx, low, chance_none):
    return 0 if ctx.rng.random() < chance_none else low + ctx.rng.randint(0, FAKES - 1)


def _has_screen(ctx):
    return struct.unpack("<I", ctx.original.read(STACK + OFFSET_RECORD + 0x0E, 4))[0] != 0


def _text(ctx):
    return bytes(ctx.rng.choice(b"abcXYZ019 \\./") for _ in range(ctx.rng.choice([0, 1, 5, 40, 120])))


def test_stack(ctx):
    calls = 0
    for case in range(80):
        stack = Stack(ctx)
        if ctx.rng.random() < 0.5:
            stack.set_default(_function(ctx, 1, 0.1), _function(ctx, 101, 0.1), ctx.rng.choice([0, 0, 1]),
                              _function(ctx, 1, 0.5), _function(ctx, 1, 0.5),
                              [ctx.rng.getrandbits(32) for _ in range(ctx.rng.randint(0, 6))])
        for step in range(ctx.rng.randint(1, 30)):
            name = f"stack {case} step {step}"
            calls += 1
            roll = ctx.rng.random()
            if roll < 0.5 or not _has_screen(ctx):
                if roll < 0.04:
                    stack.free_all()
                elif roll < 0.08:
                    stack.remove()
                else:
                    if ctx.rng.random() < 0.3:
                        stack.set_record(ctx.rng.random() < 0.3, *(ctx.rng.choice([0, 1, 7]) for _ in range(3)))
                    flag = ctx.rng.choice([0, 0, 0, 1, ctx.rng.getrandbits(32)])
                    stack.push(name, _function(ctx, 1, 0.05), _function(ctx, 101, 0.05), flag,
                               _function(ctx, 1, 0.4), _function(ctx, 1, 0.5))
            elif roll < 0.75:
                slot = ctx.rng.choice([0, 1, 59, -1, 60, ctx.rng.randint(0, 59)])
                if ctx.rng.random() < 0.5:
                    stack.set_slot(slot, ctx.rng.choice([None, _text(ctx), _text(ctx)]), True)
                else:
                    stack.set_slot(slot, ctx.rng.choice([0, ctx.rng.getrandbits(32)]), False)
            elif roll < 0.85:
                stack.set_title(ctx.rng.choice([None, _text(ctx), b"\\"]))
            elif roll < 0.92:
                stack.remove()
            else:
                stack.free_all()
            stack.expect_same(name)
            if _has_screen(ctx):
                stack.expect_same_reads(name)
    return calls


def test_stack_limit(ctx):
    """A full stack drops its first screen, unless that screen has the flag."""
    for first_flag in (0, 1):
        stack = Stack(ctx)
        for number in range(104):
            name = f"stack limit, flag {first_flag}, push {number}"
            stack.push(name, 1 + number % FAKES, 101 + number % FAKES, first_flag if number == 0 else 0,
                       1 + (number + 3) % FAKES, 0)
            if number % 10 == 0 or number > 98:
                stack.expect_same(name)
    return 208


TESTS = [
    ("stack", test_stack),
    ("stack_limit", test_stack_limit),
]
