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
- Slice 1 lives in `src/si/date.c` (Date.cpp: `FUN_00549f70`, `FUN_0054a130`, `FUN_0054a110`, `FUN_00417c10`, `FUN_0054a3f0`, `FUN_0054a2d0`, `FUN_0054c5b0`, `FUN_0054c8e0`, `FUN_0054c770`, `FUN_0054cab0`) and `src/si/random.c` (utils.cpp: `FUN_0090cfc0`, `FUN_0090d0a0`, MSVC `rand` 0x946a04 and `srand` 0x9469fa). The port keeps the table position as an index, and the end as -1 before the seed, where the original has a null pointer.
- The port checks the SHA256 of the whole exe before it reads the table at file offset 0x6a2454. A missing file or another hash stops the game with a message that names `--exe`. `--seed <n>` stands in for "-seed" of the original. Without it the seed is the time, as in `FUN_0081b890`.
- The large-count path of `FUN_0090cfc0` uses the x87 at the precision of the control word. The port uses doubles, which match 53-bit precision (control word 0x27f, the Windows default). The test sets 0x27f in the emulator. At 64-bit precision some large-count numbers differ: 2 of 160 runs of 1,000 numbers failed. That the game runs with 0x27f is an inference: it uses DirectDraw, and no code that changes the control word was read.
- Left out of slice 1: the error boxes of the date constructor (Date.cpp 0x34, 0x3b, 0x72, 0x79; the port clears `g_left_held` and corrects the value as the original does), and the other `srand` calls (0x686b34, 0x696b8c, 0x90de58, 0x90dfff), which belong to the engine specs. `pick_background` now draws from `random_number`.
- The leagues suite still replaces the date constructor of the original with a return, since the league table dates (`FUN_00667150`) are not ported. Slice 4 or the engine specs should port them with `date_init` and drop that stub.
- Text entry (slice 3) does not need the text timers `FUN_00569610`, so they are not ported. The cursor of a text entry does not blink in the original: `gui_text` draws it at the cursor place. The 100 ms timer swaps the two colours of a widget with flag 0x1000, and the 1 s timer turns the text of a widget with flag 0x800. Neither is a text entry flag.
- The hit test `FUN_00560490` has a second sweep of the rows after a text entry in a row finishes (`0x560adb`). It starts after the last area, so it never runs. The port leaves it out.
- The keys of the port map to the Windows key codes of a US keyboard. Alt and F10 give no key code: Windows sends them as WM_SYSKEYDOWN, which the window procedure does not read. SDL gives no text for Backspace, Tab, Enter and Escape, so the port sets their WM_CHAR character from the key. Ctrl with a letter gives no control character, and a typed character above U+00FF gives none. The original takes the character in the Windows code page. The port takes Latin-1, which is the same for 0xA0 to 0xFF.
- The error boxes of `FUN_005617c0` (a widget with no text entry flag) and `FUN_005eed50` (a NULL text) are left out. The port lets the button go, as the original does after the box. A text limit above 255 overruns a stack buffer in the original. The port keeps 255 characters.
- The picture branch of set text `FUN_005eed50` is ported but has no differential case: no text entry is a picture widget.
- Three widget fields and one screen manager field have names now: `text_limit` (widget +0x40), `text_changed` (+0x180), `text_source` is writable (+0x54), and `passed_focus` (screen manager +0x12f533).

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
- [ ] The load of the original runs in the emulator and gives the counts of the inventory, section 2b.

### 1. Dates and random numbers
Blocked by: none
Delivers: the date functions, `FUN_0090cfc0`, `FUN_0090d0a0`, MSVC `rand` and `srand`, and the table read from `cm0102.exe`.
Merge danger: two-way door. Blast radius: every later random choice and date.
- [x] Dates, weekdays and 100,000 random numbers equal the original, with the same state after.

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
- [x] Random keys and characters on text widgets give the same text, cursor and screen as the original.

### 4. Setup screens
Blocked by: 1, 2, 3
Delivers: the five screens, the side bar ids they use, the game creation stub, and the link from "Next" on "Select League(s)".
Merge danger: two-way door. Blast radius: the path from "Select League(s)".
- [ ] Each build and click equals the original.
- [ ] Manual check: from "Start New Game" to "Manager Status".
- [ ] The full differential test passes.
