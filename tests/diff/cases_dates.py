"""The dates of Date.cpp: the constructor and set, the weekday, the move to a weekday, and days and weeks
added and taken away."""
import struct

FUN_DATE_INIT = 0x549F70         # thiscall (s16 day, s8 month, u16 year, s16 weekday), returns this
FUN_DATE_SET = 0x54A130          # thiscall, the same arguments
FUN_DATE_DEFAULT = 0x54A110      # thiscall (), 10 Feb 1972
FUN_DATE_COPY = 0x417C10         # thiscall on the copy (out, source), returns out
FUN_DATE_WEEKDAY = 0x54A3F0      # thiscall (), returns 0 (Monday) to 6
FUN_DATE_TO_WEEKDAY = 0x54A2D0   # thiscall (s16 weekday, out)
FUN_DATE_ADD_DAYS = 0x54C5B0     # thiscall (out, s16 days)
FUN_DATE_SUB_DAYS = 0x54C8E0     # thiscall (out, s16 days)
FUN_DATE_ADD_WEEKS = 0x54C770    # thiscall (out, u8 weeks)
FUN_DATE_SUB_WEEKS = 0x54CAB0    # thiscall (out, u8 weeks)

G_LEFT_HELD = 0xB67A34           # the constructor clears it with its error box
# The guards of the function statics of the four add and take-away functions. On the first call each one
# registers a destructor with atexit, which the test does not need: it sets the guards.
STATIC_GUARDS = [0xAE2469, 0xAE2430, 0xAE249E, 0xAE24AA]

OPS = {"add": FUN_DATE_ADD_DAYS, "sub": FUN_DATE_SUB_DAYS, "addw": FUN_DATE_ADD_WEEKS, "subw": FUN_DATE_SUB_WEEKS}


def _setup(original):
    for guard in STATIC_GUARDS:
        original.write(guard, bytes([original.read(guard, 1)[0] | 1]))


def _leap(year):
    return int(year % 4 == 0 and (year % 100 != 0 or year % 400 == 0))


def _year(ctx):
    roll = ctx.rng.random()
    if roll < 0.6:
        return ctx.rng.randint(1990, 2012)
    if roll < 0.85:
        return ctx.rng.choice([1600, 1700, 1800, 1900, 2000, 2100, 2400, ctx.rng.randint(1600, 2500)])
    return ctx.rng.randint(0, 0xFFFF)


def _date(ctx):
    """Day, year and leap: mostly real dates, sometimes days past the year and odd leap values."""
    year = _year(ctx)
    roll = ctx.rng.random()
    if roll < 0.7:
        day = ctx.rng.randint(0, 364 + _leap(year))
    elif roll < 0.85:
        day = ctx.rng.choice([-1, 0, 364, 365, 366, ctx.rng.randint(-400, 800)])
    else:
        day = ctx.rng.randint(-0x8000, 0x7FFF)
    roll = ctx.rng.random()
    leap = _leap(year) if roll < 0.8 else ctx.rng.choice([0, 1, 0x10000, ctx.rng.randint(0, 0xFFFFFFFF)])
    return day, year, leap


def _pack(date):
    day, year, leap = date
    return struct.pack("<hHI", day, year, leap)


def _unpack(data):
    return list(struct.unpack("<hHI", data))


def _port_date(ctx):
    return [int(word) for word in ctx.port.reply()]


def _code_of_exe(original, address, size):
    """Bytes of the code as in the exe file: .text starts at file offset 0x1000."""
    with open(original.exe_path, "rb") as exe:
        exe.seek(address - 0x400000)
        return exe.read(size)


def test_date_init(ctx):
    original = ctx.original
    _setup(original)
    # The leagues suite replaces the start of the constructor with a return. The test puts the code back for
    # its own calls, then leaves it as it found it.
    patched = original.read(FUN_DATE_INIT, 8)
    _write_code(original, FUN_DATE_INIT, _code_of_exe(original, FUN_DATE_INIT, 8))
    try:
        return _test_date_init(ctx)
    finally:
        _write_code(original, FUN_DATE_INIT, patched)


def _write_code(original, address, code):
    """Writes code and drops what the emulator translated from the old bytes."""
    original.write(address, code)
    original.uc.ctl_remove_cache(address, address + len(code))


def _test_date_init(ctx):
    original = ctx.original
    cases = 0
    for case in range(1500):
        roll = ctx.rng.random()
        day = ctx.rng.randint(1, 31) if roll < 0.75 else ctx.rng.choice([0, -1, 32, ctx.rng.randint(-0x8000, 0x7FFF)])
        roll = ctx.rng.random()
        month = ctx.rng.randint(0, 11) if roll < 0.8 else ctx.rng.choice([-1, 12, ctx.rng.randint(-128, 127)])
        year = _year(ctx)
        roll = ctx.rng.random()
        weekday = -1 if roll < 0.4 else ctx.rng.randint(0, 6) if roll < 0.9 else ctx.rng.randint(-100, 100)
        function, kind = (FUN_DATE_INIT, "init") if case % 2 == 0 else (FUN_DATE_SET, "set")
        this = original.alloc(8)
        original.write(this, bytes(ctx.rng.randint(0, 255) for _ in range(8)))
        original.write_u32(G_LEFT_HELD, 1)
        original.error_lines.clear()
        result = original.call(function, day & 0xFFFF, month & 0xFF, year, weekday & 0xFFFF, this=this)
        wanted = _unpack(original.read(this, 8)) + [struct.unpack("<i", original.read(G_LEFT_HELD, 4))[0]]
        name = f"date_{kind} case {case} ({day}, {month}, {year}, {weekday})"
        if kind == "init":
            ctx.expect_equal(f"{name}: result", this, result & 0xFFFFFFFF)
        lines = {"init": [0x34, 0x3B], "set": [0x72, 0x79]}[kind]
        errors = ([lines[0]] if not 1 <= day <= 31 else []) + ([lines[1]] if not 0 <= month <= 11 else [])
        ctx.expect_equal(f"{name}: error lines", errors, original.error_lines, source="wanted")
        ctx.port.send("dt_init", kind, day, month, year, weekday)
        ctx.expect_equal(name, wanted, _port_date(ctx))
        cases += 1
    return cases


def test_date_default(ctx):
    original = ctx.original
    this = original.alloc(8)
    original.write(this, b"\xff" * 8)
    original.call(FUN_DATE_DEFAULT, this=this)
    ctx.port.send("dt_default")
    ctx.expect_equal("date_default", _unpack(original.read(this, 8)), _port_date(ctx))
    return 1


def test_date_weekday(ctx):
    original = ctx.original
    cases = 0
    for case in range(1500):
        date = _date(ctx)
        this = original.alloc_bytes(_pack(date))
        wanted = struct.unpack("<h", struct.pack("<I", original.call(FUN_DATE_WEEKDAY, this=this) & 0xFFFFFFFF)[:2])[0]
        ctx.port.send("dt_weekday", *date)
        ctx.expect_equal(f"date_weekday case {case} {date}", [wanted], _port_date(ctx))
        cases += 1
    return cases


def test_date_to_weekday(ctx):
    original = ctx.original
    _setup(original)
    cases = 0
    for case in range(1500):
        date = _date(ctx)
        roll = ctx.rng.random()
        weekday = ctx.rng.randint(0, 6) if roll < 0.85 else ctx.rng.randint(-100, 100)
        this = original.alloc_bytes(_pack(date))
        # Half the cases write the result over the source, as the constructor does.
        out = this if case % 2 else original.alloc(8)
        original.call(FUN_DATE_TO_WEEKDAY, weekday & 0xFFFF, out, this=this)
        ctx.port.send("dt_to_weekday", *date, weekday)
        ctx.expect_equal(f"date_to_weekday case {case} {date} to {weekday}", _unpack(original.read(out, 8)),
                         _port_date(ctx))
        cases += 1
    return cases


def test_date_copy(ctx):
    original = ctx.original
    cases = 0
    for case in range(200):
        date = _date(ctx)
        source = original.alloc_bytes(_pack(date))
        copy = original.alloc_bytes(bytes(ctx.rng.randint(0, 255) for _ in range(8)))
        out = original.alloc(8)
        result = original.call(FUN_DATE_COPY, out, source, this=copy)
        wanted = _unpack(original.read(copy, 8)) + _unpack(original.read(out, 8))
        ctx.expect_equal(f"date_copy case {case}: result", out, result & 0xFFFFFFFF)
        ctx.port.send("dt_copy", *date)
        ctx.expect_equal(f"date_copy case {case} {date}", wanted, _port_date(ctx))
        cases += 1
    return cases


def test_date_arithmetic(ctx):
    original = ctx.original
    _setup(original)
    cases = 0
    for case in range(4000):
        op = ctx.rng.choice(sorted(OPS))
        date = _date(ctx)
        if op in ("add", "sub"):
            roll = ctx.rng.random()
            # -0x8000 sends both programs into an endless recursion between the two functions.
            count = (ctx.rng.randint(-10, 10) if roll < 0.3 else ctx.rng.randint(-800, 800) if roll < 0.7
                     else ctx.rng.choice([0, 1, -1, 365, 366, -365, -366, 0x7FFF, -0x7FFF,
                                          ctx.rng.randint(-0x7FFF, 0x7FFF)]))
        else:
            count = ctx.rng.randint(0, 255) if ctx.rng.random() < 0.5 else ctx.rng.randint(0, 8)
        this = original.alloc_bytes(_pack(date))
        out = original.alloc(8)
        original.call(OPS[op], out, count & 0xFFFFFFFF, this=this)
        ctx.port.send("dt_op", op, *date, count)
        ctx.expect_equal(f"date_arithmetic case {case}: {op} {count} on {date}", _unpack(original.read(out, 8)),
                         _port_date(ctx))
        cases += 1
    return cases


def test_date_season(ctx):
    """The England start date of the inventory (section 3d): Saturday 11 Aug 2001, less 4 weeks, is day 194."""
    ctx.port.send("dt_init", "init", 10, 7, 2001, 5)
    entry = _port_date(ctx)
    ctx.expect_equal("date_season: 10 Aug 2001 to Saturday", [222, 2001, 0, 1], entry, source="wanted")
    ctx.port.send("dt_op", "subw", *entry[:3], 4)
    ctx.expect_equal("date_season: less 4 weeks", [194, 2001, 0], _port_date(ctx), source="wanted")
    return 2


TESTS = [
    ("date_init", test_date_init),
    ("date_default", test_date_default),
    ("date_weekday", test_date_weekday),
    ("date_to_weekday", test_date_to_weekday),
    ("date_copy", test_date_copy),
    ("date_arithmetic", test_date_arithmetic),
    ("date_season", test_date_season),
]
