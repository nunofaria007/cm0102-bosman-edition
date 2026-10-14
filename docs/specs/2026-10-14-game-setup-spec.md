# Game setup
Status: In build

## Problem
"Next" on "Select League(s)" only rebuilds the screen. The port reads the nation table and nothing else of the database, has no random numbers, no dates and no text entry, so no manager can be set up.

## Solution
Port the full database load of the original, its dates and random numbers, text entry, and the screens that follow "Select League(s)": "Enter Name", "Select Nationality", "Select Team", "Please Confirm" and "Manager Status". A stub stands in for game creation: it sets only the manager rows. The real game creation, which runs the game engine for 194 days, comes in later specs. The facts are in `docs/research/2026-10-14-game-creation-inventory.md`.

## User stories
1. As a player, I want "Next" on "Select League(s)" to lead to "Enter Name", "Select Nationality", "Select Team", "Please Confirm" and "Manager Status", as in the original.
2. As a player, I want to type my first name, second name and an optional password.
3. As a player, I want the nations and teams of the real database in the lists.
4. As a developer of the port, I want every table of the database in memory as in the original, field for field, so that the engine specs can build on it.
5. As a developer of the port, I want the dates and random numbers of the original, so that the engine makes the same choices.

## Decisions
- The spec covers the full database load `FUN_00528540` (all 22 entries of `index.dat`, the reserve rows, the 157 to 110 byte staff conversion, the preferences table, the id-to-pointer pass `FUN_005315a0`, the staff pass, the national clubs and the key ids `FUN_00601a40`), the date functions, the random generator, text entry `FUN_005617c0` with the keyboard it needs, and the five setup screens.
- A stub replaces game creation `FUN_00828110`. It sets the manager rows only. "Finish" on "Manager Status" rebuilds that screen until the engine specs port game creation and the run to the start date.
- The CD check of game creation is left out.
- The random generator draws from a table of 51,000 numbers inside `cm0102.exe`. The port reads that table from the player's own `cm0102.exe` at start: `--exe <path>`, by default `cm0102.exe` next to the data folder. The repo holds none of it.
- Game history files (`.tmp` in the original) live in `~/Library/Application Support/cm0102`.
- The two NULL bugs of the id pass and the common-name bug of the staff pass (inventory, sections 2c and 2d) are kept where they change data, and recorded.
- Pointers between records are real pointers in the port. The differential test turns each pointer of both programs into a row index and compares the tables byte for byte otherwise.
- Three agents build at once, each in its own worktree: dates and random numbers, the database load, and text entry. One agent then builds the setup screens. One review pair runs at the end.
- Commits are local. Nothing is pushed.
- Database load (slice 2). Each table is a native struct. Until the id pass a pointer field holds its id. The port keeps the two ids that the id pass leaves in pointer fields: club +0x5b above the club comp count (it clears +0x60) and stadium +0x48 above the stadium count (it clears +0x38).
- Database load, left out:
  - The copy of the .lng files and the language check `FUN_00535930` (0x52861f..0x5286fc).
  - The language block of a newer index.dat (0x528848) and the eng.lng names of the key ids (0x603c52). The port fails the load when index.dat has the extra byte.
  - The non-shared sizing of a saved game (0x5289c7) and the reads from a stream.
  - The format-1 converters of the club, player, non-player and club comp readers (`FUN_0054dc90`, `FUN_0054f150`, `FUN_0054e9a0`, `FUN_0054ffd0`) and the 110-byte staff reader `FUN_0054e630`. The shipped data does not use them. The port fails the load there.
  - The error boxes and log lines of the readers, the id pass and the key id checks (0x6043ac..0x60ba6e), and `FUN_0053dba0`. The delete of duff.out (0x52d205).
  - `FUN_00540a50` after 0x540b1b: the reserve team lookup over about 500 key club ids for a club of Spain, Germany or Portugal. The load calls it with need_league 1, and every club of the files has +0x244 = 0, so the load never reaches it. The port returns 0 there.
- The staff pass calls `FUN_0090cfc0` for a staff row with no nation, no club and no national club, and for a missing name when no common name is empty. The shipped data reaches neither (the test checks it). Until slice 1 merges, a stand-in in `src/cm3/database_links.c` returns 0.
- `FUN_0053b4f0` and `FUN_0053b3d0` return NULL for a club without a nation, and the original then reads through NULL. The port skips these clubs.
- The database test also loads a changed copy of the shipped data in a temp folder, so that it reaches the branches of the id pass and the staff pass that the shipped data does not. In its byte compare a pointer becomes its row, a pointer into another table 0x80000000 | table << 24 | row, and an id that the id pass keeps 0x40000000 | id.
- The key ids are 1,284 globals: 0x9cf178..0x9d0524 without 0x9d0300 and 0x9d034c (never used), and 0xb63c9c..0xb63d00. `tools/research/gen_key_ids.py` reads their name chains from the original into `src/cm3/key_ids_table.c`.

## Testing
One seam: the differential test, with the tracer harness of the inventory moved into `tests/diff`.
- Database: run `FUN_00528540` in the emulator and the port's load, turn every pointer into a row index, and compare every table.
- Dates and random numbers: many dates and weekdays, and long runs of random numbers from the same state, with the three state values compared after.
- Text entry: the same widgets, keys and characters on both sides, then the text, the cursor and the screen.
- Setup screens: the widgets after each build and the manager rows after each click, against the original.
- Manual check: from "Start New Game" to "Manager Status" with a typed name and a chosen team.

## Out of scope
- Game creation itself and the game engine.
- The news screen.
- Saved games.

## Open decisions

## Slices
### 0. Harness
Blocked by: none
Delivers: the stubs of the tracer in `tests/diff/original.py` (a larger heap, import stubs, a file layer in a temp folder, `realloc` in place), and a function that turns a pointer of the original into a row index.
Merge danger: two-way door. Blast radius: the test harness only.
- [x] The load of the original runs in the emulator and gives the counts of the inventory, section 2b.

### 1. Dates and random numbers
Blocked by: none
Delivers: the date functions, `FUN_0090cfc0`, `FUN_0090d0a0`, MSVC `rand` and `srand`, and the table read from `cm0102.exe`.
Merge danger: two-way door. Blast radius: every later random choice and date.
- [ ] Dates, weekdays and 100,000 random numbers equal the original, with the same state after.

### 2. Database load
Blocked by: 0
Delivers: the loader, the readers, the reserve rows, the conversion, the preferences table, the id pass, the staff pass, the national clubs and the key ids.
Merge danger: two-way door. Blast radius: all data of the game.
- [x] Every table equals the original after the load.
- [x] The about 1,100 key id globals equal the original.

### 3. Text entry
Blocked by: none
Delivers: `FUN_005617c0` with the key and character input of the window procedure.
Merge danger: two-way door. Blast radius: every text field.
- [ ] Random keys and characters on text widgets give the same text, cursor and screen as the original.

### 4. Setup screens
Blocked by: 1, 2, 3
Delivers: the five screens, the side bar ids they use, the game creation stub, and the link from "Next" on "Select League(s)".
Merge danger: two-way door. Blast radius: the path from "Select League(s)".
- [ ] Each build and click equals the original.
- [ ] Manual check: from "Start New Game" to "Manager Status".
- [ ] The full differential test passes.
