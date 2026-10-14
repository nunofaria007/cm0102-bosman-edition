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
- [ ] Every table equals the original after the load.
- [ ] The about 1,100 key id globals equal the original.

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
