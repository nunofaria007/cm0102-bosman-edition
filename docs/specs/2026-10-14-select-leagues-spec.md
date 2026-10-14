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
- [ ] Every field of every nation record equals the original.
- [ ] The league order and default flags equal the original.

### 2. Scrollbar input
Blocked by: none
Delivers: scrollbar click, thumb drag, scroll, scroll position, keys and wheel.
Merge danger: two-way door. Blast radius: every list.
- [ ] Random clicks, drags and keys on areas with scrollbars give the same state, screen and flips as the original.

### 3. "Select League(s)"
Blocked by: 1, 2
Delivers: the progress bar, "Start New Game", the build and input of "Select League(s)", and the link from the start screen.
Merge danger: two-way door. Blast radius: the path from the start screen.
- [ ] The build and each click give the same screen manager, screen and counters as the original.
- [ ] Manual check: the list shows 27 nations, scrolls to the end, and selects and clears.
- [ ] The full differential test passes.
