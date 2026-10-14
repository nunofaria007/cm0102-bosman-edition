# Screen stack and the start screen
Status: In build

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
- The background picker `FUN_0041bfb0` picks a random RGN file from the Pictures folder. The app takes the folder from `--pictures`, with `Pictures` next to the data folder as the default. In the differential test both programs get the same picture.
- The menu items "Start New Game", "Quick Start Game", "Restore Saved Game", "Delete Saved Game", "Network Play", "Game Settings", "Hall Of Fame", "Game Credits" and "Web Sites" rebuild the start screen. The side bar button "Exit Game" quits.
- The version text of the side bar shows the label only. The arguments that the original formats there are not traced.
- The command-line loads of the original (`-load`, `quick_start.cfg`, `auto_setup.cfg`) are left out.
- The app with no arguments opens the start screen. `--demo` keeps the made-up screen.
- Commits are local. Nothing is pushed.
- The speed rules of 2026-10-13 apply: the break tool after each slice, one review at the end of the spec, and tests for what a real screen can reach.

## Testing
One seam: the differential test.
- Screen stack: the same sequence of push, remove, set and get of slots and titles on the screen stack of the original (the global at 0xb74450, channel 0) and of the port. The test compares the record and every node.
- Helpers and the start screen: the same build on both sides, then every field of the screen manager, its areas and widgets, and the screen after the screen draw.
- Hit test: the same screen and the same mouse globals on both sides, then the clicked widget and the fields of the screen manager.
- The main loop waits for Windows messages in the original, so it has no differential case. Its check is manual: start the app, see the start screen, click items, and quit with "Exit Game".

Found while building slice 1:
- The node keeps a slot value as a pointer-sized number, because a slot holds a number or a pointer. The original keeps 32 bits.
- With no current screen, the original ends the program in set slot, get slot and the screen queries. The port returns without effect, 0 or 1.

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
- [ ] Each helper gives the same areas, widgets and pixels as the original for random arguments.

### 3. Start screen build
Blocked by: 1, 2
Delivers: the build function of "Setup Game" with the background picker and the side bar on the start screen.
Merge danger: two-way door. Blast radius: the start screen.
- [ ] The build gives the same screen manager and the same screen as the original.

### 4. Input and the main loop
Blocked by: 3
Delivers: the mouse globals from SDL, the poll and the mode 0 hit test, the main loop, the input function of the start screen, and the app start on the start screen.
Merge danger: two-way door. Blast radius: the app start.
- [ ] The hit test returns the same widget as the original for random mouse states.
- [ ] The app opens on the start screen, a menu item rebuilds it, and "Exit Game" quits. Manual check.
- [ ] The full differential test passes.
