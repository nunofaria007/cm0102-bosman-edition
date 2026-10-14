# Screen stack and the start screen
Status: Shipped 2026-10-14

## Problem
The port draws areas and widgets through the screen manager, but no screen of the original exists in it. The app shows a made-up demo. Nothing reacts to the mouse, and nothing moves from one screen to the next.

## Solution
Port the screen stack of scrman.cpp for one local player, its main loop on SDL, mouse clicks on buttons, and the first screen of the original: "Setup Game". The app starts on that screen with no arguments. A click on a menu item that has no screen in the port yet rebuilds the start screen. "Exit Game" quits.

## User stories
1. As a player, I want the app to open on the "Setup Game" screen of the original, with its heading, side bar, menu and Back and Next bar.
2. As a player, I want a random picture from the Pictures folder behind the start screen, as in the original.
3. As a player, I want a button to react to a click as in the original, and "Exit Game" to quit.
4. As a developer of the port, I want push, remove, title and slots of the screen stack to behave as in the original, so that later screens can push and pop each other.
5. As a developer of the port, I want the main loop to rebuild, draw, poll and act on the input result in the order of the original.
6. As a developer of the port, I want the build function of a screen to give the same areas and widgets as in the original, field for field.
7. As a developer of the port, I want the hit test to return the same widget as the original for the same mouse state.
8. As a developer of the port, I want each ported helper for menus, headings and button bars to be usable by later screens.

## Decisions
- The work covers one local player. The screen stack has one channel with one record. Channels 1 to 15, other records, network packets and the session object are left out.
- The screen stack keeps the node layout of the original: build, leave, input and exit functions, a flag, 60 slots of {value, owned}, previous and next, and a 260-byte title. Push sets the title to a backslash.
- The screen draw takes its background name from the title of the current node, as the original does. The backslash that `DAT_00dd6bda` forces is written into the title.
- The main loop runs on SDL. One pass: rebuild when the record asks for it (reset, build with the draw off, screen draw), pump the events, poll the input, call the input function with the clicked widget or -1, act on the result. The results -1 to -13 act as in the original. -7 ends the loop.
- Input covers mode 0 of the original: the mouse position and the left button, the hit test and a press and release on a widget. The keyboard, popup areas, the scrollbar drag, the widget drag, text entry and the text timer are left out.
- The window procedure of the original sets the mouse globals. The port sets the same values from SDL events.
- The text function `FUN_0066f4e0` is ported for plain text, `%s` and `%d`. Its `{...}` and `[...]` tags are left out.
- The background picker `FUN_0041bfb0` picks a random picture from the "England" list of `Pictures/pics.cfg`, read as the original reads it. The original draws its random numbers from a pool in its data, seeded from the clock. The port uses its own random numbers. The app takes the folder from `--pictures`, with `Pictures` next to the data folder as the default. In the differential test both programs get the same title.
- The menu items "Start New Game", "Quick Start Game", "Restore Saved Game", "Delete Saved Game", "Network Play", "Game Settings", "Hall Of Fame", "Game Credits" and "Web Sites" rebuild the start screen. The side bar button "Exit Game" opens the "Please Confirm" screen of the original, and its "Yes" returns `SCREEN_QUIT`, where the original calls `exit(0)`. (Research of 2026-10-14 found the confirm screen. The interview assumed that Exit Game quits at once.)
- The side bar shows "Version\n3.9.68", as the original does.
- The command-line loads of the original (`-load`, `quick_start.cfg`, `auto_setup.cfg`) are left out.
- The app with no arguments opens the start screen. `--demo` keeps the made-up screen. Without `--data`, the app takes `Data` in the current folder, or else the first `Data` folder with the fonts above the folder of the app. The original takes its folder from the registry.
- The original has no quit key. Escape ends the demo only. Closing the window ends the main loop.
- A quick tap gives the press and the release of the button in one pump pass. The pump of the port stops after a press, so the poll sees the button held once. The original would miss such a tap.
- The review of 2026-10-14 found four differences in the main loop that no ported screen reaches, and the port now follows the original in each: the result starts at 0 in each pass; a continue with flagged screens but none that fits sets the result to 0; -10 with no flagged chain frees nothing and calls the input function with -1; the redraw after a click also sets 0x12f551 and clears the darken flag.
- The text function takes the size of its buffer. The original has no limit.
- Commits are local. Nothing is pushed.
- The speed rules of 2026-10-13 apply: the break tool after each slice, one review at the end of the spec, and tests for what a real screen can reach.

## Testing
One seam: the differential test.
- Screen stack: the same sequence of push, remove, set and get of slots and titles on the screen stack of the original (the global at 0xb74450, channel 0) and of the port. The test compares the record and every node.
- Helpers and the start screen: the same build on both sides, then every field of the screen manager, its areas and widgets, and the screen after the screen draw.
- Hit test: the same screen and the same mouse globals on both sides, then the clicked widget and the fields of the screen manager.
- The main loop waits for Windows messages in the original, so it has no differential case. Its check is manual: start the app, see the start screen, click items, and quit with "Exit Game".

Found while building slice 1:
- The hit test of the port leaves out three paths of the original: text entry (widget flag 0x08), the scrollbar click, and the opening of a popup area. A widget with a popup area acts as a plain button, and the rows of an area past those in view are not tested, since only an area with a scrollbar has them. The start screen uses none of these.
- An area with no widget makes the original read a widget before its array in the hit test. The port reads the field before the list, as the original does, but no test case has such an area.
- The hit test gives the widget under the mouse on every frame while the button is held, as in the original.
- The node keeps a slot value as a pointer-sized number, because a slot holds a number or a pointer. The original keeps 32 bits.
- With no current screen, the original ends the program in set slot, get slot and the screen queries. The port returns without effect, 0 or 1.

Found while building slice 2:
- The screens and helpers work on one global screen stack, `g_screen_stack`, as the original works on the object at 0xb74450. The app must run the main loop on it.
- The text function keeps `{`, `[`, `}` and `]` as plain characters. The original drops an empty tag, skips one argument for each `{` or `[`, and replaces a tag with content in a later pass (0x66f93e). The port writes at most 2047 characters, where the original has no limit. A `<` note with a space and no `>` after it makes the original read past the end of the format. The port stops at the end.
- The text box `FUN_005ed070` is ported for a text that fits on one line. The line breaking of `FUN_004aeaf0` (comp_text.cpp) and the substitutions after the text are left out. The confirm screen reaches neither.
- The menu builder returns 0 for 0 columns, where the original divides by zero.
- `FUN_00804030` (a button with its own box) is ported with the other button helpers, though the start screen does not use it.

Found while building slice 3:
- "Yes" on the "Please Confirm" screen of "Exit Game" returns `SCREEN_QUIT` in the port. The original calls its shut-down code and `exit(0)` inside the side bar input function (0x762c4d), so its main loop never sees a result.
- "Restart Game" also opens "Please Confirm" (code 0x3d). Its "Yes" makes the original start a new copy of itself with `WinExec` of its command line and end (`FUN_0068dbc0`). The port frees the screen stack, pushes the start screen and returns `SCREEN_REBUILD`.
- The original opens `pics.cfg` in text mode, so the MSVC library turns CR LF into LF and ends the file at the byte 0x1a. The port reads it the same way. The research of 2026-10-14 assumed that the CR stays on the last name of a line: it does not, so "holywell03.RGN" stays in the list.
- `FUN_00591840` takes 7 arguments: title, heading, text, buttons, text colour, colour and code (0x764396..0x7643ab). The research listed 8. The side bar passes light grey and red, and the build gives the heading the red panel and the light grey text.
- A "{" ends a name and puts that name in the second list (`names2`), and the flag holds until a name is kept: "{x}" puts x in the second list (0x41b61b). The port reads names this way.
- The port loads `pics.cfg` on the first background pick when the app has not loaded it. The original loads it once at start-up (0x5d7a98). `pics_free` empties the table, for the test.
- The side bar has the mode 1 part only: no manager record. Not ported, with the address: "Add Manager" as a button (0x75de4a, needs `DAT_00dd711c`), "Chat" (0x75dea1, network), the arrows in font 0 for a font setting other than -1 (0x75dd36), modes 2 to 4 (0x75e04b..0x7611ff), and the hook `FUN_00823930` for id 0x423 (the port sets the id and no function).
- The side bar input has the answers of codes 0xb and 0x3d, the ids -2, -3, 0x402 and 0x42f, and -11 for id 0x424. Not ported: the codes 1, 7, 8, 9, 0xa, 0x2a, 0x37, 0x3a, 0x3e, 0x43 and 0x44 (0x7623a6..0x762915, from screens the port lacks), the second dialogs when a flagged screen is open (0x762946, 0x762b20), the manager record parts (0x761654, 0x76222a), and the ids from 0x3e8 to 0x434 that need a game. `FUN_007643d0` returns 0 for every id that a ported screen has, so the port returns 0.
- The confirm screen with no text lists an object from slot 6 (`FUN_005ed200`). Only `FUN_005919b0` pushes such an object, and nothing ported calls it, so it is left out. Its keys (`DAT_00b67a24`, jump table at 0x591f00) are left out with the keyboard.
- The start screen input leaves out the actions of the nine menu items, the "Game Settings" screen of a first run (`FUN_00823890`, slot 0), the command-line loads and the dialog codes 4, 5 and 6. The test leaves the menu items out of its clicks.
- "Network Play" keeps id 5 and the light grey text: the port has no `-nonetwork` option (`DAT_00a801c4` is 1 in the file).
- The original keeps the menu items on its stack and never sets their data (widget +0x48). The port sets 0, and the test leaves that field out of the compare for the menu buttons.
- The picture cache of the original (0xae291c) points into the emulated heap, which the test empties between cases. The test clears it in the original. The port keeps its cache, which holds the same picture.

## Out of scope
- The screens behind the menu items.
- Keyboard, popup areas, scrollbars, text entry, widget drag, the idle timeout.
- Network play and hot-seat managers.
- Language files and the tags of the text function.
- The database.

## Open decisions

## Slices
### 1. Screen stack
Blocked by: none
Delivers: push, remove, free all, set and get of slots, set and get of the title, the default screen and the small queries, for one channel. The screen draw reads its background name from the title.
Merge danger: two-way door. Blast radius: none for the app until slice 4.
- [x] Random sequences of stack calls give the same record and nodes as the original.

### 2. Helpers
Blocked by: none
Delivers: the text function for plain text, `%s` and `%d`, the heading and sub-heading helpers, the button helpers, the menu builder and the Back and Next bar.
Merge danger: two-way door. Blast radius: menus and headings of every later screen.
- [x] Each helper gives the same areas, widgets and pixels as the original for random arguments.

### 3. Start screen build
Blocked by: 1, 2
Delivers: the build function of "Setup Game" with the background picker and the side bar on the start screen.
Merge danger: two-way door. Blast radius: the start screen.
- [x] The build gives the same screen manager and the same screen as the original.

### 4. Input and the main loop
Blocked by: 3
Delivers: the mouse globals from SDL, the poll and the mode 0 hit test, the main loop, the input function of the start screen, and the app start on the start screen.
Merge danger: two-way door. Blast radius: the app start.
- [x] The hit test returns the same widget as the original for random mouse states.
- [x] The app opens on the start screen, a menu item rebuilds it, and "Exit Game" quits. Manual check by Max, 2026-10-14.
- [x] The full differential test passes.
