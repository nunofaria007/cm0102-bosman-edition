# Text and fonts
Status: In build

## Problem
The port draws areas and widgets with no text. The text function is empty, so a ported screen shows panels, arrows and marks only.

## Solution
Port the bitmap font path of the original: the loader for its seven `.fnt` files, the line measure, the line draw and the function that fits text into a box. Replace the empty text function with it. Check every step against the original with the differential test.

The original has a second text path through the T2K font engine, and it is the default. The port does not include it. The port looks like the original with bitmap fonts selected.

## User stories
1. As a developer of the port, I want each `.fnt` file to load into the same glyph data as in the original, so that every later text result can match.
2. As a developer of the port, I want a line of text to have the same width as in the original, so that wrapped and centred text lands on the same pixels.
3. As a player, I want anti-aliased text to blend with the picture behind it as in the original.
4. As a developer of the port, I want text that is too wide for its box to wrap or cut at the same place as in the original.
5. As a developer of the port, I want the `#xxxx#` character codes and the `|` character to behave as in the original.
6. As a player, I want a password field to show stars, and an input field to show its cursor mark, as in the original.
7. As a developer of the port, I want the widget and area tests to compare real text pixels, so that the text call is no longer checked through a recorder.
8. As a developer of the port, I want a demo screen in the app, so that I can see an area with widgets and text without the test.
9. As a developer of the port, I want to name the data folder of the original when I start the app, because the original finds it through the Windows registry.

## Decisions
- The port draws text with the bitmap fonts only. The global of the original at `DAT_009cc2c4` selects the text path, and the port always behaves as the original does when it is -1.
- The port drops the mode switch that the widget draw had for that global. A widget always draws its three marks from glyph tables.
- The widget field at 0x76 is the font number and the field at 0x78 is the text colour. The earlier names (text colour and second colour) were wrong. The field at 0x44 is the cursor place for the text function.
- The text function takes its arguments in the order of the original: box, flags, font, colour, text, cursor.
- A font has a height and 256 glyphs. Each glyph has a width, the bytes in one pixel row, a left gap, a right gap and its pixel rows. A pixel row holds two pixels per byte, 4 bits each: 0 leaves the screen as it is, 15 sets the text colour, and any other value blends the two. The original keeps the colours of the screen pixel under its last blended pixel and reads the screen again only when the next screen pixel differs from the last pixel it wrote. The port copies this, because it changes pixels.
- The original loads the seven files into fonts 1 to 7. It has no font 0: the place that its code works out for font 0 holds the pixel format and the palette, so text in font 0 is undefined there. The port keeps font 0 empty. Text in font 0, or in any font with no glyphs, has no width and draws nothing, the cursor mark included. No differential case uses font 0.
- The two Trade Condensed fonts copy some glyphs to other character codes after the load, as the original does.
- The loader of the original trusts the file. The port also rejects a height outside 1 to 255, a width or gap above 255, and a glyph whose pixel row does not hold its width. All seven files pass.
- A character code above 0xff, which a `#xxxx#` code can give, reads past the glyphs of the font in the original. The port gives such a character no glyph and no width.
- The digit test that the original applies to a `#xxxx#` code reads before its character table for a byte above 0x7f, and so accepts 35 such bytes as digits. The code is then the run of real hex digits before the first of them. The port accepts the same 35 bytes.
- The text function of the original has a fault in its wrap. To find the last space, it uses the place of the character in the whole text as an index into the current line. The two are equal only while the line holds the text from its start. They differ after a line break, after a control character and after an earlier cut. The search then starts past the end of the line, so it can find a space left over from an earlier line. The line stays too wide and words of the earlier line appear again. The port does the same, and a test covers it.
- Past those left-over bytes, the search of the original reads stack memory that the function never set. What is there depends on the calls that ran before, most often the text drawn just before. The port reads zeros there. The differential test sets that part of the stack to zeros when the original enters its text function. Without this, 10 of 2400 random widget draws and 207 of 1500 random area draws differ from the port. The random cases use small boxes and large fonts, so they cut lines far more often than a real screen does. A screen of the original that cuts a line in one of the three situations above can still differ from the port.
- The port uses the first 251 characters of a text. The line buffer of the original has 252 bytes, and a longer text writes past it.
- The app takes the data folder of the original from a `--data <folder>` argument. The default is `./Data`. An option that the app does not know ends it with a usage line.
- The app gets a `--demo` argument that draws one area with widgets and text. It is a developer aid and not part of the original. The up and down keys scroll its table.
- The differential test sets the font global of the original to -1 and loads the fonts with the loader of the original.
- Commits are local. Nothing is pushed.

## Testing
One seam: the differential test, as in the area and widget spec.

For the fonts, the test loads each file with the original and with the port and compares the height and every field and pixel row of all 256 glyphs. For a line and for text in a box, it compares the framebuffer after the same call on both sides, with text, font, colour, flags, box and cursor picked at random from a fixed seed. The cases include empty text, text wider than the box, line breaks, character codes, characters with no glyph, and boxes at the screen edge.

After slice 3 the widget and area tests draw real text on both sides and compare pixels. The recorder for text calls goes away.

The demo screen has no counterpart in the original. Its check is a manual one: start the app with `--demo` and look at the window.

## Out of scope
- The T2K font engine and the `.t2k` and `.ttf` files.
- Fonts from a `FONTS` folder.
- Text input: the keyboard handling that moves the cursor and edits the text.
- The language files and any text lookup.
- Screens of the original. The demo screen is made up.

## Open decisions

## Slices
### 1. Font files
Blocked by: none
Delivers: the loader for a `.fnt` file, the table of eight fonts, the lookup of a file in the data folder of the original, the corrected widget field names and the removal of the mode switch.
Merge danger: two-way door. Blast radius: a wrong glyph field shifts or corrupts every character on every screen.
- [x] Every glyph of all seven files equals the table that the original builds.
- [x] A missing or cut file fails the load on both sides.
- [x] The widget and area tests still pass.

### 2. One line
Blocked by: 1
Delivers: text conversion with `#xxxx#` codes, the gap between two glyphs, the width of a line, and the draw of one line with the cursor mark.
Merge danger: two-way door. Blast radius: wrong widths move all centred, right-aligned and wrapped text.
- [x] Equal widths for random strings in every font.
- [x] Equal pixels for random strings, fonts, colours, places and cursor places, on a noise picture.

### 3. Text in a box
Blocked by: 2
Delivers: wrap, line breaks, alignment, password stars and the embossed style. Replaces the empty text function. The widget and area tests draw real text.
Merge danger: two-way door. Blast radius: limited to text.
- [x] Pixel-exact match for random text and flags.
- [x] The widget and area tests compare pixels with real text, with no recorder.

### 4. Demo screen
Blocked by: 3
Delivers: `--data` and `--demo` in the app. The demo draws one area with widgets and text over a background.
Merge danger: two-way door. Blast radius: the app start. The demo code is separate from the ported code.
- [x] The app starts with `--demo` and shows the area with text.
- [x] The app still shows an RGN file when it gets one as its argument.
