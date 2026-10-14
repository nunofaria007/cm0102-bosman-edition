"""The progress and message box FUN_005e76f0, and the local part of opcode 7 (FUN_00802f80) that draws it."""
import struct

from cases_helpers import _hex, short_text
from original import FB_SIZE
from cases_stack import OFFSET_CHANNEL, OFFSET_RECORD, OFFSET_RECORD_COUNT, STACK

FUN_PROGRESS_BOX = 0x5E76F0      # cdecl (text, step, total), returns 1, or 0 when it draws nothing
FUN_OPCODE_7 = 0x802F80          # thiscall on the screen stack (text, step, total, s16 channel)
FUN_PUMP = 0x68D520              # the message pump: the test counts its calls

OFFSET_MANAGER_POINTER = 0x7FC   # record +0x28: the screen manager of channel 0
OFFSET_MANAGER_0 = 0x3844        # the screen manager of the local player, inside the stack object
OFFSET_BUFFER = 0x37FE           # the packet buffer {pointer, position, capacity}
OFFSET_SESSION = 0x3840          # the network session, 0 in a local game
CHANNEL_STRIDE = 0x300
BUFFER_SIZE = 0xC350             # what FUN_007fc380 allocates
OFFSET_DARKEN, OFFSET_12F551 = 0x12F4FA, 0x12F551
G_DARKEN_SCREENS = 0xB77C8C      # 0 in the file and never written: opcode 7 then leaves the cursor alone

TEXTS = [b"Loading database", b"Processing, Please Wait ...", b"Saving Game", b""]


class Progress:
    """The screen stack of the original with its own screen manager 0, and the port's stack."""

    def __init__(self, ctx):
        self.ctx = ctx
        original = ctx.original
        original.reset_heap()
        if not getattr(original, "progress_ready", False):
            original.progress_ready = True
            original.pumps = 0

            def pump():
                original.pumps += 1
                return 0
            original._replace(FUN_PUMP, 0, pump)
        original.pumps = 0
        self.manager = STACK + OFFSET_MANAGER_0
        original.write(STACK + OFFSET_RECORD, bytes(0x30))
        original.write_u32(STACK + OFFSET_MANAGER_POINTER, self.manager)
        # The records and record counts of the 15 other channels are 0 in a local game.
        for channel in range(1, 16):
            original.write(STACK + OFFSET_RECORD + channel * CHANNEL_STRIDE, bytes(0x30))
        original.write(STACK + OFFSET_RECORD_COUNT, bytes(32))
        original.write(STACK + OFFSET_CHANNEL, b"\0\0")
        original.write_u32(STACK + OFFSET_SESSION, 0)
        original.write(STACK + OFFSET_BUFFER, struct.pack("<3I", original.alloc(BUFFER_SIZE), 0, BUFFER_SIZE))
        original.write_u32(G_DARKEN_SCREENS, 0)
        ctx.port.send("st_init")

    def set_state(self, records, darken, unknown_551):
        original = self.ctx.original
        original.write(STACK + OFFSET_RECORD_COUNT, struct.pack("<h", records))
        original.write_u32(self.manager + OFFSET_DARKEN, darken)
        original.write_u32(self.manager + OFFSET_12F551, unknown_551)
        self.ctx.port.send("pg_state", records, darken, unknown_551)

    def expect_same_state(self, case):
        original = self.ctx.original
        wanted = [original.pumps,
                  struct.unpack("<i", original.read(self.manager + OFFSET_DARKEN, 4))[0],
                  struct.unpack("<i", original.read(self.manager + OFFSET_12F551, 4))[0]]
        self.ctx.port.send("pg_dump")
        self.ctx.expect_equal(f"{case}: pumps, darken flag, +0x12f551", wanted,
                              [int(word) for word in self.ctx.port.reply()])
        original.pumps = 0


def _text(ctx, allow_null):
    roll = ctx.rng.random()
    if allow_null and roll < 0.08:
        return None
    if roll < 0.4:
        return ctx.rng.choice(TEXTS)
    if roll < 0.55:
        # Too wide for the narrow box, so the text function cuts it.
        return b" ".join(short_text(ctx) for _ in range(ctx.rng.randint(4, 9)))[:200]
    if roll < 0.62:
        return short_text(ctx) + b"\n" + short_text(ctx)
    return short_text(ctx)


def _step_total(ctx):
    """Mostly the steps of the database load, sometimes edges and values that overflow the bar."""
    roll = ctx.rng.random()
    if roll < 0.5:
        total = 46
        step = ctx.rng.choice([0, 0, 1, 2, 23, 24, 45, 46, ctx.rng.randint(0, 46)])
    elif roll < 0.8:
        total = ctx.rng.choice([0, -1, 1, 2, 24, ctx.rng.randint(1, 1000)])
        step = ctx.rng.choice([-1, 0, total, total + 1, ctx.rng.randint(-5, max(total, 0) + 5)])
    else:
        total = ctx.rng.choice([ctx.rng.randint(1, 0x7FFFFFFF), 0x7FFFFFFF, -0x80000000])
        step = ctx.rng.choice([total, ctx.rng.randint(-0x80000000, 0x7FFFFFFF), 0x7FFFFFFF, 4329604, 4329605])
    return step, total


def test_box(ctx):
    for case in range(120):
        progress = Progress(ctx)
        ctx.sync_framebuffer(case)
        original = ctx.original
        progress.set_state(1, ctx.rng.choice([0, 0, 1]), ctx.rng.choice([0, 1]))
        text = _text(ctx, allow_null=True)
        step, total = _step_total(ctx)
        original.flips = []
        result = original.call(FUN_PROGRESS_BOX, original.alloc_bytes(text + b"\0") if text is not None else 0,
                               step & 0xFFFFFFFF, total & 0xFFFFFFFF)
        ctx.port.send("pg_box", step, total, _hex(text))
        name = f"box {case}: step {step} of {total}, text {text!r}"
        ctx.expect_equal(f"{name}: result", result, int(ctx.port.reply()[0]))
        progress.expect_same_state(name)
        _expect_same_flips(ctx, name)
        ctx.expect_same_framebuffer(name)
    return case + 1


def test_opcode_7(ctx):
    """A run of calls as the database load makes them, and single calls with other channels."""
    calls = 0
    for case in range(40):
        progress = Progress(ctx)
        ctx.sync_framebuffer(case)
        original = ctx.original
        progress.set_state(ctx.rng.choice([1, 1, 1, 0, 2]), ctx.rng.choice([0, 0, 1]), ctx.rng.choice([0, 1]))
        load = ctx.rng.random() < 0.3
        text = b"Loading database" if load else _text(ctx, allow_null=False)
        steps = [(step, 46) for step in range(47)] if load else [_step_total(ctx)
                                                                for _ in range(ctx.rng.randint(1, 4))]
        for number, (step, total) in enumerate(steps):
            channel = ctx.rng.choice([-1, -1, -1, 0, 0, ctx.rng.randint(1, 15)])
            original.flips = []
            original.call(FUN_OPCODE_7, original.alloc_bytes(text + b"\0"), step & 0xFFFFFFFF,
                          total & 0xFFFFFFFF, channel & 0xFFFF, this=STACK)
            ctx.port.send("pg_message", step, total, channel, _hex(text))
            name = f"opcode 7 {case} call {number}: step {step} of {total}, channel {channel}, text {text!r}"
            progress.expect_same_state(name)
            _expect_same_flips(ctx, name)
            calls += 1
        ctx.expect_same_framebuffer(f"opcode 7 {case}")
    return calls


def _expect_same_flips(ctx, case):
    """The same boxes flipped in the same order, each with the same screen at the time."""
    port = ctx.port
    port.send("flips")
    flips = []
    for _ in range(int(port.reply()[0])):
        words = [int(word) for word in port.reply()]
        flips.append((words[:4], port.read(FB_SIZE) if words[4] else None))
    wanted = ctx.original.flips
    ctx.expect_equal(f"{case}: flip boxes", [box for box, _ in wanted], [box for box, _ in flips])
    for number, ((_, a), (_, b)) in enumerate(zip(wanted, flips)):
        if a != b:
            ctx.expect_equal(f"{case}: screen at flip {number}", "same", "different")
    ctx.original.flips = []


TESTS = [
    ("progress_box", test_box),
    ("progress_opcode_7", test_opcode_7),
]
