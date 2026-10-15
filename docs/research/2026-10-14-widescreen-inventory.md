# Widescreen inventory (cm0102.exe)

Slice 3 of `docs/specs/2026-10-14-re-signed-edition-release-spec.md`. How the original sizes its screens and areas, and what widescreen mode (1280x800) needs. Slice 4 builds on it.

Method: capstone disassembly of every function in `tools/recomp/functions.txt` (fan-pack exe, SHA-256 `0c78e7da…bff86`), a scan for the constants 800, 799, 600, 599, 601 and 1600, and runs of the recompiled game with a trace of each area and widget create. The REA server was busy with another agent's binary, so the facts below come from the assembly. Addresses are for this exe. Slice 2 must check them again on the clean 3.9.68 exe.

## 1. Where the screen size lives

| Fact | Evidence |
|---|---|
| The display init `0x5e2620(width, height, flags, font mode, full screen, title)` takes the size as arguments. It stores the width at `0xaebe08` and the height at `0xaebdd0`. | `0x5e2b41`, `0x5e2b51` |
| The back buffer gets the size from the same arguments (DDSURFACEDESC width and height). | `0x5e2620`, the `param_3 & 2` path |
| Four calls pass the constants 800 and 600: start-up, two in the settings apply, two in a network path. | `0x5d79ed/0x5d79f2`, `0x5d8645/0x5d864a`, `0x5d8685/0x5d868a`, `0x8aeed3/0x8aeed8`, `0x8aeefb/0x8aef00` |
| Functions that read the size globals and so follow any size: the text draw `0x5e5160`, the clip helpers `0x5e3680` and `0x5e3730`, the centred intro pictures `0x5cccd0`. | `0x5e51ed`, `0x5e36fc`, `0x5e38d4`, `0x5cce74` |
| Pixel code reads the pitch from the Lock result, not from a constant. No constant 1600 in the display code. The 1600 constants are dates (`0x54a2d0`…) and struct offsets (`0x5ca4f0`, `0x642a70`…). | scan |

## 2. Size constants in the display code (inference: all of them)

Each of these holds 800, 799, 600 or 599 as a screen limit. `tools/recomp/widescreen.py` lists the same addresses.

| Function | What | Addresses |
|---|---|---|
| `0x402ac0` area init | clamps x2 to 799, y2 to 599 | `0x402b7c`, `0x402b84`, `0x402b8d` |
| `0x403760` popup placement | opens the popup below its widget if it ends above y 600 | `0x40384b`, `0x403922`, `0x4039b5` |
| `0x403a70` area scroll and drag | limits 799 and 599 | `0x403b00`, `0x403b64`, `0x403bdc`, `0x403c40` |
| `0x55f2c0` full-screen clear before a progress bar | rect (0,0)-(799,599) | `0x55f2ca`…`0x55f2ec` |
| `0x55f300` screen draw | plain background, clear, fade and flip of (0,0)-(799,599) | `0x55f38d`, `0x55f392`, `0x55f56b`…`0x55f5f0` |
| `0x561210` widget drag | clamps to 799 and 599 | `0x5612a0`…`0x5612d1` |
| `0x5e7840` outline draw | span list starts at x 800, Lock rect 800x600 | `0x5e78ba`, `0x5e78e2`, `0x5e78ea` |
| `0x5e8290` error box | centred in 800 | `0x5e8309` |
| `0x5ee150` widget init | clamps x2 to 799, y2 to 599 | `0x5ee1f5`, `0x5ee20a` |

The other hits of the scan are not screen sizes: money and limits (`0x412680`, `0x42b130`, `0x460590`, `0x517fa0`, `0x5a90b0`, `0x79d680`), buffer sizes for `0x944e46` (`0x48e1c0`, `0x7c6a30`, `0x8423f0`), stack frames (`0x48d500`, `0x5fab70`…), string lengths for `0x90d130` (`0x7d0171`, `0x7d2992`, `0x8bd444`), match and date code (`0x5d219b`, `0x844780`, `0x87dd80`, `0x89f710`, `0x90e760`).

## 3. How screens place areas and widgets

- Screens build their layout with absolute coordinates for 800x600. The area create `0x55fba0` (thiscall, 11 arguments, `ret 0x2c`) takes x1, y1, x2, y2 first. The widget create `0x55f990` (thiscall, 18 arguments, `ret 0x48`) takes flags, then x1, y1, x2, y2. Edges are inclusive: the whole screen is (0,0)-(799,599).
- The coordinates are constants in each screen's build function, or arguments of helpers that pass constants on, for example the menu builder `0x5ed300(110, 145, 780, 535, …)` and the bottom bar `0x5edcc0` at (100,555)-(790,590). Trace of the start screen: side bar (0,0)-(89,599) from `0x75dc27`, heading widget (100,10)-(790,70) from `0x803f79`, menu area (110,145)-(780,535) from `0x5ed529`.
- Widgets in a grid cell pass 0,0,0,0. The area places them by its column and row weights, so they follow the area. Inference from the trace: every cell widget of the start screen and the game screens has zero coordinates.
- A popup area passes 0,0,0,0 and an owner widget (`0x823df2`, flags 0x100030, owner 25). Its place comes from the owner at draw time (`0x403760`).
- Rows: an area has a fixed row count with weights (`0x402ac0` copies the weights to +0xbcb). A taller area gets taller rows, not more rows.

Conclusion: a scale of the four coordinates in `0x55fba0` and `0x55f990` moves every area and widget that a screen creates. No screen needs its own change.

## 4. What draws straight onto the screen

These do not use areas or widgets, so the scale in section 3 does not reach them.

| Function | What | Coordinates |
|---|---|---|
| `0x5e76f0` progress box | panel (100,555)-(790,590), text and bar, flip | constants at `0x5e7738`…`0x5e7823`; the bar fill is `step * 496 / total + 272` (`0x5e77b3`…`0x5e77d2`) |
| `0x5e8290` error and message box | box (x,242)-(x+w,357) centred in 800, an Ok button at y 318..342 | many y constants: `0xf2`, `0x165`, `0x13e`, `0x156` and others |

## 5. Pictures

- The picture load `0x5e40d0` (cdecl: path, picture to reuse) reads an RGN file: a 48-byte header (width, height, size of pixels, …) and 16-bit pixels. The struct keeps width +0, height +4, size +8, pixels +0xc. It allocates with `0x945501` (calloc) and the pixels are freed with `0x9452ca` (free). Evidence: `0x5e4166`…`0x5e41ce`.
- The screen draw `0x55f300` loads the background by name (`0x55f3d3`, the second try in `CM3_PICTURES` at `0x55f3fb`) and blits it at (0,0) with `0x5e3fd0` (`0x55f436`). The backgrounds in `Pictures/` are 800x600 (960,048 bytes).
- The blue bar at the left is a picture too: `Data/game.mbr` (90x600, 108,048 bytes), and `Data/match.mbr` during a match. The exe names only these two (`strings`). The fan pack adds `g800.mbr`, `m800.mbr` (126x800) and `bkg1280_800.rgn`. No build uses them.

## 6. What 1280x800 needs, and what slice 4 does

| Need | Built |
|---|---|
| Display init, back buffer, window and texture at 1280x800 | the constants of section 1 read `rt_screen_w` and `rt_screen_h`; `ddraw.c` and `user32.c` use them |
| Limits of section 2 | `tools/recomp/widescreen.py`: the recompiler writes `rt_screen_w`/`rt_screen_h` in place of each constant and stops the build if an address has no such constant |
| Layout | `RT_WRAP` of `0x55fba0` and `0x55f990` scales x by 1280/800 and y by 800/600. A left or top edge maps to the first pixel, a right or bottom edge to the last, so touching areas still touch. Zero and negative values stay. |
| Progress box | `RT_REPLACE` of `0x5e76f0` draws it with the original's functions at scaled places |
| Background | `RT_WRAP` of `0x5e40d0`: an 800x600 picture is scaled by 1.6 in both directions and cut to 1280x800 (80 rows off the top and bottom). Bilinear. |
| Left bar | a picture 600 high and narrower is scaled to 800 high and by 1.6 across (game.mbr becomes 144x800), the same as its area |
| Classic mode | `settings.txt` in the save folder, line `display = classic`. The runtime writes the file with `display = widescreen` when it is missing. |

The recompiler gained `RT_WRAP(addr, name)`: the translation of the function becomes `o_<addr>`, and every call goes to the runtime's `f_<addr>`.

## 7. Known limits

- The message box of `0x5e8290` keeps its classic y (242..357). It is centred across the screen but sits above the middle. Moving it needs about 15 more y constants, some of them in mouse tests. Not done.
- Text keeps its size. Areas are larger, so lists show the same number of rows, with more space.
- Not checked on screen: a match in progress (`match.mbr`), the tactics pitch, graphs, and the drag of a widget. Max plays a new game in widescreen mode as the spec's manual check.
- Unknown: whether other functions draw straight onto the screen with classic coordinates. The scan finds constants 799 and 599, not other coordinates such as 555. A screen that draws that way shows its part in the top-left 800x600.

## 8. Fixes after the first play (2026-10-14)

Max found two faults in widescreen mode. Both are fixed.

| Fault | Cause | Fix |
|---|---|---|
| Tactics screen: dotted circles and dashes all over the screen | The circle draw `0x5e7840` plots points with `0x5e7aa0`. It finds the row of a pixel as y * 25 * 32 (`lea`, `lea`, `shl 5`), a row of 800 pixels, at `0x5e7b7d`, `0x5e7be5`, `0x5e7c42`, `0x5e7cb8`, `0x5e7d73`, `0x5e7de2`, `0x5e7e44`, `0x5e7ebb`. | `widescreen.py` `INSNS`: the recompiler writes y * `rt_screen_w` in place of each `shl`. In classic mode the result is the same as before. |
| Match Overview: the possession bar sits at its classic place | The match screens move widgets with the rect setter `0x800890` (thiscall: widget, x1, y1, x2, y2). The bar is set at x 277 to 277 + 436 * share / 100 and a y from the screen's slots (`0x71db48`, `0x71db9b`). Other callers: `0x720320` and `0x724520`. | `RT_WRAP` of `0x800890` scales the four values as the widget create does. |

Search for the same faults:
- Rows of 800 or 1600: the only screen code with y * 800 is `0x5e7aa0`. The other `lea`/`lea`/`shl` sequences are money (`0x4d4fb0`, `0x59cfa0`), struct strides (`0x55f290`, `0x562370`, `0x5ef5e0`, `0x717470`) and other values.
- Pixel code: of the 11 functions that lock the back buffer, only `0x5e7840` holds a size constant (section 2). The others read the pitch from the Lock result.
- Direct draws outside the display code: `0x7fdc00` (network session code) and the display's own functions. No graph or bar outside these draws straight onto the screen.
- Widget moves: `0x800890` is the only setter that changes a rect.

Not checked on screen: the possession bar. A scripted run did not reach a match (injuries, offers and squad numbers stop Continue at random). The fix follows from the code. Max checks it in play.


## 9. Sweep for click areas that do not match what is drawn (2026-10-14)

Method: every function that reads the mouse position (`0xb67a20` x, `0xb67a14` y), every function that touches the widget rect fields (Display +0xba96e..+0xba97a) outside the display code, and every call of the text measure functions `0x5e5ad0` (width) and `0x5e5cd0` (line height).

Mouse readers, all clean (they compare the mouse with area and widget rects, which the area draw sets in screen pixels):
- `0x560490` hit test, `0x560c30` input poll (its 300 is a time in ms), `0x561050` and `0x561f10` scroll bar and thumb drag, `0x561210` widget drag (limits fixed in section 2), `0x5617c0` text entry, `0x5621f0` synthetic click, `0x5e2eb0` flip.
- `0x5e8290` message box: drawn and tested with the same classic y numbers and the same left edge, so clicks land on the buttons.
- Popup menus and drop-down lists: popup areas take their place from the owner widget (`0x403760`), and their items are widgets. Checked on screen: the manager popup, the Tactics and Edit drop-downs.

Fault found: text widths. Screen code places widgets at "x + width of a text" or sizes them to a text, for example the underline of "Non-Playing Staff" (`0x461656`), "Friendly" on Next Match (`0x45d941`), the match screens (`0x71c9e2`, `0x71d445`, `0x72481c`), the title bar box (`0x808f68`) and the menu helpers (`0x5ed153`, `0x5ed25f`, `0x5edfa6`). The width came in screen pixels and the coordinates were scaled again, so the widget came out too wide or too far right of its text. Fix: `RT_WRAP` of `0x5e5ad0` returns the width in classic pixels (rounded up) to these calls. The calls in `widescreen.py` `TRUE_WIDTH` (text draw, text entry, widget init, message box, and `0x4aeaf0`, which the clickable names work owns) still get screen pixels; the build checks each address. Line heights (`0x5e5cd0`) stay as they are: rows and the widgets on them get the same vertical scale.

Other fix: the tactics drop search `0x89f710` (`0x8a20f8`, `0x8a21e2`, `0x8a260f`, `0x8a26f6`) ignores a drop more than 800 screen pixels from every position. Now the limit is the screen width.

Direct users of widget rects outside the display code: only `0x89f710` (tactics input), which works in screen pixels from the pitch widget. Checked on screen: a player dragged on the pitch lands on the position under the mouse, and the With Ball view draws its zones on the scaled pitch.

Not checked on screen: the match screens (a scripted run does not reach a match; see section 8).

## 10. Text with links centred in the classic box (2026-10-15)

Fault (Max): the confirm "The file ... already exists. Replace this file ?" (string `0x9c7ac8`, shown by `0x5909f9` through `0x591840`, screen build `0x591b70`) was off centre in widescreen mode. Classic mode shows the text at the top left of the box, on two lines.

Cause: the text box helper `0x5ed070` passes the box (110,145)-(780,535) to the text with links `0x4aeaf0` (comp_text.cpp), x1 + 5 and x2 - 5. `0x4aeaf0` wraps the text at x2 - x1 + 1 (`0x4aedf7`) and, with flag 1, centres each line at x1 + (x2 - x1 + 1 - width) / 2 (`0x4af17d`). The widths are screen pixels (TRUE_WIDTH), the box was classic, and then the widget create scaled the line x. So each line wrapped at 660 screen pixels and moved right of its centre. The same fault moved the news headline left of the centre.

Fix: `RT_WRAP` of `0x4aeaf0` in `display.c` gives it x1 and x2 in screen pixels, and the widget create scales only the y of the widgets that `0x4aeaf0` makes (return address in `0x4aeaf0..0x4afa30`). This replaces the line shift of the link buttons (section 9). In widescreen mode the confirm is now one line centred in its box (centre x 712), and news text wraps at the width of its box. Checked: the link under "Thierry Henry" on the bid news lands on the name. Classic mode does not change.
