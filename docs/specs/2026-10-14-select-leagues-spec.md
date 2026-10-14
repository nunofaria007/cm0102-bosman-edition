# Select League(s)
Status: In build

## Problem
"Start New Game" on the start screen only rebuilds the screen. The port reads no data of the original, and its lists cannot scroll.

## Solution
Port the path from "Start New Game" to the first screen with real data: the "Loading database" progress bar, the nation table of `nation.dat`, the league table of the original, and the "Select League(s)" screen with its scrolling list of 27 nations. Port the scrollbar input that the list needs. The facts are in `docs/research/2026-10-14-start-new-game-inventory.md` and `docs/research/2026-10-13-input-inventory.md`.

## User stories
1. As a player, I want "Start New Game" to show the "Loading database" progress bar and then "Select League(s)", as in the original.
2. As a player, I want the list of 27 nations in the order of the original, with England selected at the start.
3. As a player, I want "SELECTED", "BACKGROUND", the extra-division buttons, "Select All" and "De-Select All" to work as in the original.
4. As a player, I want to scroll the list with the arrows, the track, the thumb, the arrow keys, Page Up, Page Down and the wheel, as in the original.
5. As a developer of the port, I want the nation records as native structs whose fields match the original, so that later screens can read them.
6. As a developer of the port, I want the scrollbar input in the screen manager, so that every later list scrolls.

## Decisions
- The spec covers the nation table, the continent table as far as the sort needs it, the league table of `FUN_00667150`, the default selection by language, the progress bar `FUN_005e76f0`, "Start New Game" `FUN_0081ce00`, the build and input of "Select League(s)" for a new game, and the scrollbar input.
- The full loader `FUN_00528540` (staff, clubs and the rest) waits for the spec that creates the game.
- The free-memory check and its "No Memory" screen are left out. The port acts as if memory is enough.
- Records are native C structs with the offsets of the original in comments. The original overwrites id fields with 32-bit pointers after the load (`FUN_005315a0`). The port keeps real pointer fields that a resolve step fills. The differential test compares the id behind each pointer.
- The port shows the "Loading database" progress bar with its 46 steps.
- The load leaves out its side effects: the copy of the `.lng` files to a temp folder, the delete of `duff.out` and the write of `game.cfg`.
- The language is 0, English. England is then selected at the start.
- The scrollbar input covers the click on the arrows, the track and the thumb (`FUN_00561f10`), the thumb drag (mode 2, `FUN_00561050`), the scroll (`FUN_00403ee0`) and the scroll position functions, and the keys and the wheel through the simulated click of the original (`FUN_005621f0`). Popup areas and text entry stay for a later spec.
- Small fixed tables of the original that the code needs, such as the 768-byte name compare table at 0xaa2154, are copied into the source, as the glyph tables were.
- The `DAT_00dd7118` != 0 branches of "Select League(s)" (adding a manager to a running game) are left out.
- The buttons that lead past "Select League(s)" rebuild the screen until game creation is ported.
- Two agents build slices 1 and 2 in parallel, each in its own worktree. One agent then builds slice 3. One review pair runs at the end.
- Commits are local. Nothing is pushed.
- Slice 1 left these parts out:
  - The dates (+0x15, +0x2a, +0x39, through `FUN_00549f70` and `FUN_00417c10`) and the club comp pointers (+0x1e, +0x22) of the league entries of `FUN_00667150`. They need the club comp table and the date code.
  - Every key id of `FUN_00601a40` except the 27 nations of `FUN_0060d810` and the 6 continents of the loop at 0x604278.
  - The links of a nation to city (+0x7a), stadium (+0x80) and colours (+0x90..+0xa4). The port keeps these ids until the full loader reads their tables.
  - The language block of `index.dat` (0x528848..0x52896f). The shipped file has none. The port fails the load when the byte before it is not 0.
- Where the original makes a pointer outside a table, the port gives NULL: a link id at or above the record count (`FUN_005315a0` checks only for a negative id), and a key nation or continent with id -1 in the league table and in `FUN_00827d50`. The shipped data has neither.
- The port keeps the language index `DAT_009eb2dc` at 0. The name compare `FUN_0090c1d0` has no assert string. It goes to `src/si/utils.c` next to `FUN_0090bde0` (inference).
- The progress box `FUN_005e76f0` is `gui_progress_box` and the local part of opcode 7 `FUN_00802f80` is `screen_stack_progress`, in `src/si/progress_box.c`. `include/progress_box.h` lists the 47 calls of the load (steps 0 to 46, total 46). The darken before step 0, `FUN_0055f2c0`, is `scrman_darken_once`.
- The progress box leaves out the check of `DAT_00aebe0c` (display off, 0x5e76f0). The port has no such switch.
- The progress box runs `g_screen_pump` where the original runs its message pump `FUN_0068d520`, and ignores its result. A closed window ends the main loop at its next pump, after the load.
- Opcode 7 leaves out the packet buffer at S+0x37fe and its read back (`FUN_005601e0`, `FUN_00564080`), the network send `FUN_0077ac30`, and the cursor call `FUN_005e8170(1)` at 0x567073. That call runs only when `DAT_00b77c8c` is not 0, and nothing writes it.
- Channels 1 to 15 have no records in a local game, and their record word is 0. The original then applies opcode 7 to the local screen manager too (0x80317b), and the port does the same. A channel outside -1 to 15 does nothing in the port. The original reads past its records there.
- The original fails on opcode 7 with a NULL text (`strlen` at 0x803123). The port does nothing.
- The fourth argument of the progress calls in the two loops and the last call of the loader is a register of the loader. Its value is unknown. Every channel from -1 to 15 gives the same box for one local player, so the port's load can pass -1.
- The key and wheel scrollbar `DAT_00ad9c58` points to the scrollbar struct of an area (area +0xb7a). The scrollbar setup `FUN_00403600` sets it, so it is the scrollbar of the last area laid out with one. The area release `FUN_004031a0` clears it when it points to that area. The port keeps it as `g_key_scrollbar`.
- The scroll `FUN_00403ee0` ends with a write for network players (`FUN_00800330`, net buffer `DAT_00ae28f8`). The port leaves it out. Without room in that buffer, the original shows an error box and lets the button go.
- The second pass of the hit test only walks the rows again (area +0xb78), as text entry is not ported.
- An SDL wheel event with y = 0 (part of a notch) does nothing. In the original, a wheel delta under 120 gives `FUN_005621f0(0, 0)`, which holds the button until the next real button-up.
- The port does not keep the key code `DAT_00b67a04`, as text entry is not ported. Keys Left and Right do nothing, as in the original.
- `FUN_00803320` uses the record of channel `+0x382a`. The port has one channel.
- The differential test stubs the text timers `FUN_00569610` and the cursor `FUN_005e8170` to run the poll `FUN_0055f620` of the original. The window procedure and the pump have no differential test. The test models the countdown of the pump from the assembly.
- Slice 3 left these parts out:
  - The free memory `FUN_007a1710` in all its uses. The port acts as if memory is enough. "Select All" is then a button when fewer than 15 nations are selected (the original also needs free MB / 7 >= 16, 0x81d5e6). The input adds a nation with no memory check and no "Not enough memory" box (0x81dfc2, 0x81e17e, 0x81e309, 0x81e517).
  - The settings apply `FUN_005d8480`. For language 0 it reloads nothing.
  - The writes of `DAT_00dd7124` (0 in the build at 0x81d0b2, 1 before game creation at 0x81e708) and of `DAT_00a15704` (1 at 0x81cff2, already 1 in the file). Their meaning is unknown.
- "Next" with one or more selected nations returns -4 (rebuild). The original pushes "Select Start Season" (more than one) or creates the game (one), and returns -4 too. With none, both return 0.
- A failed load or league table makes "Start New Game" return SCREEN_QUIT after a line on stderr. The original shows an error box and calls `exit(-1)`.
- The build ports the second options area of 0x81d33e: when the first options area has index 0, the original makes another area for "Attribute Masking". A real screen never has index 0 there, as the side bar comes first.
- The original reads a NULL nation for an injected id 0xc, 0xd, 0xe or 0x38 with no clicked widget. The port returns -11 and changes nothing.
- The port's load (`database_load`) makes the 47 progress calls of the loader and reads the two tables after the read steps. The reads draw nothing, so the order does not show.
- The differential test of "Start New Game" models the loader `FUN_00524d50` with a small x86 stub: 47 calls of opcode 7 with "Loading database", steps 0 to 46 of 46, channel -1, then the nation table of the original readers is put in place. It stubs `FUN_007a1710` with 8190 MB (the most that `GlobalMemoryStatus` gives) and `FUN_005d8480` with a return. `FUN_00827d50` and `FUN_00667150` run for real. "Next" with a selected nation is checked on the port only.

## Testing
One seam: the differential test.
- Nation table: the port and the original each load `nation.dat` and `continent.dat`, and the test compares every field of every record.
- League table: the order and the default flags after the load equal the original.
- Scrollbar input: the same area with more rows than fit, the same mouse and key states, then the scroll position, the area fields, the screen and the flips.
- "Select League(s)": the same build, then every area and widget and the screen. Then each click and its result, and the counters after it.
- Manual check: start the app, click "Start New Game", see the progress bar and the list, scroll to the end, and select and clear nations.

## Out of scope
- The full database and game creation.
- Popup areas, text entry.
- `.lng` files, `game.cfg`, settings.
- Adding a manager to a running game.

## Open decisions

## Slices
### 1. Nation and league tables
Blocked by: none
Delivers: `index.dat`, the nation and continent tables, the league table, the default selection for English, the league order and the extra-division text.
Merge danger: two-way door. Blast radius: the data of every later screen.
- [x] Every field of every nation record equals the original.
- [x] The league order and default flags equal the original.

### 2. Scrollbar input
Blocked by: none
Delivers: scrollbar click, thumb drag, scroll, scroll position, keys and wheel.
Merge danger: two-way door. Blast radius: every list.
- [x] Random clicks, drags and keys on areas with scrollbars give the same state, screen and flips as the original.

### 3. "Select League(s)"
Blocked by: 1, 2
Delivers: the progress bar, "Start New Game", the build and input of "Select League(s)", and the link from the start screen.
Merge danger: two-way door. Blast radius: the path from the start screen.
- [x] The build and each click give the same screen manager, screen and counters as the original.
- [ ] Manual check: the list shows 27 nations, scrolls to the end, and selects and clears.
- [x] The full differential test passes.
