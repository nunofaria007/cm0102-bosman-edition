# From "Start New Game" to the first data screen

Scope: one local player, the shipped data, default settings. The path starts with the click on menu item 1 of the "Setup Game" screen and ends at the first screen that lists database records. That screen is "Select League(s)". It lists the nations whose leagues the player can run.

Method: objdump disassembly of the original (Intel syntax), REA (Ghidra) for function sizes, and reads of the files in `Data/`. Each fact gives the address that shows it. "Inference" marks a conclusion that the code does not show directly. Numbers are hex unless the text says otherwise. `S` is the screen stack object at 0xb74450. `D` is the screen manager of the current record. `area_create`, `widget_create`, `cell_button` (0x8040a0) and `cell_label` (0x804100) have the argument orders of `2026-10-14-start-screen-helpers.md`.

Source files named by the assert strings: `Setup.cpp` (0xa801dc) for `FUN_0081ce00`, the league screen and `FUN_00827d50`. `key_nation.cpp` (0x9eaff4) for `FUN_00667150` and `FUN_00669f50`. `Database.cpp` (0x9b5ac8) for the loader and `FUN_005315a0`. `game_config.cpp` (0x9cd3a0) for `FUN_005d8480`.

## Summary

1. The click calls `FUN_0081ce00`. With no database in memory, it checks free memory, loads the whole database with a progress box, runs three setup functions and pushes the league screen. There is no screen between "Setup Game" and "Select League(s)", only the progress box.
2. "Select League(s)" (build 0x81d010, input 0x81dc30) lists 27 nations from `nation.dat` in a scrolled area of 16 rows. England is selected at the start for an English language setting.
3. The screen reads the nation table (`DAT_00ae23a8`, count `DAT_00ae2350`) at +0x00 (id), +0x04 (name) and +0x11c (league flags, a run-time byte). It also reads a static table of 35 league entries at 0xb63d60 that `FUN_00667150` fills after the load. The continent table (+0x04 name) is read only by the sort.
4. The texts go through the plain path of `FUN_0066f4e0`. No `{...}` or `[...]` tag appears. One format uses the `<` note rule. The nation names go through `sprintf("  %s")` and not through the text function. No `.lng` file is read on this path when the language setting is 0.

## 1. Screens the player sees

### 1a. "Setup Game" with the progress box

The click on item 1 runs `FUN_0081bd70`, which calls `FUN_0081ce00` and returns -4 (start screen helpers, section 8).

`FUN_0081ce00` (0x81ce00..0x81d002, 515 bytes, cdecl, no arguments):

```c
void start_new_game(void) {
    if (DAT_00ae23a8 == 0) {                                   /* nation table, 0x81ce00 */
        if (free_mb() < 50) {                                  /* FUN_007a1710, 0x81ce13 */
            DAT_00dd7138 = 1;
            if (push(S, 0x826d20, 0x826e60, DAT_00dd7118, 0, 0))   /* "No Memory" screen */
                set_slot(S, 0, 0, 0);
            return;                                            /* 0x81ce5d */
        }
        char path[0x100];
        strcpy(path, FUN_0090bde0("CM3_DATA", ""));            /* 0x81ce66..0x81ce9b */
        if (!FUN_00524d50(path, 1)) {                          /* 0x81cea5 */
            error_box("Error", "v.. Setup.cpp 1672");          /* line 0x688 */
            FUN_005cc7d0(); FUN_005a8f60(); exit(-1);          /* 0x81cf0c..0x81cf22 */
        }
        FUN_005d8480(DAT_00ae3138);                            /* settings apply, thiscall, 0x81cf2d */
        FUN_00827d50();                                        /* league flags, 0x81cf32 */
        if (!FUN_00667150()) {                                 /* league table, 0x81cf37 */
            error_box("Error", "v.. Setup.cpp 1683");          /* line 0x693 */
            exit(-1);
        }
    }
    if (push(S, 0x81d010, 0x81dc30, 0, 0, 0)) {                /* 0x81cfc9 */
        set_slot(S, 0, 0, 0);                                  /* scroll position */
        set_slot(S, 1, 0, 0);                                  /* area index */
        DAT_00a15704 = 1;                                      /* 1 in the file already */
    }
}
```

- `FUN_007a1710` (64 bytes): `GlobalMemoryStatus` (import 0x9670c4), returns `(dwAvailPhys + dwAvailPageFile) >> 20`, the free memory in MB (0x7a172e..0x7a173a).
- The "No Memory" screen (build 0x826d20, 320 bytes; input 0x826e60, 190 bytes) shows heading, side bar, sub-heading "No Memory" (0xa8127c), the text "There is not enough available system memory to create the game. ..." (0xa81214, slot 0 = 0 selects it) through `FUN_005ed070`, and an "Ok" button id 0x34 at (100,555)-(790,590). The port can leave this screen out if it treats free memory as large. That is a decision for the spec.
- When the database is already in memory (a second click after "Back"), the load is skipped and the league screen is pushed again (0x81ce0d).

The load runs inside the input function of "Setup Game". The player sees the "Setup Game" screen with a progress box over its bottom bar. The box is drawn directly, not as a screen (section 2c).

### 1b. "Select League(s)", build `0x81d010`

Build 0x81d010..0x81dc24 (3,094 bytes). Input 0x81dc30..0x81e8cd (3,230 bytes). Neither is a Ghidra procedure start. Push flag 0, no leave and no exit function.

Build steps, in order. Values are for a fresh new game: `DAT_00dd7118` = 0 (no game exists), `DAT_00b74348` = 0.

1. Local column weights: `{0x19,1,0xd,1,0xf,1,0x13}` at esp+0x18 and `{0x2a,0xd,1,0xd,0xa,0x2a,0xd,1,0xd}` at esp+0x20 (0x81d01a..0x81d08a).
2. `mem7 = free_mb() / 7`, at least 1 (0x81d08f..0x81d0ad). `DAT_00dd7124 = 0`.
3. `FUN_0041bfb0()` background. `text_format(0xdd70a8, "Championship Manager 2001/02")` and `heading(DAT_00ae31a8, DAT_00ae3184, 0xdd70a8)` (0x81d0b8..0x81d0e9).
4. `FUN_0075db00(4, 0)` side bar, mode 1, the same as on the start screen.
5. Sub-heading "Select League(s)" (0xa80418). It is "Selected League(s)" (0xa8042c) when `DAT_00dd7118` != 0 (0x81d0fe).
6. Options area, when `DAT_00b74348` == 0: `area_create(110, 145, 525, 165, 9, {42,13,1,13,10,42,13,1,13}, 1, 0, 2, 0, -1)` (0x81d16a). When `DAT_00b74348` != 0, the weights are `{0,0,0,0,0,42,13,1,13}` and the real-players widgets are not made (0x81d343).
7. Option widgets. Font 1, text flags 0xc for the buttons, colour `DAT_00ae317e` (grey). Selected value: style 0x801 and text colour `DAT_00aebdd4` (orange). Other value: style 1, text colour grey.

| Column | Text | Kind | Id | Selected when | Evidence |
|---|---|---|---|---|---|
| 0 | "  Use Real Players:" (0xa80404) | cell_button, style 1, text flags 1, text colour light grey | 0 | | 0x81d1a7 |
| 1 | "Yes" (0x98e8d0) | cell_button | 0x39 | byte `DAT_009b5981` != 0 | 0x81d26b..0x81d2cd |
| 3 | "No" (0x98e8cc) | cell_button | 0x3a | `DAT_009b5981` == 0 | 0x81d2d2..0x81d339 |
| 5 | "  Attribute Masking:" (0xa803ec) | cell_button, style 1, text flags 1 | 0 | | 0x81d3b5 |
| 6 | "Yes" | cell_button | 0x3b | byte `DAT_009cc270` != 0 | 0x81d49b..0x81d4fe |
| 8 | "No" | cell_button | 0x3c | `DAT_009cc270` == 0 | 0x81d503..0x81d56a |

`DAT_009b5981` is 1 in the file (bytes `01 01 00 00` at 0x9b5980). `DAT_009cc270` is 1 in the file. So "Yes" is selected in both rows at the start. When `DAT_00dd7118` != 0, the same widgets have id 0.

8. Select area: `area_create(530, 145, 780, 165, 2, 0, 1, 0, 1, 0, -1)` (0x81d5a1).
   - Column 0 "Select All" (0x99b4c8): `cell_button(style 0x30, colour grey, text flags 0xc, font 1, light grey, id 0xa)` when `DAT_00ae2d04` < 15 and `mem7` >= 16. Else `cell_label` with text flags 0x2c (0x81d5dd..0x81d63a). `DAT_00ae2d04` here is the value from before step 9.
   - Column 1 "De-Select All" (0xa803dc): button id 0xb when `DAT_00ae2d00` > 0, else label 0x2c (0x81d674..0x81d6cd).
9. List area: `area_create(110, 170, 780, 535, 7, {25,1,13,1,15,1,19}, 16, 0, 2, 0, -1)` (0x81d705). 16 rows are visible.
10. `order = FUN_0066a0c0(0)` (0x81d70f): a byte array of the 35 league entries in sort order (section 3c). Then `DAT_00ae2d04` = count of entries with a nation whose flag byte has bit 1 (0x81d71f..0x81d74a).
11. Row loop over the 35 entries in `order` (0x81d753..0x81db86). `row` starts at 0. An entry with nation pointer 0 is skipped and uses no row (0x81d767 jumps past `inc edi`). For each other entry `n = entry.nation`, `f = n->flags (+0x11c)`:

| Column | Widget | Evidence |
|---|---|---|
| 0 | `"  " + n->name` by `sprintf("  %s", n+4)` (0x987ff0, `FUN_00944c9f`). When `entry+8` == 0: cell_label, style 1, colour 0, text flags 1, font 3, grey. Else cell_button style 1, colour 0, text flags 1, font 3, text colour yellow (`DAT_00aebdec`) when f != 0, else light grey, id 0xc, data n | 0x81d9ab..0x81da2e |
| 2 | "SELECTED" (0xa803d0), cell_button font 2, text flags 0xc, colour 0, id 0xd, data n. Style 0x801 and orange when bit 1 of f, else style 1 and grey | 0x81da4f..0x81da9f |
| 4 | "BACKGROUND" (0xa803c4), the same with id 0xe and bit 0 | 0x81daa4..0x81daf3 |
| 6 | Text from `FUN_00669f50(n, buf, 2000)`. Only when it returns 1. cell_button id 0x38, data n, font 2, text flags 0xc. Style 0x801 and orange when bit 2 of f, else style 1 and grey | 0x81daf8..0x81db74 |

   Columns 2, 4 and 6 are made only when `entry+8` != 0. Every entry has a non-zero function pointer at +8 (section 3c), so all 27 rows are full rows.
12. `FUN_005edcc0(back, next)` with `back = (FUN_007fec60(0) == 0)` and `next = (DAT_00ae2d04 != 0)` (0x81dba8..0x81dbc7). After "Start New Game" the start screen is the previous node with flag 0, so Back is a button. With England selected, Next is a button.
13. `pos = get_slot(0)`. If the list area index is not -1: `FUN_00403320(pos)` on the area object `D + 4 + area * 0xbf1`, which sets the area's `scroll_to` (+0x1fc), clamped to `0 .. last_row - rows_shown + 1` (0x403320..0x403348). Then `set_slot(1, area, 0)` (0x81dbd5..0x81dc19).

`FUN_00669f50(nation, buf, size)` (368 bytes) gives the text of column 6 by the nation id at nation+0 (0x669fdf). It errors when `buf` is set and `size` < 20 (0x669f69).

| Key id global | Nation (matched by name) | Text | Evidence |
|---|---|---|---|
| `DAT_009cf2e4` | England | "Conference Division<%s - COMMENT - English Conference>" with "" gives "Conference Division" | 0x669fe7, 0x9eb078 |
| `DAT_009cf314` | Germany | "Regional Divisions" | 0x66a012 |
| `DAT_009cf360` | Italy | "Serie C2 A, B, C" | 0x66a03c |
| `DAT_009cf434`, `DAT_009cf488` | Portugal, Spain | "Second Division B" | 0x66a062..0x66a06a |
| `DAT_009cf4a8` | Sweden | "Second Division" | 0x66a090 |

The key ids are -1 in the file and are set at load time by name (section 2d). The names come from the code that writes each global: 0x60deb1 "England", 0x60e00d "Germany", 0x60e234 "Italy", 0x60e844 "Portugal", 0x60ea8c "Spain", 0x60eb64 "Sweden".

What the player sees at the start, English language setting, inference from the code and the data: 27 rows in this order: Argentina, Australia, Belgium, Brazil, Croatia, Denmark, England, Finland, France, Germany, Greece, Holland, Italy, Japan, Northern Ireland, Norway, Poland, Portugal, Republic Of Ireland, Russia, Scotland, South Korea, Spain, Sweden, Turkey, United States, Wales. Rows 0..15 are visible, with a scrollbar. England is yellow, with "SELECTED" in orange and a grey "Conference Division" button. All 27 names exist in `nation.dat` (rows 8, 11, 19, 27, 48, 53, 61, 69, 70, 74, 76, 84, 93, 95, 98, 129, 139, 149, 150, 155, 161, 171, 172, 180, 192, 196, 207) [data].

### 1c. "Select League(s)", input `0x81dc30`

cdecl, one argument `clicked` (s16 in a 32-bit slot). The id is `widget[clicked]+0x08`, or the injected id `S+0x26278e` (0xdd6bde) when that is 0. Widget data (+0x48, read at `0xba9a6`) gives the nation pointer.

1. If `DAT_00ae23a8` == 0: return -2 (0x81dc43).
2. If clicked != -1: `set_slot(0, FUN_00803320(get_slot(1)), 0)` (0x81dc62..0x81dc81). This saves the scroll position of the list area for the rebuild. `FUN_00803320` (112 bytes) returns `FUN_004032e0` of the area: the row of the first visible widget when the area scrolls (+0xbed != 0), else 0 (0x4032e0..0x403310).
3. Ids. Each case returns -11 (rebuild) unless the table says otherwise.

| Id | Action | Evidence |
|---|---|---|
| 0xa Select All | For each of the 35 entries with a nation and a function pointer: f == 0 gives f = 2, `d00++`, `d04++`. f with bit 0 gives f = (f - 1) or 2, `d04++`. `d08` = this nation when `d08` is 0 | 0x81dcca..0x81dd6b |
| 0x39, 0x3a | `DAT_009b5981` = 1, 0 | 0x81ddb6, 0x81de08 |
| 0x3b, 0x3c | `DAT_009cc270` = 1, 0 | 0x81deac, 0x81de5a |
| 0xb De-Select All | Flag byte = 0 for every nation in the table. `d00 = d04 = d08 = 0` | 0x81defb..0x81df3b |
| 0xc name | f == 0: memory check, then f = 2, `d00++`, `d04++`, `d08` = n when 0. Else: `d00--`, `d04--` when bit 1, f = 0. If n was `d08`: `d08` = first entry in table order with bit 1 | 0x81df7d..0x81e0fd |
| 0x38 extra division | f == 0: memory check, then f = 2, `d00++`, `d04++`, `d08` set as above. Then toggle bit 2: set gives `d00++`, clear gives `d00--` | 0x81e139..0x81e288 |
| 0xd SELECTED | f == 0: as 0xc. Bit 1 set: `d00--` (again for bit 2), `d04--`, f = 0, new `d08`. Else (background): `d04++`, f = (f and ~1) or 2, `d08` = n when 0 | 0x81e2c4..0x81e496 |
| 0xe BACKGROUND | f == 0: memory check, then `d00++`, f = 1. Bit 0 set: `d00--` (again for bit 2), f = 0. Else (selected): `d04--`, f = (f and ~2) or 1. If n was `d08`: `d08` = first entry whose flag byte equals 2 | 0x81e4d2..0x81e667 |
| -3 Next | Section 1d | 0x81e6a3 |
| other | r = `FUN_007615d0(clicked)`. If 0, r = `FUN_007643d0(clicked)`. Back (-2) returns -2, history back to "Setup Game" | 0x81e8ad..0x81e8c3 |

`d00`, `d04`, `d08` are `DAT_00ae2d00` (count of leagues to load: selected, background and extra divisions), `DAT_00ae2d04` (count of selected nations) and `DAT_00ae2d08` (pointer to the first selected nation). Meanings are inference from the updates.

Memory check (0x81dfc2..0x81e043, repeated for 0x38, 0xd, 0xe): when the command line (0xae2c98) does not contain "-dontcheckram" (0xa80484), and `DAT_009cd1fc` != 0 (1 in the file), and `d00` >= `free_mb() / 7` (at least 1): error box "Not enough memory" / "Not enough memory to select that many leagues" (0xa80470, 0xa80440) through `FUN_005e8290`, and return -11 with no change.

### 1d. After "Select League(s)" (not traced in full)

Next (-3), 0x81e6ad: `DAT_00dd7118` != 0 gives -3. `d04` == 0 gives 0. `d04` > 1 pushes "Select Start Season" (0xa804ac; build 0x81e8d0, input 0x81eb00) and returns -4. `d04` == 1 (the default England case) sets `DAT_00dd7124 = 1` and calls `FUN_00828110(0)` (10,416 bytes, game creation). Then it pushes either 0x826aa0/0x826c20 (when `DAT_00dd7134`), the "No Memory" screen (when `DAT_00dd7138`), or the "Manager Status" screen 0x81ff10/0x820950 with slots 0..4 = 0 and calls `FUN_00820d40` (0x81e707..0x81e8a6). These are outside the scope of this file.

## 2. The database load

### 2a. Call chain and signature

`FUN_0081ce00` → `FUN_00524d50(path, 1)` → `FUN_00528540(path, 1, 0)`.

- `FUN_00524d50` (32 bytes): `push 0; push [esp+8]; push [esp+4]; call 0x528540` (0x524d50..0x524d5c).
- `FUN_00524d10` passes 1 as the third argument (0x524d18).
- Correction to `2026-10-13-database-inventory.md`: the signature is `int FUN_00528540(const char *data_dir, int shared_block, int from_stream)`. Argument 1 is the directory (ebp, used for `_makepath` at 0x52862f and for `FUN_0054d580(dir, "index.dat")` at 0x528722). Argument 2 goes to `DAT_00ae2340` (0x5289bc). When it is non-zero, the sizing loop at 0x52990e sums all sizes and makes one `GlobalAlloc` block (0x529ccb..0x529cdf), and `FUN_00528080` frees with `GlobalFree`. Argument 3 selects `FUN_00932c30` (stream) or `fread` (`FUN_00945bfe`). Start New Game passes (data dir, 1, 0). Returns 1 on success (0x52d902), 0 on a short read.

### 2b. Steps of `FUN_00528540` for a new game

1. `FUN_0090bde0("CM3_TEMP", "")` (0x52857a). `FUN_00528080()` frees the old tables (0x5285a8, 1,216 bytes).
2. Progress box: `text_format(buf, "Loading database")` (0x9b5c54), then `FUN_00802f80(buf, 0, 0x2e, -1)` (0x5285d4).
3. Copy every `*.lng` file (0x9b5b38) of the data folder to `CM3_TEMP\<name>`: `FUN_004c0aa0` / `FUN_004c0b50` (findfirst, findnext), `FUN_0090cc50(src, dst)` (file copy, "rb" to "wb") (0x52861f..0x5286d2).
4. `FUN_00535930([DAT_00ae3138]+8)` (0x5286e4): 1 when `CM3_TEMP\<lang>.lng` exists. The language index maps 0, 1, 11, 12 to eng.lng and 2..10 to fra, ger, ita, esp, dan, hol, nor, por, swe (0x535930..0x535a60). If the file is missing: `FUN_005d8020(0)` on the settings object sets the language to 0 (0x5286f7).
5. Open `index.dat`. Read int → `DAT_00ae2344`, int → `DAT_00ae2348`, 22 entries of 0x43 bytes → 0xae1a80 (0x528734..0x5287da). Then one more byte (0x528834). The shipped file has no such byte (1482 = 8 + 22 × 67), so the byte stays 0. Then `DAT_009b5984` = 0 (0x528a30). With a non-zero byte the loader would read 0x1cc bytes, a count, a list of ints into `DAT_00ae23fc`, and 4 bytes into `DAT_009b5984` (0x528848..0x52896f).
6. Progress step 1 (0x5289a9). Sizing loop over the 22 entries, one progress step per entry (0x529930, jump table 0x52d9d8 for shared-block mode).
7. Read loop over the 22 entries in index order, one progress step per entry (0x52b3ab). The table order and readers are in the database inventory, section 5.
8. Last progress step (0x52c671). Staff conversion 157 to 110 bytes and the 52-byte extra table (database inventory, 3.1).
9. Shared-block mode: `FUN_005315a0(0)` (0x52d1aa, 6,240 bytes). Its error texts name it `resolve_database_indexes()` (0x9b6240). It replaces each id field by a 32-bit pointer into the target table, in place. A negative id becomes NULL (example 0x531f6a..0x531f8b). Section 3a lists the nation fields.
10. Delete `duff.out` in the current folder (`FUN_0094643a` = `DeleteFileA`, 0x52d205).
11. Staff pass over every staff row except the last 16 and kind 6 (0x52d225..0x52d81c): drop staff of extinct clubs (`FUN_0053c190`), fix job bytes +0x28 and +0x3d, set a missing nation (+0x1a) from the club (+0x39 → club+0x53) or the national club (+0x24), else a random nation (`FUN_0090cfc0(nation count)`, 0x52d3aa). A missing name pointer (+0x04, +0x08, +0x0c) becomes the first empty common name. The log texts go to 0xde1f64 through `sprintf` and are not shown. In the shipped data, 77 staff rows have no nation, and each of them has a club or a national club [data]. Inference: the random branch does not run for the shipped data.
12. When the index byte is 0: for each national club row, `+0x80 = FUN_0053b3d0(club+0x53)->+0x80 / 2`; a club whose +0x57 record is named "Reserve" (0x9b5b40) gets +0x57 = 0 (0x52d82c..0x52d8f8).
13. `FUN_00601a40()` (0x52d8fa, 41,024 bytes): sets about 1,100 key id globals (0x9cf178..0x9d0524) to -1, then matches records by name. For each nation row i, `FUN_0060d810(nation+4, i)` (6,080 bytes) compares the name with `FUN_00951c50` (inference: `_stricmp`) against a fixed list and stores i in the matching global (0x60420d..0x60423c). The same is done for colours, club comps (`FUN_0060efd0`), continents and staff comps (`FUN_006115e0`). When `DAT_009b5984` != 0 it first reads `CM3_TEMP\eng.lng` (0x603c52..0x603c89). Not for the shipped index.
14. Return 1.

### 2c. The progress box

`FUN_00802f80(text, step, total, channel)` writes opcode 7. The local side calls `FUN_005e76f0(text, step, total)` (336 bytes):

- Panel (100,555)-(790,590), grey, style 0x30 (0x5e774e).
- When 0 < total and step <= total: text in (120,565)-(250,580), font 1, text flags 0xc, light grey; panel (270,565)-(770,580), grey, style 0x60; fill (272,567)-(272 + step × 496 / total, 578) in navy (`DAT_00aebe04`), style 4 (0x5e776a..0x5e77e2).
- Else: the text across (120,565)-(770,580).
- Then it shows the box rectangle (`FUN_005e2eb0(100, 555, 790, 590)`).

The steps are 0, 1, then 22 sizing steps, 22 read steps and one last call, with total 46 (0x2e). The fourth argument of the read loop calls is EDI, whose value there was not traced.

### 2d. Setup after the load

| Order | Function | Size | What it does | Evidence |
|---|---|---|---|---|
| 1 | `FUN_005d8480` (thiscall on `[DAT_00ae3138]`) | 1,728 | Applies the settings. If the settings language (+0x08) differs from `FUN_0066f4d0()` it reloads `language.ldb` (`FUN_0066eb40`). If it differs from `DAT_009b5984` (the language of the names in the database) it calls `FUN_005339a0(lang, 0)` (8,064 bytes), which loads the names of `CM3_TEMP\<lang>.lng` into the tables. It applies sound and display settings and writes `CM3_PREFS\game.cfg` (mode "wb", 0x5d87ea..0x5d8802) | 0x5d8580..0x5d85bd |
| 2 | `FUN_00827d50` | 960 | Flag byte +0x11c = 0 for every nation. `d00 = d04 = d08 = 0`. Then by `FUN_0066f4d0()` (= `DAT_009eb2dc`, jump table 0x8280dc) one nation gets flag 2, `d00 = d04 = 1`, `d08` = that nation. 0, 1 England; 2 France; 3 Germany; 4 Italy; 5 Spain; 6 Denmark; 7 Holland; 8 Norway; 9 Portugal; 10 Sweden; 11 Japan; 12 South Korea. Other values give an error box | 0x827d94..0x828068 |
| 3 | `FUN_00667150` | 8,176 | Fills the league table at 0xb63d60 (section 3c). Error and return 0 when the nation table is missing | 0x6671e8..0x669124 |

## 3. Record layouts the first data screen uses

All structs are packed to 1 byte. After `FUN_005315a0` the id fields hold 32-bit pointers. A port on 64-bit macOS cannot keep pointers in a 4-byte field. Inference: the port should keep the ids and resolve them on access.

### 3a. Nation, 290 bytes (`DAT_00ae23a8`, count `DAT_00ae2350` = 213)

| Offset | Size | Type | Meaning | Evidence |
|---|---|---|---|---|
| 0x000 | 4 | s32 | Id, equals the row | `FUN_00669f50` compares it with key ids (0x669fdf). Key ids are row numbers (0x604220). [data] England row 61 has id 61 |
| 0x004 | 51 | char[] | Name, shown on the screen | 0x81d9ab, 0x66a25d, 0x604221 |
| 0x037 | 1 | u8 | Unknown, 0xff in the data | [data] |
| 0x038 | 26 | char[] | Second name. The sort uses it when `FUN_0066a0c0` gets 1 ("Select Start Season") | 0x66a245. Size [data], inference |
| 0x052 | 1 | u8 | Unknown, 0xff | [data] |
| 0x053 | 4 | char[] | Three-letter code, "ENG" | [data] |
| 0x057 | 26 | char[] | Nationality, "English". The loader prints nation+0x57 as a name | 0x52d264. Size inference |
| 0x071 | 4 | id → continent* | Continent | 0x531f6a..0x531f8b |
| 0x075 | 5 | | Unknown | [data] |
| 0x07a | 4 | id → city* | City | 0x531f95..0x531fb5 |
| 0x07e | 2 | | Unknown | |
| 0x080 | 4 | id → stadium* | Stadium | 0x531fbf..0x531fe4 |
| 0x084 | 12 | | Unknown | |
| 0x090 | 24 | 6 × id → colour* | Colours at 0x90, 0x94, 0x98, 0x9c, 0xa0, 0xa4 | 0x531ff1..0x532128 |
| 0x0a8 | 104 | | Unknown, doubles in the data | [data] |
| 0x110 | 12 | 3 × id → nation* | Other nations (inference: rivals) | 0x532135..0x5321ac |
| 0x11c | 1 | u8 | League flags at run time. Bit 0 background, bit 1 selected, bit 2 extra division. 0 in the file for every nation | 0x81d7db, 0x81d85c, 0x81e245, 0x827da8. [data] all 0 |
| 0x11d | 5 | | Unknown, `ff ff ff ff 00` | [data] |

The first data screen reads +0x00, +0x04 and +0x11c, and writes +0x11c.

### 3b. Continent, 198 bytes (`DAT_00ae23a4`, count `DAT_00ae234c` = 6)

Only +0x04 (name) is read on this path, by the sort comparator when two league entries have no nation and both have a continent (0x66a21b..0x66a227). Those entries are not shown, so the name only decides their order. Other fields: database inventory 3.6.

### 3c. League entry, 0x48 bytes, 35 entries at 0xb63d60 (not from a file)

`FUN_00667150` zeroes 0x276 dwords (35 × 0x48) and writes each entry from constants and key ids (0x6671ea..0x6671f9).

| Offset | Size | Meaning | Evidence |
|---|---|---|---|
| +0x00 | 4 | Nation pointer: `nation_base + key_id × 0x122`. 0 for entries 0..7 | 0x667887 (entry 8, Argentina) |
| +0x04 | 4 | Continent pointer: `continent_base + key_id × 0xc6`. Set for entries 3..7, and also for the nation entries | 0x66742d (entry 3) |
| +0x08 | 4 | Function pointer, non-zero for all 35: 0x830620, 0x830ac0, ... 0x8383b0. Inference: the function that sets up that league in a new game | 0x667225, 0x669047 |
| +0x15 | 2 | First word of a date from `FUN_00549f70` | 0x6673ea |
| +0x17 | 4 | 0 or 1. The season screen prints "%s %02d/%02d" for 1 and "%s %d" for 0 (inference: season over two years) | 0x6673d6, 0x81e9be |
| +0x1e | 4 | Club comp pointer: `club_comp_base + key_id × 0x6b` | 0x668f72 |
| +0x2a | 8 | Date | 0x66723a |
| +0x39 | 8 | Date | 0x667276 |

Entries 8..34 hold, in this order: Argentina, Australia, Belgium, Brazil, Croatia, Denmark, England, Finland, France, Germany, Greece, Holland, Republic Of Ireland, Italy, Japan, Northern Ireland, Norway, Poland, Portugal, Russia, Scotland, South Korea, Spain, Sweden, Turkey, United States, Wales. The names come from the matcher code that writes each key id global (0x60d8da Argentina, ... 0x60ee33 Wales).

Sort, `FUN_0066a0c0(param)` (0x66a0c0..0x66a0f4): fills 0xb63d38 with 0..34, stores `param` at 0xb63d5c, and calls `qsort` (`FUN_00945333`, MSVC) with the comparator 0x66a100. Comparator, for entries a and b:

- Both with no nation and no continent: order by index (0x66a191..0x66a1fa).
- No nation and no continent before anything else.
- No nation (continent entries) before nations. Two continent entries: by continent name.
- Two nations: by name (+0x04), or by +0x38 when `param` != 0.
- Names compare with `FUN_0090c1d0(a, b, 1, 0)` (256 bytes). It maps each byte through column 0 of the 3-byte table at 0xaa2154, which removes accents and keeps the case (é → e, É → E, A → A, a → a), and compares as unsigned bytes. When `FUN_0066f4d0()` == 10 it skips the table. With argument 3 = 0 it would use column 2 (lower case).

For the 27 ASCII names this order equals a plain byte compare (inference).

## 4. Functions on the path that the port does not have

Sizes are in bytes of the original. "Large" means more than 4,000 bytes.

### Database

| Function | Size | Note |
|---|---|---|
| `FUN_00524d50` | 32 | Wrapper |
| `FUN_00528540` | 21,904, large | Loader. Ghidra garbles it |
| `FUN_00528080` | 1,216 | Frees the tables |
| `FUN_0054d580` | 48 | Path and fopen |
| 17 table readers (0x54d5e0, 0x54d740, 0x54d8a0, 0x54da00, 0x54db50, 0x54e630, 0x54e780, 0x54e880, 0x54f030, 0x54fd60, 0x54feb0, 0x550230, 0x550380, 0x5504d0, 0x5505c0, 0x550710, 0x550860) | 240 to 320 each, about 4,400 in all | Raw reads for the shipped formats |
| `FUN_005315a0` | 6,240, large | `resolve_database_indexes()` |
| `FUN_0053c190`, `FUN_00542740`, `FUN_0053b3d0`, `FUN_0054fc10` | 1,024, 352, 288, 240 | Staff and club fixups |
| `FUN_00549f70`, `FUN_00417c10`, `FUN_0054a110` | 416, 64, 32 | Dates |
| `FUN_0053dba0` | 1,440 | Staff name text, used only for log texts. Can be left out |
| `FUN_00601a40` | 41,024, large | Key ids. Most of it sets globals to -1 |
| `FUN_0060d810`, `FUN_0060efd0`, `FUN_006115e0` | 6,080, 9,744, 12,496, large | Name matchers. The first screen needs only the 27 nation names of `FUN_0060d810` |
| `FUN_00535930` | 336 | Language file check |
| `FUN_004c0aa0`, `FUN_004c0b50`, `FUN_004c0c10`, `FUN_0090cc50` | 176, 192, 48, 256 | Copy of .lng files to CM3_TEMP. Can be left out while no .lng is read |
| `FUN_005339a0` | 8,064, large | .lng names. Only when the settings language differs from the database language |
| `FUN_0054dc90`, `FUN_0054e9a0`, `FUN_0054f150`, `FUN_0054ffd0` | 2,368, 1,584, 2,656, 512 | Format 1 converters. Unreachable for the shipped data. Leave out |

### Screens

| Function | Size | Note |
|---|---|---|
| `FUN_0081ce00` | 515 | Start New Game action |
| Build 0x81d010, input 0x81dc30 | 3,094, 3,230 | "Select League(s)". The `DAT_00dd7118` != 0 branches (about a third) are for adding a manager to a running game and can wait |
| Build 0x826d20, input 0x826e60 | 320, 190 | "No Memory". Can be left out |
| `FUN_00669f50` | 368 | Extra division text |
| `FUN_0066a0c0` + comparator 0x66a100 | 53 + 379 | League order |

### Helpers (text, lists, scrollbars, popups, keyboard)

| Function | Size | Note |
|---|---|---|
| `FUN_00802f80` | 928 | Opcode 7. The local part is a call of `FUN_005e76f0`. The rest is network |
| `FUN_005e76f0` | 336 | Progress and message box |
| `FUN_005e8290` | 1,552 | Error box. Needed for "Not enough memory" and the fatal errors |
| `FUN_00803320`, `FUN_004032e0`, `FUN_00403320` | 112, 64, 48 | Get and set the scroll position |
| `FUN_00561f10` | 736 | Scrollbar arrow and track click |
| `FUN_00561050` | 448 | Scrollbar thumb drag (mode 2) |
| `FUN_00403ee0` | 752 | Scroll an area by n rows |
| `FUN_00803390`, `FUN_005621f0` | 48, 176 | Wheel and arrow keys warp into the scroll zone (input inventory, 2.5) |
| `FUN_0090c1d0` | 256 | Name compare. Needs the 768-byte table at 0xaa2154 |
| `FUN_00945333` | 340 | MSVC `qsort`. The host `qsort` gives the same order, because no two entries compare equal |

No popup and no text entry is on this path.

### Game state

| Function | Size | Note |
|---|---|---|
| `FUN_00667150` | 8,176, large | League table. Repetitive: 35 blocks of the same shape |
| `FUN_00827d50` | 960 | Default league by language |
| `FUN_005d8480` | 1,728 | Settings apply and `game.cfg` write |
| `FUN_007a1710` | 64 | Free memory |
| `FUN_00828110` | 10,416, large | Game creation, after this screen |

## 5. Language and text functions

All fixed texts on the path go through the plain path of `FUN_0066f4e0` when `DAT_009eb2dc` is -1 or 0:

- "Loading database", "Championship Manager 2001/02", "Select League(s)", "Selected League(s)", "  Use Real Players:", "Yes", "No", "  Attribute Masking:", "Select All", "De-Select All", "SELECTED", "BACKGROUND", "Regional Divisions" and the other column 6 texts, "Not enough memory", "Not enough memory to select that many leagues", "Back", "Next", the side bar texts.
- No format contains `{`, `[`, `}` or `]`. One format uses the `<` rule: "Conference Division<%s - COMMENT - English Conference>" becomes "Conference Division%s", and the argument is "" (0xad9c64). Result "Conference Division".
- The two leading spaces of "  Use Real Players:" and "  Attribute Masking:" are copied by the leading-space rule.
- The nation names use `sprintf("  %s", name)` (`FUN_00944c9f`, 0x81d9b9). They do not pass the text function, so no tag pass and no translation applies to them.
- Error box headings use `sprintf("v%s %s.%s %d", ...)` and `FUN_00944cff` (`_splitpath`, inference).

`.lng` files on the path: the loader copies all ten to `CM3_TEMP` (step 2b.3). It reads none for the shipped index. `FUN_005339a0` reads `CM3_TEMP\<lang>.lng` only when the settings language differs from `DAT_009b5984` (0 after the load). For settings language 0 nothing is read. For 1 (English US) `eng.lng` is read (jump table 0x5358e0, case 1 → 0x533a50, "eng.lng" 0x9b632c). The settings language value on a plain start is unknown.

`language.ldb` is not read on the path unless `FUN_005d8480` sees a language change (0x5d858f).

## 6. Settings and files on the path

| Item | Read or write | Where |
|---|---|---|
| `Data\index.dat` and the 20 .dat files | Read | `FUN_00528540` |
| `Data\*.lng` | Read, copied to `CM3_TEMP` | 0x52861f..0x5286d2 |
| `CM3_TEMP\<lang>.lng` | Existence check | `FUN_00535930` |
| `duff.out` (current folder) | Deleted | 0x52d205 |
| `CM3_PREFS\game.cfg` | Written | `FUN_005d8480`, 0x5d87ea |
| Command line `-dontcheckram` | Read | 0x81dfeb, turns off the league memory check |
| Settings object `[DAT_00ae3138]` | +0x08 language read | 0x5286da, 0x5d8580 |
| `DAT_009eb2dc` (language index from the keyboard layout and `language.ldb`) | Read | `FUN_00827d50`, `FUN_0090c1d0` |
| `DAT_009b5981` real players, `DAT_009cc270` attribute masking | Read and written | Build and input. 1 and 1 in the file. No other direct byte write found. A block copy was not searched for |
| `DAT_009cd1fc` | Read, 1 in the file | Memory check on |
| `DAT_00b74348` | Read, 0 on a plain start | Written to 1 only at 0x7a4323 (when a command code is 0x80, in the function around 0x7a431b, purpose unknown) |
| `GlobalMemoryStatus` | Read | `FUN_007a1710` |

No registry call appears in the functions read on this path. `FUN_0090bc50`, which `FUN_0090bde0` calls to find a folder, was not read.

## 7. Proposed slice order

Each slice ends with a test against the original. Line counts are estimates for the port.

1. **Nation table only** (about 250 lines). Read `index.dat`, find entry 4, read `nation.dat` (raw 290-byte records) and `continent.dat`. Struct with ids, not pointers. Test: count 213, row 61 is "England", flag bytes 0. Uses only `index.dat` and two readers of `FUN_00528540`.
2. **League table and default** (about 250 lines). The 35 entries of `FUN_00667150` with the fields +0x00, +0x04 and +0x08 only (+0x08 as a "playable" flag). The 27 names matched to nation rows as `FUN_0060d810` does. `FUN_00827d50` for the language. `FUN_0066a0c0` and `FUN_0090c1d0`, with the table at 0xaa2154 read from the user's exe or built from its rule. `FUN_00669f50`. Test: order array and default flags equal the original's after the load.
3. **The screen** (about 450 lines). `FUN_0081ce00` (database path, no "No Memory" screen), build 0x81d010 and input 0x81dc30 for `DAT_00dd7118` == 0, the progress box `FUN_005e76f0`, and a minimal error box for "Not enough memory". Free memory as a fixed large value. Test: a differential test of the widget list after the build, and of the counters after each id.
4. **Scrolling** (about 400 lines). `FUN_00403320`, `FUN_004032e0`, `FUN_00803320`, `FUN_00403ee0`, `FUN_00561f10`, `FUN_00561050`, wheel and keys. Without this slice only rows 0..15 can be reached.
5. **Full loader** (about 1,500 lines, large). All 22 entries, staff conversion, reserve slots, `FUN_005315a0` as id checks, the staff pass, the rest of the key ids. Needed by game creation, not by the first screen.
6. **Settings and names** (later). `FUN_005d8480` and `game.cfg`, `FUN_005339a0` and the `.lng` format.

Slices 1 to 3 give the first data screen with real data. Slice 4 makes the list complete.

## 8. Open doubts

- The value of `DAT_009eb2dc` and of the settings language `[DAT_00ae3138]+8` on a plain start. They select the default nation and whether `eng.lng` is read. Both English values (0 and 1) give England.
- What `FUN_005339a0` changes when it loads `eng.lng` (setting 1) into the English database.
- `FUN_005d8480` writes `game.cfg` on every Start New Game. The content and the conditions were not read in full.
- On a modern system `GlobalMemoryStatus` caps its values. The "Select All" limit (`mem7` >= 16) and the league limit depend on it. The port needs a fixed value. It is a spec decision.
- Entries 0..7 of the league table have no nation. Inference: world and continental competitions. Their +0x04 continents are entries 3..7. Not checked.
- A key id of -1 would give a nation pointer `base - 0x122`, which is not 0, so the screen would show a bad row. The shipped data has all 27 names. A port with other data must check.
- The meaning of `DAT_00b74348` and of `DAT_00a15704`.
- The fourth argument (channel) of the progress calls in the loader's read loop.
- Whether `DAT_009b5981` and `DAT_009cc270` are loaded from a file at start-up through a block copy.
- The functions at league entry +0x08 and the meaning of the fields at +0x15..+0x39 were not read.
- The resolve pass `FUN_005315a0` was read only for the nation part and the error texts. The club and staff parts need their own read before slice 5.
- `FUN_00828110` (after Next with one league) may load the database again. The earlier inventory lists it as a caller of `FUN_00524d50`. Not traced.
