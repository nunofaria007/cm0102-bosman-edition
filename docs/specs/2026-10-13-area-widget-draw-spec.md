# Area and widget draw
Status: In build

## Problem
The port shows a background image and nothing else. Every screen of the original is built from areas and widgets, and the port has no code for either. The port also has no way to prove that a ported function behaves like the original.

## Solution
Port the drawing code that sits between the background image and a finished screen: the basic drawing primitives, the panel, the area and the widget. Check each ported function against the original with a differential test, so that a mismatch of one pixel fails the test.

Text stays out of this work. The widget calls a text function that does nothing until the font system is ported. The differential test records each call to that function and compares its arguments with the calls the original makes.

## User stories
1. As a developer of the port, I want each primitive to match the original pixel for pixel, so that later screens look the same as the original.
2. As a developer of the port, I want a differential test that needs only the original and a Python environment, so that I can check a port without Windows.
3. As a developer of the port, I want a line, fill or glyph table that is partly off screen to draw nothing, as in the original, so that screens that depend on that behaviour stay correct.
4. As a developer of the port, I want the area to compute the same column and row edges as the original for any weights, flags and scroll position, so that every widget lands on the right pixels.
5. As a developer of the port, I want an area with more rows than it can show to get a scrollbar with the same thumb size and position as the original.
6. As a developer of the port, I want a popup area to open on the same side of its widget as in the original, including near the screen edges.
7. As a developer of the port, I want the struct fields to carry the original offsets in comments, so that I can match new findings from the original to the port.
8. As a player, I want panels, grid lines, scroll arrows and popup arrows to look the same as in the original.
9. As a developer of the port, I want the text call to be a single stub, so that the font work can replace it without a change to the widget code.

## Decisions
- The structs for the area, the widget and the screen manager are native C structs. Each field records its offset in the original. The structs are not byte-exact, because pointers are 8 bytes on macOS.
- The original packs these structs to 1 byte. An area is 0xBF1 bytes and a widget is 0x18C bytes. The screen manager holds 250 areas and 1200 widgets.
- Where the original shows its error box (`FUN_005e8290`), the port returns without drawing or writing. The error box is not ported.
- The port keeps the clip rule of the original. A line, fill, darken, glyph table or capture does nothing when any part of it is off screen. An outline rectangle applies that rule to each side on its own, so the sides that are fully on screen still draw.
- The port always uses RGB565. The original also supports RGB555 at run time. The port drops that branch, because the RGN loader already converts images to RGB565.
- The eight glyph tables for scroll arrows and popup arrows are copied from the original into the source.
- A widget draws its three marks as text characters unless the global at `DAT_009cc2c4` is -1. Only then does it use the glyph tables. The original starts with -2. Nothing in this work sets the global.
- Picture widgets share one cached image per file name, as the original does.
- The colours that the original sets up at start (`FUN_005e4560`) are constants in the port, because they depend only on the pixel format. The differential test checks each constant against the original.
- The ported primitives go in a new module named after gui_utils.cpp. The function addresses sit next to that file's asserts in the original, so the name is an inference from link order.
- The existing drawing functions in the display module stay as they are in this work.
- Unicorn runs in a virtual environment outside the repo. The original stays outside the repo. The test takes its path as an argument.
- Commits are local. Nothing is pushed.

## Testing
One seam: the differential test.

A Python script loads the original into the Unicorn emulator and replaces the DirectDraw surface with a fake surface that writes to an 800x600 buffer in emulated memory. The script calls one function of the original. A command-line driver built from the port runs the same call. The script then compares the two framebuffers byte for byte and, for functions that fill a struct, compares each output field.

A good test picks its cases at random from a fixed seed and includes the awkward ones: zero-size shapes, reversed coordinates, shapes that touch the screen edge and shapes that cross it. It checks behaviour through the public functions of the port.

The repo has no other tests.

## Out of scope
- Text and fonts. The widget's text call is a stub.
- The ellipse panel style, flag 0x10000 (`FUN_005e7840` and `FUN_005e7aa0`).
- Mouse and keyboard handling for areas, widgets and scrollbars.
- The screen manager's own functions in scrman.cpp.
- Any change to the RGN display path of the current app.

## Open decisions

## Slices
### 1. Test harness and basic primitives
Blocked by: none
Delivers: clip, colour scale, line, rectangle, darken, glyph table draw and screen capture in the port, each checked against the original.
Merge danger: two-way door. Blast radius: none for the current app, because the RGN display path does not change.
- [x] The differential test passes for each primitive with random cases, including shapes that are partly off screen.
- [x] The app still builds.

### 2. Panel
Blocked by: 1
Delivers: panel draw for every style flag except the ellipse flag 0x10000.
Merge danger: two-way door. Blast radius: a wrong reading of a flag gives wrong borders on every later screen.
- [x] Pixel-exact match for each flag alone.
- [x] Pixel-exact match for random sets of flags.

### 3. Area structs and layout
Blocked by: none
Delivers: the area, widget and screen manager structs, area layout, scrollbar setup and popup position. Removes the unused struct `cm_area_t`.
Merge danger: two-way door. Blast radius: wrong edges move every widget on a screen.
- [x] Every output field matches the original for random grids, weights, flags and scroll positions.
- [x] The popup position matches the original for widgets near each screen edge.

### 4. Widget draw
Blocked by: 2, 3
Delivers: widget draw with the text call as a stub, and the eight glyph tables.
Merge danger: two-way door. Blast radius: limited to widget appearance.
- [x] Pixel-exact match for widgets with no text, for each widget flag.
- [x] Pixel-exact match for popup arrows on all four sides.

### 5. Area draw and scrollbar draw
Blocked by: 4
Delivers: a complete area on screen with panel, widgets, grid lines and scrollbar.
Merge danger: two-way door. Blast radius: limited to area appearance.
- [ ] Pixel-exact match for a scrolled area with widgets.
- [ ] The app still builds and shows an RGN image.
