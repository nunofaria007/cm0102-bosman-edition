# Screen manager
Status: In build

## Problem
The port can draw one area or one widget, but nothing puts them together. The demo fills the area and widget structs by hand and draws them itself. The 595 places where the original creates an area and the 1,296 places where it creates a widget have no function to call in the port, so no screen of the original can be ported.

## Solution
Port the functions of the original that build, draw and empty one screen manager: the constructor, the reset, area create, widget create, widget release and the screen draw. Check each one against the original with the differential test. Change the demo so that it builds its screen through these functions.

## User stories
1. As a developer of the port, I want an area create function with the arguments of the original, so that a ported build function can call it unchanged.
2. As a developer of the port, I want a widget create function with the 18 arguments of the original, so that a ported build function can call it unchanged.
3. As a developer of the port, I want every field of a new area and a new widget to equal the field that the original sets, so that later draw and input code reads the right values.
4. As a developer of the port, I want an area to hold its widgets in row order with the lowest and highest row recorded, as in the original, so that scrolling works on a built screen.
5. As a developer of the port, I want a widget that goes into a popup area to widen that area, take the next row and pick its column as in the original.
6. As a developer of the port, I want area create and widget create to return -1 when no slot is left.
7. As a developer of the port, I want the reset to free every saved screen and picture and leave the screen manager empty, so that the next screen starts clean.
8. As a player, I want a screen to draw in the order of the original: background, areas with no owner, widgets with no area, popup areas.
9. As a player, I want a screen to show its background picture, and the default picture when the file is missing.
10. As a developer of the port, I want a screen with an empty background name to keep the present screen as its background, as in the original.
11. As a developer of the port, I want the five flags that the screen draw reads to exist in the port, so that later work finds them in place.
12. As a developer of the port, I want the demo to use the ported functions, so that the app exercises the same code as a real screen.

## Decisions
- The object that owns the areas and widgets comes from display.cpp of the original. Its functions sit from `FUN_0055ef50` to `FUN_005603e0`. The glossary keeps the term screen manager for it, because the port already uses the name display for its SDL surface.
- scrman.cpp of the original is a different object, a global at 0xb74450. It holds a stack of screens for each of 16 players, and most of its code sends network packets. The glossary calls it the screen stack. It is not part of this work.
- The work ports these functions: the constructor `FUN_0055ef50`, the reset `FUN_0055f7c0`, area create `FUN_0055fba0` with area init `FUN_00402ac0` and order append `FUN_005603e0`, widget create `FUN_0055f990` with widget init `FUN_005ee150` and add to area `FUN_00403200`, widget release `FUN_005eef00`, and the screen draw `FUN_0055f300`.
- Area create takes 11 arguments in the order of the original: x1, y1, x2, y2, column count, column weights, row count, row weights, flags, colour, owner widget. It returns the area index.
- Widget create takes 18 arguments in the order of the original: flags, x1, y1, x2, y2, column, row, style, colour, highlight colour, text flags, font, text colour, text, the value for 0x40, the value for 0x08, the value for 0x48, area index. It returns the widget index.
- The screen manager counts the slots it used apart from the entries in its draw order. The original keeps the area slots at 0x12e99e and the widget slots at 0x12e9a0.
- The port has one screen manager. The original has two, and the second draws the screens of a remote player.
- The port leaves out the network writes. Area create and the reset of the original write each call into a buffer at 0xae2908. The differential test gives the original a real buffer and does not compare it.
- Where the original shows its error box, the port returns without the box. Area create and widget create return -1 when no slot is left, and set the overflow flags at 0x12f54d and 0x12f549 as the original does.
- Area create of the original refuses a new area when 249 are in use, so the last of its 250 slots stays empty. The port does the same.
- The original accepts a weight table for 1 to 29 columns or rows. With a count of 30 it ignores the table and gives each column or row the weight 1. The port does the same.
- A count above 30 makes the original write weights past its two tables, over the end of the area and into the next area. The port sets 30 weights and stops. No differential case uses a count above 30.
- The constructor of the original leaves the two counts of the draw order as they are. A caller must run the reset before the first create. The port does the same.
- Area init sets a field at 0x210 to -1. Its purpose is not known. The port stores it under an offset name.
- The screen manager has a pointer at 0x12f529 that the constructor clears. Its purpose is not known. The differential test checks only that it is clear.
- After each error box the original sets `DAT_00b67a34` to 0. It looks like the state of the left mouse button. The port does not do this until it has mouse handling.
- The image loader of the port counts the images it frees. The differential test compares that count with the frees of the original, because a pointer set to nothing looks the same with and without a free.
- Widget create sets three fields whose purpose is not known: 0x08, 0x40 and 0x48. Widget init also stores the tick count of the original (`DAT_00b67a08`) at 0x50 and sets 0x7e to -1. The port stores them under offset names. The differential test compares them and sets the tick count to a fixed value.
- Widget create of the original refuses a new widget when 1199 are in use, so the last of its 1200 slots stays empty. The port does the same.
- The text of a widget holds 256 bytes, and a field at 0x180 follows it. The earlier spec gave 260 bytes. The original copies a text with no limit. The port keeps 255 characters. No differential case uses a longer text.
- Widget init puts a reversed box in order, except when the style has bit 0x400 and not bit 0x4000. The bits are those of the style, not of the widget flags.
- A widget in a popup area gets five spaces before its text. It takes the next row of the area and the column after that of the last widget. The area becomes at least 125 pixels wide, or as wide as the text plus 15. Its height grows by the font height plus 6. A widget on an odd row gets the lighter colour.
- For font 0 the original reads the font height from the pixel format, and the port reads 0. No differential case puts a widget with font 0 into a popup area.
- A popup area with no columns makes the original divide by zero. The port uses column 0.
- An area index outside the 250 areas makes the original read other memory. The port treats the widget as a widget in a plain area and does not add it to an area. An area that no create made has no widget array, and the original reads address 0 for it. No differential case uses either.
- Add to area of the original takes a widget when the area holds 1199 and then writes one entry past its list. The port takes none then. Widget create cannot reach this state, because it stops at 1199 widgets.
- Widget release frees the picture of a widget unless the text of the widget equals the name of the shared picture. Then it lowers the use count of the shared picture.
- New fields with known purpose: the widget array of the screen manager at 0x04 of a widget, the index of a widget at 0x70, and the text of the caller at 0x54 for a text entry widget. Fields with offset names: 0x7e and 0x180 of a widget, and 0xb78 of an area, which the search for the last column uses as its place in the list.
- The screen draw of the original takes the background file name from the screen stack (`FUN_007fee80`). The port's screen draw takes the name as an argument until the screen stack exists. The port passes the name to its RGN loader unchanged.
- When the name differs from the last one, the screen draw loads that file. When the load fails, it loads `DEFAULT_PIC.RGN` from the data folder. When the name is empty, it captures the present screen as the background.
- The port copies the five flags that the screen draw reads. Three are fields of the screen manager: skip the draw (0x12f53d), clear the screen (0x12f4fa) and darken once (0x12f535). Two are globals of the original: `DAT_00dd6bda` forces a fixed name and `DAT_00b77c8c` gates the clear. Nothing in this work sets them. The two globals start at 0 in the port.
- The screen draw takes the four arguments of the original for the part of the screen to flip. A value of -1 in any of them flips the whole screen.
- The demo stays a made-up screen. It builds its area and widgets through area create and widget create and draws through the screen draw.
- Commits are local. Nothing is pushed.

## Testing
One seam: the differential test, as in the two earlier specs.

For the constructor, the reset, area create and widget create, the test runs the same sequence of calls on the original and on the port and compares every known field of the screen manager, of each area and of each widget. The sequences come from a fixed seed. They include reversed and off-screen rectangles, zero and 30 columns and rows, weight tables and no weight tables, widgets with no area, widgets in a plain area, widgets in a popup area, picture widgets, text entry widgets, and a run that uses every area slot and every widget slot.

For the screen draw, the test builds the same screen on both sides through the create functions and compares the framebuffer. In the emulator it replaces `FUN_007fee80`, so both sides get the same background name. The cases cover a name that loads, a name that fails, an empty name, an unchanged name on a second draw, each of the five flags on and off, and a flip of part of the screen.

After each slice, break the port on purpose in several places and confirm that the test fails for each break.

The demo has no counterpart in the original. Its check is a manual one: start the app with `--demo` and compare the window with the one before the change.

## Out of scope
- The screen stack of scrman.cpp: push and remove a screen, the 60 text slots of a screen, the screen title.
- The 11 functions that change a widget after its create (`FUN_00800580` to `FUN_008022d0`), the packet function `FUN_00564080` that applies them, and set text `FUN_005eed50`.
- Mouse and keyboard handling: `FUN_0055f620`, `FUN_00560490`, `FUN_00560c30`, `FUN_00561050`, `FUN_00561210`, `FUN_005617c0`, `FUN_00561f10`, and the text timer `FUN_00569610`.
- The main loop of a screen, `FUN_007fc7e0`.
- Network games, the second screen manager and the snapshot pool (`FUN_0055f1c0`).
- The destructor `FUN_0055efe0`. The port's screen manager lives for the whole run.
- A screen of the original. The first one comes with the screen stack.

## Open decisions

## Slices
### 1. Constructor, reset and area create
Blocked by: none
Delivers: an empty screen manager, area create with area init and order append, and the reset. Adds the new fields of the screen manager to its struct.
Merge danger: two-way door. Blast radius: none for the current app, because the demo does not call the new code until slice 4.
- [x] After random sequences of area create and reset, every known field of the screen manager and of each area equals the original.
- [x] The 250th area create returns -1 on both sides.
- [x] The reset frees the saved screens of every area.
- [x] The earlier suites still pass.

### 2. Widget create and widget release
Blocked by: 1
Delivers: widget create with widget init and add to area, for a widget with no area, in a plain area and in a popup area. Adds widget release and calls it from the reset. Adds the new fields of the widget to its struct.
Merge danger: two-way door. Blast radius: a wrong field or a wrong order in an area misplaces widgets on every later screen.
- [x] After random sequences of area create and widget create, every known field of each widget and each area equals the original, the row order of each area included.
- [x] A widget in a popup area changes the width, the height and the row count of that area as in the original.
- [x] The 1200th widget create returns -1 on both sides.
- [x] The reset frees the saved screen and the picture of every widget.

### 3. Screen draw
Blocked by: 2
Delivers: the screen draw with the background load, the draw order, the five flags and the partial flip.
Merge danger: two-way door. Blast radius: limited to what a screen shows.
- [ ] Pixel-exact match for random screens built through the create functions.
- [ ] Pixel-exact match for each background case: loads, fails, empty, unchanged.
- [ ] Pixel-exact match with each flag on and off.

### 4. Demo on the screen manager
Blocked by: 3
Delivers: the demo builds its screen with area create and widget create and draws it with the screen draw.
Merge danger: two-way door. Blast radius: the demo only.
- [ ] The app with `--demo` shows the same window as before the change.
- [ ] The up and down keys still scroll the table.
- [ ] The full differential test passes.
