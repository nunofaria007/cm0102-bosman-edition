# Database inventory of the original (input for the record-struct spec)

Scope: Data/*.dat files, the loader in cm0102.exe, record layouts, the port's state. Evidence tags: **[code]** = shown by an address in the exe, **[data]** = seen in the file bytes, **[inf]** = inference. Source file named by the assert strings: `E:\dev\CM3\cm3 00-01\cm3\code\Database.cpp` (string 0x9b5ac8, used by the loader) and a second file `Db_files.cpp`-style string at 0x9b8dc0 (starts `E:\dev\CM3\cm3 00-01\cm3\code\Db...`, used by every per-table reader; full name not read). All structs are packed to 1 byte.

## 1. Data files

Correction to the task text: `Data/` holds **20** .dat files (19 tables plus index.dat), not 34. There are also 10 .lng files, 10 events_*.cfg, 4 other .cfg, and language.ldb.

### index.dat (1482 bytes) **[code]+[data]**
- Size check: 8 + 22 x 67 = 1482. Header: two zero ints (unused by the loader as far as read).
- Entry (67 bytes, 0x43, stride seen in FUN_00528540 `pcVar17 + 0x43`, end 0xae2079 = 22 entries):

| Offset | Size | Meaning |
|---|---|---|
| 0x00 | 51 | file name, NUL-terminated. Bytes after the NUL are garbage (uninitialised memory), ignore them |
| 0x33 | 4 | table id (the loader switches on it) |
| 0x37 | 4 | record count |
| 0x3b | 4 | byte offset of the table inside the file |
| 0x3f | 4 | format version 1 or 2 (readers branch on it) |

- Readers take the entry as `param_4`: count at `+0x37`, flag at `+0x3f` (FUN_0054db50 at 0x54db50, FUN_0054d5e0, FUN_0054d740 and others).
- Format 1 = old layout, converted on read. Format 2 = raw copy of the memory struct. Any other value: error box (assert, FUN_005e8290).

### Table list (index order)

Sizes verified: file size = count x record size (or the staff section split, below). Globals are the loader's `DAT_` arrays; count global in brackets.

| # | File | id | Count | Rec size | Fmt | File size | Mem array [count] | Reader |
|---|---|---|---|---|---|---|---|---|
| 0 | club.dat | 0 | 13214 | 581 (0x245) | 2 | 7,677,334 | DAT_00ae23b4 [00ae235c] | 0x54db50 |
| 1 | nat_club.dat | 1 | 426 | 581 | 2 | 247,506 | DAT_00ae23b8 [00ae2360] | 0x54db50 |
| 2 | colour.dat | 2 | 34 | 58 (0x3a) | 1 | 1,972 | DAT_00ae23ec [00ae2394] | 0x550710 |
| 3 | continent.dat | 3 | 6 | 198 (0xc6) | 1 | 1,188 | DAT_00ae23a4 [00ae234c] | 0x54d5e0 |
| 4 | nation.dat | 4 | 213 | 290 (0x122) | 2 | 61,770 | DAT_00ae23a8 [00ae2350] | 0x54d740 |
| 5 | officials.dat | 7 | 4567 | 43 (0x2b) | 1 | 196,381 | DAT_00ae23e8 [00ae2390] | 0x5505c0 |
| 6 | stadium.dat | 5 | 8917 | 78 (0x4e) | 1 | 695,526 | DAT_00ae23b0 [00ae2358] | 0x54da00 |
| 7 | staff.dat part A | 6 | 169342 | 157 (0x9d) file, 110 (0x6e) memory | 1 | at offset 0 | DAT_00ae23bc [00ae2364] (110); temp DAT_00ae2404 [00ae2400] (157) | 0x54e780 (157) or 0x54e630 (110, flag 2) |
| 8 | staff.dat part B | 8 | 0 | - | 1 | offset 26,586,694 | none (no case 8 in the switch) | none |
| 9 | staff.dat part C | 9 | 30865 | 68 (0x44) | 2 | offset 26,586,694 | DAT_00ae23c0 [00ae2368] | 0x54e880 |
| 10 | staff.dat part D | 10 | 139769 | 70 (0x46) | 2 | offset 28,685,514 | DAT_00ae23c4 [00ae236c] | 0x54f030 |
| 11 | staff_comp.dat | 11 | 543 | 101 (0x65) | 2 | 54,843 | DAT_00ae23e0 [00ae2388] | 0x550380 |
| 12 | city.dat | 21 | 6682 | 56 (0x38) | 1 | 374,192 | DAT_00ae23ac [00ae2354] | 0x54d8a0 |
| 13 | club_comp.dat | 12 | 433 | 107 (0x6b) | 2 | 46,331 | DAT_00ae23d0 [00ae2378] | 0x54feb0 |
| 14 | nation_comp.dat | 16 | 19 | 107 | 2 | 2,033 | DAT_00ae23d8 [00ae2380] | 0x54feb0 |
| 15 | first_names.dat | 13 | 44281 | 60 (0x3c) | 1 | 2,656,860 | DAT_00ae23f0 [00ae2398] | 0x550860 |
| 16 | second_names.dat | 14 | 104395 | 60 | 1 | 6,263,700 | DAT_00ae23f4 [00ae239c] | 0x550860 |
| 17 | common_names.dat | 15 | 13256 | 60 | 1 | 795,360 | DAT_00ae23f8 [00ae23a0] | 0x550860 |
| 18 | staff_history.dat | 17 | 385846 | 17 (0x11) | 1 | 6,559,382 | DAT_00ae23cc [00ae2374] | 0x54fd60 |
| 19 | staff_comp_history.dat | 18 | 2609 | 58 (0x3a) | 1 | 151,322 | DAT_00ae23e4 [00ae238c] | 0x5504d0 |
| 20 | club_comp_history.dat | 19 | 10766 | 26 (0x1a) | 1 | 279,916 | DAT_00ae23d4 [00ae237c] | 0x550230 |
| 21 | nation_comp_history.dat | 20 | 273 | 26 | 1 | 7,098 | DAT_00ae23dc [00ae2384] | 0x550230 |

- Every size divides exactly. staff.dat: 169342 x 157 = 26,586,694 (= offset of part C); 30865 x 68 = 2,098,820 (26,586,694 + this = 28,685,514 = offset of part D); 139769 x 70 = 9,783,830 (28,685,514 + this = 38,469,344 = file size). **[data]**
- Record sizes in the table come from `_malloc(count * N)` in FUN_00528540 (0x245, 0x3a, 0xc6, 0x122, 0x4e, 0x6e, 0x9d, 0x2b, 0x44, 0x46, 0x65, 0x6b, 0x3c, 0x11, 0x1a, 0x38) and from the fread size in each reader. **[code]**
- Format flag 1 here means the reader just freads one record size (`fread(buf, N, count, f)`); the "format 1 = converted" branches (FUN_0054dc90 club, FUN_0054e9a0 player, FUN_0054f150 non-player, FUN_0054ffd0 comp) are used only when the flag is 1 for club/player/non-player/comp. In the shipped data those four tables have flag 2, so the converters are **unreachable for shipped data**. The port can skip them. For colour, continent, officials, stadium, staff_comp, city, names and the history tables the reader asserts flag 1 or 2 and does a raw fread. Exceptions: nation, club, nation_comp-style tables assert flag 2. **[code]**
- Staff part A has flag 1 and reads 157-byte records (FUN_0054e780 asserts flag 1). Flag 2 would read 110-byte records (FUN_0054e630 asserts flag 2). So the shipped file is the 157-byte form, converted to 110 in memory. **[code]**
- Part B (id 8) has count 0 and no switch case. Unused. **[code]**
- Network, tools, savegame: the loader has a second read path (FUN_00932c30 instead of fread FUN_00945bfe) that reads the same record layout from a memory/stream source. The wrappers FUN_00524d10 (calls FUN_00528540(a, b, 1)) is called from 0x82a9c0 and FUN_00524d50 from 0x81ce00, 0x827110, 0x828110. The port needs only the file path for a new game; the stream path is for saved games.

### Other files read at start
- `language.ldb` (string 0x9cd3d0), `*.lng` (strings 0x9b5b38 to 0x9b6344: dan swe esp nor por ita eng ger fra hol). eng.lng header: ints `0x3548(13640) 6 0x13(19) 0xd5(213) 0x1a1a(6682) 0x1b1(433) 0x22d5(8917) 0x21f(543)` then strings, the first being club name "KSK 's Gravenwezel". **[data]** Reading: counts of club+nat_club (13214+426 = 13640), continents, nation comps, nations, cities, club comps, stadiums, staff comps; the .lng holds the translated names for exactly those tables. **[inf]** from counts matching index.dat.
- `.cfg`: game.cfg (0x9cd378), pics.cfg (0x9899f0), player_setup.cfg (xref 0x7a30cc), pundit.cfg (xref 0x7da2db), weather.cfg (xref 0x92281f), euro.cfg (xrefs 0x9059fc, 0x905d4c), events_<lang>.cfg (xref 0x6d9f9e, 12 names listed from 0xa15c3c), auto_setup.cfg, quick_start.cfg (both named in strings, no xref followed).

## 2. The loader

- **FUN_00528540** (0x528540 to 0x52d97b, 21,564 bytes, 715 blocks; Ghidra garbles it). Signature from the callers' assembly: `__cdecl FUN_00528540(int allocMode, int arg2, int source)`. Wrappers push 1 as the third argument (FUN_00524d10 at 0x524d10: `mov eax,[esp+8]; mov ecx,[esp+4]; push 1; push eax; push ecx; call 0x528540`). **[code]** `source` selects fread (FUN_00945bfe) or the stream reader (FUN_00932c30); `allocMode` is stored in DAT_00ae2340 and selects `_malloc` or `GlobalAlloc`. The value 0 for the third argument is **[inf]** (FUN_00524d50 assembly not read).
- Path build: FUN_00945b13 is `_makepath`; FUN_0054d580 builds the path (`&DAT_00de1f64`, ext/dir string `&DAT_0098f928`) and fopens (FUN_00945b00). FUN_00946a22 is the fseek to the entry offset (field 0x3b). **[code]** Each index entry reopens its file and seeks, so staff.dat is opened four times.
- Steps (FUN_00528540): (1) read index.dat (string 0x9b5c48, referenced at 0x52870c and 0x52871c) into the 22-entry table at DAT_00ae1ab3 (entry base 0xae1a80, id at +0x33); (2) loop over entries, `switch(id)`: add counts to the totals (DAT_00ae235c etc.) and `_malloc(count * recsize)`; on malloc failure show the error box with the Database.cpp string; (3) a second loop over entries (`pcStack_818 * 0x43`, from 0x52b3b0) opens the file, seeks, calls the per-table reader, returns 0 on a short read; (4) a post-pass converts staff (157 to 110, below); (5) default-initialises reserve slots.
- Reserve slots: the sizing branch adds fixed extras: staff + 0x5f05 (24,325), or + 0x52d (1,325) when `cStack_81d != 0`; players + 23000; non-players + 0x52d; plus 0x7f club-comp/nation-comp rows and a 128-row loop at `(count - n) * 0x6b` that sets id, zeroes bytes 4, 0x38, 0x53, 0x57, 0x58, -1 to 0x59..0x65, 0 to 0x69. Staff reserve rows get default values (id, -1 ids, date 1972 day 0x28 via FUN_0054a110, year 0x7b3, ...). Used when a new game needs blank people (youth intake, regens). **[code]**; the exact condition for the extra count is unknown (`param_1` / DAT_00ae2340 versus `cStack_81d`).
- When allocMode is nonzero, all tables share one block (DAT_00ae23a4) laid out by the pointer-arithmetic at the end of the sizing branch (continent, then the three name tables, city, staff_comp, comps...). Pure memory layout, the port does not need it.
- Helper semantics: FUN_00417c10 copies an 8-byte date (`short, short, int`); FUN_0054a110 writes the default date `{0x28, 0x7b4, 1}`. **[code]**

## 3. Record layouts

Dates are 8 bytes: `u16 dayOfYear?, u16 year, u32 flag?`. **[code]** copy and default (FUN_00417c10, FUN_0054a110); **[data]** the year at +2 is 1850..2040 for all 3000 sampled staff at offsets 0x10, 0x31, 0x3e, 0x46. The meaning of the first and last parts is [inf].

### 3.1 staff.dat part A: staff, 157 bytes in file, 110 in memory
Memory struct (DAT_00ae23bc, 110 = 0x6e). The first 0x60 bytes are identical in file and memory. Evidence: the copy block at 0x52cc45 to 0x52d002 (xrefs to DAT_00ae2404) and the blank-slot init in FUN_00528540.

| Offset | Size | Type | Meaning | Evidence |
|---|---|---|---|---|
| 0x00 | 4 | int | staff id (equals the row number) | init writes row index; [data] row i has id i |
| 0x04 | 4 | int | first-name id | init -1; [inf] |
| 0x08 | 4 | int | second-name id | init -1; [inf] |
| 0x0c | 4 | int | common-name id | init 0; [inf] |
| 0x10 | 8 | date | date of birth | [data]; [inf] meaning |
| 0x18 | 2 | short | unknown (init 0x7b3 = 1971) | init |
| 0x1a | 4 | int | unknown id (init -1) | init |
| 0x1e | 4 | int | unknown id (init -1) | init |
| 0x22 | 1 | byte | unknown | copy |
| 0x23 | 1 | byte | unknown | copy |
| 0x24 | 4 | int | unknown id (init -1) | init |
| 0x28 | 1 | byte | unknown | copy |
| 0x29 | 8 | date | unknown date | copy (FUN_00417c10) |
| 0x31 | 8 | date | unknown date | [data] |
| 0x39 | 4 | int | unknown id (init -1) | init |
| 0x3d | 1 | byte | unknown | copy |
| 0x3e | 8 | date | unknown date | [data] |
| 0x46 | 8 | date | unknown date | [data] |
| 0x4e | 4 | int | unknown | init 0 |
| 0x52 | 4 | int | unknown | init 0 |
| 0x56 to 0x5e | 9 | bytes | unknown | init 0 |
| 0x5f | 1 | byte | staff kind: [data] 1 = player record only (29,571 rows), 2 = non-player only (138,477), 3 = both (1,294); init in blank slots = 6, so more kinds may exist | [data] tally; init |
| 0x60 | 1 | byte | unknown | copy |
| 0x61 | 4 | int | index into the non-player table (id 10) or -1 | file 0x91 copied here at 0x52cc.. ; [data] range 0..139768 only when 0x5f is 2 or 3 |
| 0x65 | 4 | int | index into the 52-byte extra table (DAT_00ae23c8); set by the loader to the row number | [code] |
| 0x69 | 4 | int | index into the player table (id 9) or -1 | file 0x99 copied here; [data] range 0..30864 when 0x5f is 1 or 3 |
| 0x6d | 1 | byte | unknown (init 0) | init |

File-only tail (157 - 0x61 = 60 bytes): 0x61 to 0x90 = 12 ints (mostly -1) copied into the extra table; 0x91 int = non-player index; 0x95 int = ignored by the loader; 0x99 int = player index. Total 0x9d. **[code]**
- Extra table DAT_00ae23c8 (52 = 0x34 bytes per row, count DAT_00ae2370, built from DAT_00ae2400 rows): `[0] int row id`, `[4..0x30] 12 ints` from file 0x61..0x8d. Meaning of the 12 ints unknown. **[code]**, meaning unknown.
- The 157-byte buffer is freed after conversion (FUN_009452ca). So the port can read 157 and keep only the converted form.

### 3.2 staff.dat part C: player data, 68 bytes (DAT_00ae23c0)
Reader 0x54e880 (raw fread 0x44). Field boundaries from the format-1 reader FUN_0054e9a0 and the blank init in FUN_00528540: int @0x00 (row index; blank init writes it); shorts @0x04, 0x06, 0x08, 0x0a, 0x0c; 21 single bytes @0x0e..0x22; 8 ints @0x23, 0x27, 0x2b, 0x2f, 0x33, 0x37, 0x3b, 0x3f (init -1); byte @0x43. Meaning: unknown for all fields (inference: attribute and position bytes; no access function traced).

### 3.3 staff.dat part D: non-player data, 70 bytes (DAT_00ae23c4)
Reader 0x54f030; boundaries from FUN_0054f150: int @0x00; byte @0x04; shorts @0x05, 0x07, 0x09, 0x0b, 0x0d; 55 bytes @0x0f..0x45. Meaning unknown.

### 3.4 club.dat and nat_club.dat, 581 bytes (DAT_00ae23b4, DAT_00ae23b8)
Boundaries from the format-1 reader FUN_0054dc90 (0x54dc90) with 0x80 as a short (the shipped format 2 reads raw):

| Offset | Size | Type | Notes |
|---|---|---|---|
| 0x00 | 4 | int | club id (= row number [data]) |
| 0x04 | 51 | char[51] | full name, e.g. "KSK 's Gravenwezel" [data] |
| 0x38 | 26 | char[26] | short name, e.g. "'s Gravenwezel" [data]. Bytes 0x37 and 0x52 are not read by the converter; [data] 0xff-style separators [inf] |
| 0x53 | 4 | int | unknown (id-like; -2 or e.g. 147) |
| 0x57 | 4 | int | unknown (id-like, -2, or odd values like -5290557 for row 1000 -> maybe not an id) |
| 0x5b | 4 | int | unknown |
| 0x5f | 1 | byte | unknown |
| 0x60 | 4 | int | unknown |
| 0x64 | 1 | byte | unknown |
| 0x65, 0x69 | 4+4 | int | unknown |
| 0x6d | 1 | byte | unknown |
| 0x6e | 4 | int | unknown |
| 0x72 | 1 | byte | unknown (255 default) |
| 0x73, 0x77, 0x7b | 4 each | int | unknown |
| 0x7f | 1 | byte | unknown |
| 0x80 | 2 | short | scaled value: converter stores `byte * 500`, 0 becomes 1 (FUN_0054dc90); raw reader also fixes 0 to 1 at +0x80 (FUN_0054db50 loop). Values 500, 1000 [data]. Meaning unknown |
| 0x82 | 1 | byte | unknown |
| 0x83 to 0xbf | 16 x 4 | int | unknown (many -1/-2: ids) |
| 0xc3 | 12 | int[3] | unknown |
| 0xcf, 0xd3 | 4+4 | int | unknown |
| 0xd7 | 200 | int[50] | all -1 for sampled clubs; probably a list (players?) |
| 0x19f | 20 | int[5] | list |
| 0x1b3 | 28 | int[7] | list |
| 0x1cf | 12 | int[3] | list |
| 0x1db | 4 | int | unknown |
| 0x1df | 1 | byte | unknown (255) |
| 0x1e0 | 80 | int[20] | list |
| 0x230 | 16 | int[4] | list |
| 0x240 | 4 | int | unknown |
| 0x244 | 1 | byte | unknown (end, 581 total) |

Meanings of every non-name field are unknown: no accessor was traced. Sizes and offsets are exact **[code]**.

### 3.5 Name files (first_names, second_names, common_names), 60 bytes **[data]**, no field access traced
`char name[51] @0` (NUL-padded, ISO-8859-1 accents such as 0xE9 [data]); `int id @0x33` (equals the row number, row 0 is a placeholder); `int nation @0x37` (nation id up to 201, -2 on row 0/1 entries [data]); `u8 count @0x3b` (1..~100, a frequency weight [inf]). Longest names: first 24, second 50, common 28. Reader 0x550860 (raw fread 0x3c).

### 3.6 Other tables (all offsets [data] unless marked; sizes [code])
All of colour, continent, nation, stadium, city, comps start with `int id @0` and `char name[51] @4` (names in the first 55 bytes), except where noted.
- **colour** (58): id, name[51] @4, then 3 bytes @0x37 that equal the RGB of the colour (Black 00 00 00, White ff ff ff) [data] [inf].
- **continent** (198): id; name @4 ("Asia"); 3-letter code @0x1e+1 ("ASI" at 30 after a 0xff); adjective @35 ("Asian"); long name @61; short code @162 (AFC); a double at ~0xbd ("0.9" style weight) [data].
- **nation** (290): id; name @4; second name @0x37+1 (55) ; code @82 ("ALB") ; adjective @87 ("Albanian"); two ints -2 @160; doubles at 173, 225 and 249 (floats like 0.33) ; three ints -2 @272; trailing bytes @285. Meaning of the numbers unknown.
- **stadium** (78): id; name @4; byte @55 = 0xff; ints at 56.. (capacity-like values, e.g. 0x3ef = 1007? unknown); int -2 @72.
- **city** (56): id; name @4 (up to 0x1f); 0xff; int 0x13 @36 (country-like); two doubles (latitude, longitude style: 51.0, 3.7 for Gent) @40-ish; int -2 @52. Doubles [data] [inf].
- **club_comp** (107) and **nation_comp** (107): id; long name @4; short name @56; abbreviation @83 ("D2"); ints; -2 pairs @97. Reader 0x54feb0 (raw 0x6b).
- **staff_comp** (101): id; name @4; short name @56; ints -2 @83 or 87. Reader 0x550380.
- **officials** (43): int id; int @4 (e.g. 0x33); int @8 (staff-id style, 0x6777); further ints and bytes; layout unknown beyond that. Reader 0x5505c0.
- **staff_history** (17, sequential rows per staff): int row @0; int @4 (staff id, 0xf843 repeated for two rows); u16 year @8 (0x7c0 = 1984, 1985); int @0xa (club id [inf]); byte @0xe; u16 @0xf (appearances-style count [inf]). Reader 0x54fd60. [data], meaning [inf].
- **staff_comp_history** (58), **club_comp_history** (26), **nation_comp_history** (26): rows of ints with a u16 year; club_comp_history: int row, int comp id (7), u16 year (0x761 = 1889), int, int, and two -1 ints [data]. Readers 0x5504d0, 0x550230. Field meanings unknown.

## 4. The port

- `src/cm3/database.c` (38 lines): a stub. `database_init(data_dir)` stores the path and returns true; `database_shutdown` clears it. Its comment cites `FUN_0068d520` and `Db_files.cpp` as the loader; **this does not match**. The loader is FUN_00528540 and the source file is Database.cpp (record readers in a `Db…` file). FUN_0068d520 was not checked.
- No record structs exist in `include/` or elsewhere. Only `database.c` mentions staff, club, `.dat` or `database_init`. `CMakeLists.txt` line 23 builds it. The port neither agrees nor disagrees on layout, since it has none.

## 5. Order of loading at game start

1. Read `Data/index.dat` (22 entries) into the entry table (0x52870c).
2. Allocate every table (entry order, via `_malloc(count * recsize)`).
3. Read in index order, reopening each file and seeking to the entry offset: club, nat_club, colour, continent, nation, officials, stadium, staff A (157-byte, then converted to 110 + extra table), staff B (skipped), staff C (players), staff D (non-players), staff_comp, city, club_comp, nation_comp, first_names, second_names, common_names, staff_history, staff_comp_history, club_comp_history, nation_comp_history.
4. Convert staff 157 to 110, build the 52-byte extra table, set default values of reserve slots.
5. Other start needs: `language.ldb`, one `<lang>.lng` (names for clubs, nations, cities, comps, stadiums), `events_<lang>.cfg`, `player_setup.cfg`, `pundit.cfg`, `weather.cfg`, `euro.cfg`, `game.cfg`, `pics.cfg`, and for a quick start `auto_setup.cfg` / `quick_start.cfg`. Order of these relative to the database load is unknown.

## 6. Open doubts

- Meaning of nearly every field in staff, player, non-player, club and the numeric parts of the other tables. Needs accessor functions (callers of the DAT_ arrays) or the .lng format.
- Whether the club name fields in club.dat or the .lng are shown to the player (the .lng has an own club-name table).
- The third argument of the loader for a file load (FUN_00524d50 not read) and the exact condition for the reserve-slot counts.
- `Db…` source file name (string 0x9b8dc0) not read in full.
- The port's `database.c` comment names FUN_0068d520; unchecked.
- Staff kind byte 0x5f: value 6 in blank slots, only 1 to 3 in the file.
- Officials, staff_history, history tables, nation, stadium and city sub-fields are layout guesses from file bytes, not code.
- The 8-byte date: meaning of the 4th to 8th bytes and of the first short (day of year?) is inferred.
