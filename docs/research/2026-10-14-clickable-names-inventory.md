# Clickable names inventory (issues #11 and #18)

Research for slice 5, part A, of `docs/specs/2026-10-14-quality-of-life-0.2.0-spec.md`. Original: `clean-3968/cm0102.exe` (3.9.68, SHA-256 0c78e7da...). Method: objdump disassembly (Intel syntax), REA (Ghidra) where it decompiles, and unattended runs of the recompiled game with a scratch copy of the runtime that dumps the widget table and logs the text calls. The scratch copy is not in the repo. "Inference" marks a conclusion that the code does not show directly. All numbers are hex.

## Main finding

The original already has clickable names. News text, squad lists, results and many other screens put a button widget on each name, with an action id and the record pointer. A click goes through the game's own handler, which pushes the profile screen, so Back returns. What the original does not have is a visible cue: no underline, and no colour change. The original changes the mouse cursor over these widgets, but the port shows the arrow everywhere. So #11 and #18 need a hover cue and a cursor, not new click handling.

Runs that show this (scratch runs, classic mode, new game as Arsenal):

- News screen, item "Arsenal appoint Test Smoke as manager": widget 28, rect 329,293-432,314, id 7d7, data = staff record of the manager, empty text, lies exactly over "Test Smoke" in the body. Widget 29, rect 120,293-185,314, id 7d5, data = club record, lies over "Arsenal". The headline has the same pair (ids 7da and 7d5).
- A click at (380, 303) opened the profile "Test Smoke (Arsenal)". Back at (358, 572) returned to the same news item. A click at (150, 303) opened the Arsenal squad screen.
- Squad screen: each player name is a widget with its text, id 7d7, data = staff record, style 1. Example: widget 202 "Seaman, D", rect 177,223-350,244. 20 such widgets.
- Afternoon Results screen: club names are widgets with id 7d5 (example "Birmingham", rect 177,178-406,198). The score ":" widget has id 7d3 (match).
- Mouse over the news link: cursor kind global 0x9c36e8 = 2 and 0x9cd804 = 2. Mouse over empty space: both 0.

Not confirmed at run time: the transfer screens (the club's Transfers tab is empty on day one) and the match report (the holiday that reaches a match stops at random screens, so the script did not reach it). For those, see the static evidence in section 3.

## 1. News

### How a news item is stored

- A news item keeps a type and parameters, not finished text. +0 is a u32 type code. `FUN_00785670` (0x785670) maps the code to a category by range: 1..3e7 gives 2, 3e8..7cf gives 7, 7d0..bb7 gives 3, bb8..f9f gives 4, fa0..1387 gives 5, and so on.
- The parameters follow as packed u32 values at +5, +9, +d, +11, +15, +19, +1d, +21 (reads in 0x77bc80..0x782b6f: 32, 28, 23, 17, 12, 12, 6, 6 reads). Some are ids: for example `[item+9]` indexes the competition table `DAT_00adadfc` (0x77be2a). +1 is a byte that the builder compares (0x77c09f). The meaning of +4 is unknown.
- Inference: the parameters are ids or record pointers of the people, clubs and competitions that the item names, chosen per type.

### Where the text is built

- `FUN_0077bc80` (news.cpp, thiscall on the item, args (?, comp_text*, 1, 0)), 0x77bc80..0x782b6f, about 28 KB. It formats the text with the text function `FUN_0066f4e0` (101 calls) from language strings such as 0xa65574 "<%s - Player Name...> will be unavailable for ...". Player names go in as plain `%s` strings.
- For some types it hands the work to the module that owns the event: example type 3e8 calls `FUN_0044c650` (0x780211), type 3e9 calls `FUN_004a7f50` (0x78024f).
- The result goes into a compound text object (comp_text.cpp, string 0x99ebd8). The object is {char *text, list *links}. `FUN_004ae660` sets the text (thiscall, 1 arg). `FUN_004ae8a0` (thiscall, (name, id, data), ret 0xc) adds a link: a 0xd0-byte entry with the name (up to c8 bytes), +c8 = action id, +cc = data.
- Links that the news builder adds: in 0x77bc80, 58 pushes of id 7d5 (club), 28 of 7d0 (competition), 6 of 7d8 (staff, example injury news 0x78160b next to string 0xa65574), 3 of 7d1. Across the whole program, 1193 call sites add links, 281 of them with id 7d7.

### Where it is drawn

- The news screen is pushed by `FUN_00788010` (news_screens.cpp, string 0xa694ac): push 0x7fe4a0 with build 0x7881e0, input 0x7892b0, leave 0x7897a0.
- The build function calls 0x77bc80 (0x788ff6) and then `FUN_004aeaf0` (0x78901a), thiscall on the comp_text, 8 args, ret 0x20: (area, 78, 302, text, 3, colour, 0, 0).
- `FUN_004aeaf0` (comp_text.cpp) wraps the text into lines with the width function `FUN_005e5ad0`, and makes one label widget per line (`FUN_0055f990`, 0x4af298, flags 1, text in static line buffers at 0xadae1c + n*1f5). Then for each link it finds the name in the text (`strstr` 0x945a60, 0x4af34a), measures the text before it and the name, and makes a button widget over the name (0x4af741, and 0x4af801 / 0x4af904 / 0x4af9c4 for a name that wraps over lines): flags 2, style 1, colours 0, no text, id = entry+c8, data = entry+cc, x1 = line x + width of the text before the name, x2 = x1 + width of the name.
- So the screen rect of each linked name is in the widget table. The run above shows it.

## 2. Profiles

The common input handler `FUN_007643d0` (110 callers; the start screen also falls through to it, see 2026-10-14-start-screen-helpers.md section 8) reads the clicked widget's id (+08) and data (+48), calls the opener and returns -4. The main loop then builds the new top screen. Back (-2) returns to the node before, so Back works with no change.

| Id | What | Call (cdecl, args) | Evidence |
|---|---|---|---|
| 7d7 | Staff profile (player or non-player) | `FUN_008702a0(staff, 1, 1, 0, 0)` | 0x764b2a..0x764b63 |
| 7e7 | Staff profile, other start page | `FUN_008702a0(staff, 1, 1, 0, 2)` | 0x764bcb..0x764c04 |
| 7d8 | Staff profile, other start page | `FUN_008702a0(staff, 2, 1, 0, 0)` | 0x764c6d..0x764ca6 |
| 7da and later | Staff profile through `FUN_004739a0` | `FUN_008702a0(x, 3, 1, 0, 0)` | 0x764cf9..0x764d21 |
| 7d5 | Club | `FUN_00453d60(club, 0, 0, 0, 0)` if the checks with `FUN_0053b900` and club+53 pass | 0x764930..0x7649a0 |
| 7d6 | Club reserves | `FUN_00453d60(club, 1, 0, 0, 0)` | 0x7649f7..0x764a30 |
| 7d0 | Competition | `FUN_0048f4a0(comp, -2, -2, 0)` if `DAT_00adadfc[comp->id]` != 0 and two checks fail | 0x764470..0x7644dc |
| 7d3 | Match | `FUN_00729e30`, `FUN_0071b4a0` | 0x76488d..0x7648e6 |

- `FUN_008702a0` (staff_screens.cpp, string 0xa84aac): arg 0 = staff record (box with assert 0x149 when null), arg 3 non-zero keeps the 60 slots of the current node. It pushes build 0x870700, input 0x888260, leave 0x88b8a0 with `FUN_007fe600` and fills slots 0..15. It serves players and non-players. Inference from the job byte +3d tests at 0x87031c..0x870392.
- `FUN_0048f4a0` (comp_screens.cpp by position) pushes 0x48fcd0 / 0x4965d0 / 0x4985e0.
- Inference: the handler reads +48 only for a clicked widget. With no widget (injected id at 0xdd6bde) it passes a null record (0x764b78), so the runtime must not inject these ids. It does not need to.

## 3. Text drawing and how screens draw names

- Text in a box: `FUN_005e6d80`, cdecl (x1, y1, x2, y2, flags, font, colour, text, cursor). Flags 1 = left, 2 = top, 40 = right, 100 = wrap. Callers: the widget draw `FUN_005ee590` (4 sites, for example 0x5ee9f4, box inset from the widget rect), the error box, the progress box. Every screen text goes through it.
- Line width: `FUN_005e5ad0`, cdecl (font, text). Line draw: `FUN_005e5160` (called twice from 0x5e6d80; inference: plain and emboss).
- Where a word lands: 0x5e6d80 does not return positions. The runtime can wrap it, keep the box, flags and font, and measure a prefix with 0x5e5ad0. For linked names it does not need to: the comp_text link widgets already have the exact rect.
- Two kinds of name on screens:
  1. Linked words inside running text (news, and 281 other 7d7 link sites such as match_screens `FUN_00725f70` x9 and transfer_screens 0x8e9760, 0x8f24c0, 0x8f6a10, 0x8f9a20, 0x8fbcc0): an invisible button over the name. Rect = name.
  2. List rows (squad, results, and at least 26 widget-create sites with id 7d7, for example club_screens 0x460590 x8, match_screens 0x71c270 x2 and 0x722ed0, transfer_screens 0x8e9760 and 0x8f5080, tactics_screens 0x8a2ca0): a button that holds the name as its text, style 1. Rect = the whole cell, wider than the name. Counts are lower limits, because an id that comes from a register is not counted.
- Free text with no widget: none found on the screens checked. Unknown for screens not checked.

## 4. Hover

- The runtime already wraps the input poll `FUN_0055f620` (src/recomp_rt/win/user32.c, `RT_WRAP(0055f620, input_poll)`). The main loop calls it once per pass on screen manager 0 (0x7fcc0e).
- The hit test `FUN_00560490` sets the cursor kind global 0x9c36e8 to 2 when the mouse is over a widget with flag 2 and an id (+08) or a popup (+7c). The poll then calls `FUN_005e8170(kind)` (0x55f72c), which keeps it in 0x9cd804 and calls SetCursor with handle 0xaebe20 (kind 2, resource 0x67), 0xaebe1c (kind 1, resource 0x69, busy) or 0xaebe24 (kind 0, resource 0x65). Loader: 0x5e266d..0x5e26a7.
- The port's SetCursor shows the system arrow for every handle except IDC_WAIT (user32.c, `RT_IMPORT(SetCursor)`), so kind 2 looks like kind 0. The shape of resource 0x67 was not decoded. Inference: a hand.
- Widget table: screen manager 0 at 0xb77c94 (scrman + 3844). Count s16 at +12e9a0, widgets at +ba95e, stride 18c. Fields: +08 id, +0c flags, +10..+1c screen rect (set by the area draw), +38 style, +3c text flags, +48 data, +76 font, +7a area, +80 text. Areas: area +bed open, the row checks are as in the hit test (2026-10-13-input-inventory.md 2.2). Widgets in closed popups keep rect 0,0-0,0.
- Read the table from the poll wrapper only. The script dump found `S+382a` (current channel) at 16 outside a loop pass, so the record pointer at 0xb74c4c + channel*300 is not safe there.

## 5. Recommended design

Keep the game's widgets, clicks and drawing as they are. Add a cue in the runtime only.

1. Hover target, in the `input_poll` wrapper after `o_0055f620`: if 0x9c36e8 == 2, find the topmost widget under the mouse with the test of the hit test (flag 2, inside +10..+1c, area open and row in view). Accept it as a name when the id is 7d5, 7d6, 7d7, 7d8, 7da..7df or 7e7 and the style +38 is 1. Inference: style 1 separates names from menu items (style 10) and panel buttons (style 30). Check the id list on more screens in the build.
2. Underline extent: for a link widget (empty text), use x1..x2 of the rect. For a row widget with text, record the text position at draw time: wrap `FUN_005ee590` (thiscall on the widget) to know the widget, and wrap `FUN_005e6d80` to keep its box, flags and font. Compute the text's x from the flags and its width with `FUN_005e5ad0` inside the wrapper, where calling the game is safe.
3. Draw: one line of the text colour (+78) under the text, 1 px (2 px in widescreen), on the frame that the runtime presents, not in the game's surface. Remove it when the target changes. This leaves game memory, and so saves and decisions, unchanged.
4. Cursor: map handle CURSOR_BASE+67 to `SDL_SYSTEM_CURSOR_HAND` in `RT_IMPORT(SetCursor)`. This restores what the original does.
5. Option: one line `clickable_names = on|off` in the settings file. Off gives today's look.

Addresses the build needs: 0x55f620 (wrapped), 0x5ee590, 0x5e6d80, 0x5e5ad0, globals 0x9c36e8, 0x9cd804, 0xb77c94 and the widget offsets above. The recompiler's RT_WRAP check stops the build if an address moves.

Risks:

- 0x5e6d80 is called with the T2K or the bitmap text path. Measure with the game's own 0x5e5ad0, not the port's font code.
- Lists that scroll move widget rects (`FUN_00403ee0`). Recompute the target on every poll.
- A name that wraps has two link widgets. Underline both when either is under the mouse (same id and data).
- Popups opened over a list: the hit test order already gives the topmost widget. Use the same order.
- Names that the game never links (a news type whose builder adds no link) stay plain. Adding links would need hooks inside the per-type builders. Leave that out of 0.2.0 unless Max asks.
- Drawing in the present path must not mark the screen dirty for the game, or the same-save check may still pass while the frame timing changes. Inference: low risk, since the game does not read the presented frame.

Estimate: 2 sessions. Session 1 (#11): cursor, hover target, underline for link widgets, option, news screenshots, same-save check. Session 2 (#18): row widgets through the draw wrappers, then squad, transfer and match report screenshots in both modes. Add half a session if the match report needs a scripted route to a played match.

## 6. Unknown

- The shape of cursor resource 0x67.
- Which news types add no link for a named person or club.
- The meaning of news item +1 and +4, and the record type behind each parameter for each type.
- Whether the transfer screens and the match report draw every name as a widget with an id (static evidence only).
