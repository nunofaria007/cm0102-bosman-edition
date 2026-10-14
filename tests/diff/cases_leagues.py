"""The nation and league tables: index.dat, the nation and continent tables, the key ids, the league
table, the default selection, the league order, the extra division text and the name compare."""
import os
import struct
import tempfile

from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_ESP

from cases_helpers import _hex

# Readers of Db_files.cpp: (FILE *, from stream, buffer, index entry). Return 1 when all records came.
FUN_READ_CONTINENTS = 0x54D5E0
FUN_READ_NATIONS = 0x54D740
FUN_MATCH_NATION = 0x60D810          # (name, row) of index.cpp
CONTINENT_LOOP = (0x604278, 0x604347)  # the continent loop inside FUN_00601a40
FUN_LEAGUE_TABLE = 0x667150
FUN_DEFAULT_LEAGUES = 0x827D50
FUN_LEAGUE_ORDER = 0x66A0C0
FUN_EXTRA_DIVISION = 0x669F50
FUN_NAME_COMPARE = 0x90C1D0
FUN_DATE = 0x549F70                  # thiscall, 4 arguments. The port has no dates

G_CONTINENTS, G_CONTINENT_COUNT = 0xAE23A4, 0xAE234C
G_NATIONS, G_NATION_COUNT = 0xAE23A8, 0xAE2350
G_LANGUAGE = 0x9EB2DC
G_KEY_IDS = (0x9CF178, 0x9D0528)
G_LEAGUES = 0xB63D60
G_TO_LOAD, G_SELECTED, G_FIRST = 0xAE2D00, 0xAE2D04, 0xAE2D08

INDEX_ENTRIES, ENTRY_SIZE = 22, 0x43
NATION_SIZE, CONTINENT_SIZE = 0x122, 0xC6
LEAGUES, LEAGUE_SIZE = 35, 0x48

KEY_NATIONS = [0x9CF218, 0x9CF224, 0x9CF244, 0x9CF260, 0x9CF2AC, 0x9CF2C0, 0x9CF2E4, 0x9CF2FC, 0x9CF300,
               0x9CF314, 0x9CF31C, 0x9CF338, 0x9CF2DC, 0x9CF360, 0x9CF36C, 0x9CF3E4, 0x9CF408, 0x9CF430,
               0x9CF434, 0x9CF448, 0x9CF45C, 0x9CF484, 0x9CF488, 0x9CF4A8, 0x9CF4DC, 0x9CF4F8, 0x9CF510]
KEY_CONTINENTS = [0x9CFA08, 0x9CFA0C, 0x9CFA10, 0x9CFA14, 0x9CFA18, 0x9CFA1C]

# Id fields of a nation that the original turns into pointers, with their target: "c" continent,
# "n" nation. The port keeps the other id fields (city, stadium, colours) as they are.
NATION_LINKS = {0x71: "c", 0x110: "n", 0x114: "n", 0x118: "n"}

# Names with accents, other cases and near misses, for the sort and the matchers.
NAMES = [b"England", b"ENGLAND", b"england", b"Engl", b"Englands", b"\xc9cosse", b"Ecosse", b"\xe9cosse",
         b"Spain", b"Espa\xf1a", b"Wales", b"Wal\xe9s", b"Japan", b"", b"Zaire", b"a", b"A", b"Republic Of Ireland",
         b"Republic of Ireland", b"Holland", b"Africa", b"EUROPE", b"Oceania", b"\xd6sterreich", b"Austria"]


def _u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def _s32(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


class Tables:
    """The nation and continent tables in both programs, loaded from the data folder."""

    def __init__(self, ctx, data_dir=None):
        self.ctx = ctx
        self.data_dir = data_dir or ctx.data_dir
        original = ctx.original
        self._patch(original)
        with open(os.path.join(self.data_dir, "index.dat"), "rb") as index:
            self.index = index.read()
        self.entries = {}
        for i in range(INDEX_ENTRIES):
            raw = self.index[8 + i * ENTRY_SIZE:8 + (i + 1) * ENTRY_SIZE]
            self.entries[_s32(raw, 0x33)] = raw
        self.continents = self._read(FUN_READ_CONTINENTS, self.entries[3], CONTINENT_SIZE)
        self.nations = self._read(FUN_READ_NATIONS, self.entries[4], NATION_SIZE)
        self.continent_count = _s32(self.entries[3], 0x37)
        self.nation_count = _s32(self.entries[4], 0x37)
        original.write_u32(G_CONTINENTS, self.continents)
        original.write_u32(G_CONTINENT_COUNT, self.continent_count)
        original.write_u32(G_NATIONS, self.nations)
        original.write_u32(G_NATION_COUNT, self.nation_count)
        ctx.port.send("lg_load", self.data_dir)
        loaded = ctx.port.reply()
        ctx.expect_equal("port load", [b"1", str(self.continent_count).encode(), str(self.nation_count).encode()],
                         loaded, source="wanted")
        self.set_language(0)

    @staticmethod
    def _patch(original):
        if getattr(original, "leagues_ready", False):
            return
        original.leagues_ready = True
        # mov eax, ecx; ret 0x10. The dates of the league table are not ported.
        original.write(FUN_DATE, bytes.fromhex("89c8c21000"))

    def _read(self, reader, entry, size):
        """Opens the file of an index entry, seeks to its table and calls the reader of the original."""
        original = self.ctx.original
        name = entry[:entry.index(0)].decode("latin-1")
        found = [e for e in os.listdir(self.data_dir) if e.lower() == name.lower()]
        stream = original._fopen(original.alloc_c_string(os.path.join(self.data_dir, found[0])), 0)
        original.files[stream].seek(_s32(entry, 0x3B))
        count = _s32(entry, 0x37)
        buffer = original.alloc(count * size)
        result = original.call(reader, stream, 0, buffer, original.alloc_bytes(entry))
        original._fclose(stream)
        self.ctx.expect_equal(f"original reader {reader:#x}", 1, result, source="wanted")
        return buffer

    def set_language(self, language):
        self.ctx.original.write_u32(G_LANGUAGE, language)
        self.ctx.port.send("lg_language", language)

    def nation_row(self, pointer):
        return -1 if pointer == 0 else (pointer - self.nations) // NATION_SIZE

    def continent_row(self, pointer):
        return -1 if pointer == 0 else (pointer - self.continents) // CONTINENT_SIZE

    def rename(self, row, name):
        self.ctx.original.write(self.nations + row * NATION_SIZE + 4, name[:50].ljust(51, b"\0"))
        self.ctx.port.send("lg_rename", row, _hex(name[:50]))

    def rename_continent(self, row, name):
        self.ctx.original.write(self.continents + row * CONTINENT_SIZE + 4, name[:25].ljust(26, b"\0"))
        self.ctx.port.send("lg_crename", row, _hex(name[:25]))

    def match_keys(self, case):
        """The key ids of both programs after the nation and continent matchers."""
        original = self.ctx.original
        for address in range(*G_KEY_IDS, 4):
            original.write_u32(address, -1)
        for row in range(self.nation_count):
            original.call(FUN_MATCH_NATION, self.nations + row * NATION_SIZE + 4, row)
        uc = original.uc
        uc.reg_write(UC_X86_REG_ESP, 0x20000000 + 0x100000 - 0x1000)
        uc.reg_write(UC_X86_REG_EBX, 0)
        uc.emu_start(CONTINENT_LOOP[0], CONTINENT_LOOP[1])
        wanted = [str(struct.unpack("<i", original.read(a, 4))[0]).encode() for a in KEY_NATIONS + KEY_CONTINENTS]
        self.ctx.port.send("lg_keys", original.exe_path)
        self.ctx.expect_equal(f"{case}: key ids", wanted, self.ctx.port.reply())
        return [int(w) for w in wanted]

    def build_table(self, case):
        original = self.ctx.original
        result = original.call(FUN_LEAGUE_TABLE)
        wanted = [str(result).encode()]
        for i in range(LEAGUES):
            entry = original.read(G_LEAGUES + i * LEAGUE_SIZE, LEAGUE_SIZE)
            wanted += [str(v).encode() for v in (self.nation_row(_u32(entry, 0)), self.continent_row(_u32(entry, 4)),
                                                _u32(entry, 8), _s32(entry, 0x17))]
        self.ctx.port.send("lg_table")
        self.ctx.expect_equal(f"{case}: league table", wanted, self.ctx.port.reply())

    def flags(self):
        original = self.ctx.original
        flags = bytes(original.read(self.nations + row * NATION_SIZE + 0x11C, 1)[0]
                      for row in range(self.nation_count))
        first = _u32(original.read(G_FIRST, 4), 0)
        return [flags.hex().encode(), str(_s32(original.read(G_TO_LOAD, 4), 0)).encode(),
                str(_s32(original.read(G_SELECTED, 4), 0)).encode(), str(self.nation_row(first)).encode()]


def _expect_index(ctx, data):
    wanted = [str(_s32(data, 0)).encode(), str(_s32(data, 4)).encode()]
    for i in range(INDEX_ENTRIES):
        raw = data[8 + i * ENTRY_SIZE:8 + (i + 1) * ENTRY_SIZE]
        wanted.append(_hex(raw[:raw.index(0)]).encode())
        wanted += [str(_s32(raw, offset)).encode() for offset in (0x33, 0x37, 0x3B, 0x3F)]
    ctx.port.send("lg_index")
    ctx.expect_equal("index", wanted, ctx.port.reply())


def leagues_index(ctx):
    """index.dat: the two ints and the 22 entries that the loader reads into 0xae1a80."""
    tables = Tables(ctx)
    _expect_index(ctx, tables.index)
    ctx.port.send("lg_load", os.path.join(ctx.data_dir, "no such folder"))
    ctx.expect_equal("missing folder", [b"0", b"0", b"0"], ctx.port.reply(), source="wanted")
    return 2


def _synthetic_folder(ctx, folder, extra_byte=b""):
    """A data folder with other header ints, the two tables after some junk at the offsets of their
    index entries, and changed links of some nations."""
    with open(os.path.join(ctx.data_dir, "index.dat"), "rb") as index:
        data = bytearray(index.read())
    data[0:8] = struct.pack("<2i", ctx.rng.randint(-99, 99), ctx.rng.randint(1, 99999))
    for i in range(INDEX_ENTRIES):
        base = 8 + i * ENTRY_SIZE
        table = _s32(data, base + 0x33)
        if table not in (3, 4):
            continue
        name = data[base:data.index(0, base)].decode()
        junk = ctx.rng.randint(1, 300)
        with open(os.path.join(ctx.data_dir, [e for e in os.listdir(ctx.data_dir) if e.lower() == name][0]),
                  "rb") as source:
            records = bytearray(source.read())
        if table == 4:
            for row in range(_s32(data, base + 0x37)):
                offset = row * NATION_SIZE
                if ctx.rng.random() < 0.3:
                    link = ctx.rng.choice([0x110, 0x114, 0x118, 0x71])
                    top = 5 if link == 0x71 else 212
                    value = ctx.rng.choice([0, 1, top, -1, -2, -77, ctx.rng.randint(0, top)])
                    records[offset + link:offset + link + 4] = struct.pack("<i", value)
        data[base + 0x3B:base + 0x3F] = struct.pack("<i", junk)
        with open(os.path.join(folder, name), "wb") as target:
            target.write(bytes(ctx.rng.randint(0, 255) for _ in range(junk)) + records)
    with open(os.path.join(folder, "index.dat"), "wb") as index:
        index.write(bytes(data) + extra_byte)


def leagues_other_files(ctx):
    """index.dat, the records and the links from a data folder with other header ints, offsets and
    links. An index with a non-zero byte after the entries makes the port fail: that path is not
    ported."""
    cases = 0
    for case in range(3):
        with tempfile.TemporaryDirectory() as folder:
            _synthetic_folder(ctx, folder)
            tables = Tables(ctx, folder)
            _expect_index(ctx, tables.index)
            cases += 1 + _expect_records(ctx, tables)
    with tempfile.TemporaryDirectory() as folder:
        _synthetic_folder(ctx, folder, b"\x00")
        Tables(ctx, folder)
        cases += 1
    with tempfile.TemporaryDirectory() as folder:
        _synthetic_folder(ctx, folder, b"\x01")
        ctx.port.send("lg_load", folder)
        ctx.expect_equal("index with a language block", [b"0", b"0", b"0"], ctx.port.reply(), source="wanted")
        cases += 1
    return cases


def _expect_record(ctx, case, original_bytes, port_hex, links, counts):
    """Compares a record field by field. A link field holds the row behind the pointer, or -1."""
    wanted = bytearray(original_bytes)
    for offset, target in links.items():
        value = _s32(wanted, offset)
        if value < 0:
            value = -1
        elif value >= counts[target]:
            value = -2       # the original points past the table here. The port has no such pointer
        wanted[offset:offset + 4] = struct.pack("<i", value)
    port = bytes.fromhex(port_hex.decode())
    if bytes(wanted) != port:
        first = next(i for i in range(len(port)) if wanted[i] != port[i])
        ctx.expect_equal(f"{case}: byte {first:#x}", bytes(wanted).hex(), port.hex())


def _expect_records(ctx, tables):
    original = ctx.original
    counts = {"c": tables.continent_count, "n": tables.nation_count}
    for row in range(tables.continent_count):
        ctx.port.send("lg_continent", row)
        _expect_record(ctx, f"continent {row}", original.read(tables.continents + row * CONTINENT_SIZE,
                                                               CONTINENT_SIZE), ctx.port.reply()[0], {}, counts)
    for row in range(tables.nation_count):
        ctx.port.send("lg_nation", row)
        _expect_record(ctx, f"nation {row}", original.read(tables.nations + row * NATION_SIZE, NATION_SIZE),
                       ctx.port.reply()[0], NATION_LINKS, counts)
    return tables.continent_count + tables.nation_count


def leagues_records(ctx):
    """Every field of every continent and nation record, with the row behind each link."""
    return _expect_records(ctx, Tables(ctx))


def leagues_keys(ctx):
    """The key ids of the 27 nations and 6 continents, with the shipped names and with renamed rows."""
    tables = Tables(ctx)
    tables.match_keys("shipped")
    cases = 1
    for case in range(40):
        for _ in range(ctx.rng.randint(1, 6)):
            row = ctx.rng.randrange(tables.nation_count)
            tables.rename(row, ctx.rng.choice(NAMES + [b"Brazil", b"WALES", b"united states", b"Norway "]))
        if ctx.rng.random() < 0.4:
            tables.rename_continent(ctx.rng.randrange(tables.continent_count),
                                    ctx.rng.choice(NAMES + [b"asia", b"North America", b"South  America"]))
        tables.match_keys(f"renamed {case}")
        cases += 1
    return cases


def leagues_table(ctx):
    """The league table of FUN_00667150, with the shipped names and with key nations missing."""
    tables = Tables(ctx)
    tables.match_keys("shipped")
    tables.build_table("shipped")
    cases = 1
    for case in range(20):
        for _ in range(ctx.rng.randint(1, 4)):
            tables.rename(ctx.rng.randrange(tables.nation_count), ctx.rng.choice(NAMES))
        if ctx.rng.random() < 0.3:
            tables.rename_continent(ctx.rng.randrange(tables.continent_count), ctx.rng.choice(NAMES))
        tables.match_keys(f"renamed {case}")
        tables.build_table(f"renamed {case}")
        cases += 1
    # No nation table: the original shows an error box and returns 0.
    ctx.original.write_u32(G_NATIONS, 0)
    ctx.port.send("lg_free")
    ctx.expect_equal("no table", ctx.original.call(FUN_LEAGUE_TABLE), int(_table_result(ctx)))
    tables.set_language(-1)
    return cases + 1


def _table_result(ctx):
    ctx.port.send("lg_table")
    return ctx.port.reply()[0]


def leagues_default(ctx):
    """FUN_00827d50 for each language index, after random flags and counters."""
    tables = Tables(ctx)
    tables.match_keys("shipped")
    original = ctx.original
    cases = 0
    for language in list(range(-2, 15)) * 2:
        for _ in range(ctx.rng.randint(0, 8)):
            row = ctx.rng.randrange(tables.nation_count)
            value = ctx.rng.choice([1, 2, 3, 4, 6, 7, 0xFF])
            original.write(tables.nations + row * NATION_SIZE + 0x11C, bytes([value]))
            ctx.port.send("lg_set_flag", row, value)
        counters = [ctx.rng.randint(0, 9), ctx.rng.randint(0, 9), ctx.rng.randrange(tables.nation_count)]
        original.write_u32(G_TO_LOAD, counters[0])
        original.write_u32(G_SELECTED, counters[1])
        original.write_u32(G_FIRST, tables.nations + counters[2] * NATION_SIZE)
        ctx.port.send("lg_counters", *counters)
        tables.set_language(language)
        original.call(FUN_DEFAULT_LEAGUES)
        ctx.port.send("lg_default")
        reply = ctx.port.reply()
        wanted = tables.flags()
        ctx.expect_equal(f"language {language}", wanted, reply[1:])
        ctx.expect_equal(f"language {language}: result", b"1" if 0 <= language <= 12 else b"0", reply[0],
                         source="wanted")
        cases += 1
    tables.set_language(-1)
    return cases


def leagues_order(ctx):
    """FUN_0066a0c0 by name and by second name, for language 0 and 10, with renamed rows."""
    tables = Tables(ctx)
    original = ctx.original
    cases = 0
    keys = tables.match_keys("shipped")
    for case in range(30):
        if case:
            # A missing key nation gives a pointer before the table in the original, so the renames
            # keep every key: they change rows that hold no key, and Africa, which no league uses.
            for _ in range(ctx.rng.randint(1, 5)):
                row = ctx.rng.choice([r for r in range(tables.nation_count) if r not in keys[:27]])
                tables.rename(row, ctx.rng.choice(NAMES))
                if ctx.rng.random() < 0.5:
                    second = ctx.rng.choice(NAMES)[:25]
                    original.write(tables.nations + row * NATION_SIZE + 0x38, second.ljust(26, b"\0"))
                    ctx.port.send("lg_rename2", row, _hex(second))
            if ctx.rng.random() < 0.3 and keys[27] >= 0:
                tables.rename_continent(keys[27], ctx.rng.choice(NAMES[:8]))
        keys = tables.match_keys(f"order {case}")
        tables.build_table(f"order {case}")
        for language in (0, 10):
            tables.set_language(language)
            for by_second in (0, 1):
                address = original.call(FUN_LEAGUE_ORDER, by_second)
                wanted = [str(b).encode() for b in original.read(address, LEAGUES)]
                ctx.port.send("lg_order", by_second)
                ctx.expect_equal(f"order {case}, language {language}, second {by_second}", wanted,
                                 ctx.port.reply())
                cases += 1
    tables.set_language(-1)
    return cases


def leagues_extra(ctx):
    """FUN_00669f50 for every nation, with a buffer, with a short buffer and with none."""
    tables = Tables(ctx)
    tables.match_keys("shipped")
    original = ctx.original
    cases = 0
    for row in list(range(-1, tables.nation_count)):
        for size, with_buffer in ((2000, 1), (20, 1), (19, 1), (0, 0), (5, 0)):
            buffer = original.alloc(2048) if with_buffer else 0
            nation = 0 if row < 0 else tables.nations + row * NATION_SIZE
            result = original.call(FUN_EXTRA_DIVISION, nation, buffer, size)
            text = original.read_c_string(buffer) if buffer else ""
            ctx.port.send("lg_extra", row, size, with_buffer)
            reply = ctx.port.reply()
            ctx.expect_equal(f"nation {row}, size {size}, buffer {with_buffer}",
                             [str(result).encode(), _hex(text.encode("latin-1")).encode()], reply)
            cases += 1
    tables.set_language(-1)
    return cases


def leagues_name_compare(ctx):
    """FUN_0090c1d0 on random names with accents, for both cases, limits and languages 0 and 10."""
    original = ctx.original
    port = ctx.port
    cases = 0
    for language in (0, 10, 3):
        original.write_u32(G_LANGUAGE, language)
        port.send("lg_language", language)
        for _ in range(400):
            a = _random_name(ctx)
            b = _random_name(ctx) if ctx.rng.random() < 0.6 else _near(ctx, a)
            if ctx.rng.random() < 0.05:
                a = None
            if ctx.rng.random() < 0.05:
                b = None
            keep_case = ctx.rng.choice([0, 1, 1, 2])
            max_len = ctx.rng.choice([0, 0, 1, 2, 3, 5, 0xFFFFFFFF])
            pa = original.alloc_bytes(a + b"\0") if a is not None else 0
            pb = original.alloc_bytes(b + b"\0") if b is not None else 0
            wanted = original.call(FUN_NAME_COMPARE, pa, pb, keep_case, max_len)
            port.send("lg_cmp", keep_case, max_len, _hex(a), _hex(b))
            ctx.expect_equal(f"{a!r} {b!r} case {keep_case} limit {max_len} language {language}",
                             wanted, int(port.reply()[0]))
            cases += 1
    original.write_u32(G_LANGUAGE, -1)
    port.send("lg_language", -1)
    return cases


def _random_name(ctx):
    roll = ctx.rng.random()
    if roll < 0.4:
        return ctx.rng.choice(NAMES)
    if roll < 0.7:
        return bytes(ctx.rng.choice(b"aAeEoOzZ \xe9\xc9\xf1\xd1\xe0\xff\x80\x9a\x8a\x01") for _ in
                     range(ctx.rng.randint(0, 6)))
    return bytes(ctx.rng.randint(1, 255) for _ in range(ctx.rng.randint(0, 8)))


def _near(ctx, name):
    """The same name with one change, or none."""
    name = bytearray(name)
    roll = ctx.rng.random()
    if name and roll < 0.3:
        i = ctx.rng.randrange(len(name))
        name[i] = ctx.rng.choice([name[i] ^ 0x20, 0xE9, 0x45, 0x65, ctx.rng.randint(1, 255)])
    elif roll < 0.5:
        name += bytes([ctx.rng.randint(1, 255)])
    elif name and roll < 0.7:
        name = name[:-1]
    return bytes(name)


TESTS = [("leagues_index", leagues_index), ("leagues_records", leagues_records),
         ("leagues_other_files", leagues_other_files), ("leagues_keys", leagues_keys),
         ("leagues_table", leagues_table), ("leagues_default", leagues_default), ("leagues_order", leagues_order),
         ("leagues_extra", leagues_extra), ("leagues_name_compare", leagues_name_compare)]
