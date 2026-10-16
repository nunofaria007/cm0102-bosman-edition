# Club screen frame: tab bar, heading, side bar, Back (inventory for the team views screen)

Date: 2026-10-15. Source: `clean-3968/cm0102.exe`, read with capstone (`/tmp/claude-501/tvs/disx.py`). Every fact has its address. "Inference" marks what the code does not show directly. "Unknown" marks what I could not find.

This file does not repeat earlier facts. It cites them:

- Heading `0x803f30`, side bar `0x75db00`, side bar input `0x7615d0` and `0x7643d0`, Back/Next bar `0x5edcc0`, `cell_button 0x8040a0`: `2026-10-14-start-screen-helpers.md` sections 2, 3, 5, 7, 8.
- Club screen push, node slots 0, 1, 4, 7, 0x18, popup pattern (`0x55fba0` with an owner, `0x55f990` items), id read through `D + 0xba966 + w*0x18c`, data through `D + 0xba9a6 + w*0x18c`, result -0xb = rebuild: `2026-10-15-squad-dropdown-inventory.md`.
- Main loop results (-2 back, -0xb rebuild), push `0x7fe600` and the leave call: `2026-10-13-screen-stack-inventory.md`.

Helpers used below, with their signatures:

- `0x7ff010(slot)`: thiscall on S = 0xb74450, `ret 4`, returns the node slot value.
- `0x7ff270(slot, value, 0)`: thiscall on S, writes the node slot (squad inventory).
- `0x55fba0`: area create, thiscall on D, 11 args `(x1, y1, x2, y2, columns, col_weights, rows, row_weights, flags, colour, owner)`.
- `0x55f990`: widget create, thiscall on D, 18 args `(flags, x1, y1, x2, y2, column, row, style, colour, highlight_colour, text_flags, font, text_colour, text, +0x40, id, data, area)`.
- `0x804030`: button with its own rectangle. Thiscall on S, 12 args, `ret 0x30` (0x804090): `(x1, y1, x2, y2, style, colour, text_flags, font, text_colour, text, id, data)`. It calls `0x55f990(2, x1, y1, x2, y2, 0, 0, style, colour, colour, text_flags, font, text_colour, text, 0, id, data, -1)` on the current Display (0x804030..0x80408b).
- D (current Display) = `[0xb74c4c + s16[0xb77c7a]*0x300]`.

## 1. Tab bar `0x5ed780` (gui_utils.cpp)

### Signature

```c
/* cdecl, plain ret (0x5edcb6). Returns 1, or 0 after an error box. */
int tab_bar(TabEntry *entries, int count, int selected,
            int *y_top, int *y_bottom, int split_up);
```

Evidence: the frame is 0x414 + 0xc (SEH) + 4 pushes. Arg 1 `[esp+0x424]` before the pushes (0x5ed7c1). After `push edi` (0x5ed862): arg 1 `[esp+0x434]`, arg 2 `[esp+0x438]`, arg 3 `[esp+0x43c]`, arg 4 `[esp+0x440]`, arg 5 `[esp+0x444]`, arg 6 `[esp+0x448]` (byte). The club build pops 0x24 = 6 args + 3 left from the sprintf before it (0x455248).

Errors (error box `0x5e8290` with gui_utils.cpp string 0x9cdc00, then return 0):

- `entries` == 0: line 0x15e (0x5ed7cc).
- `count` > 12: line 0x166 (0x5ed80b).
- `y_top` == 0 or `y_bottom` == 0: line 0x16e (0x5ed843..0x5ed859).

### Entry record, 0xd4 bytes, packed

The constructor `0x4568f0` sets +0 = -1 and +2 = 0 (0x4568f2, 0x4568f7). The destructor `0x5a53c0` is a plain `ret`. The club build makes 0x19 entries with `0x9463af` (vector constructor) at 0x45494e.

| Offset | Type | Meaning | Evidence |
|---|---|---|---|
| +0x00 | s16 | Font. -1 means font 1. | 0x5edb51..0x5edb5a |
| +0x02 | u16 | Text colour override. 0 means the default colour of the state (below). | 0x5edaef, 0x5edb1a, 0x5edb34 |
| +0x04 | char[0xc8] | Text. The tab bar passes `entry+4` as the text. Size 0xc8 is from the layout (inference). | 0x5edb78, 0x5edbb0 |
| +0xcc | s32 | Widget id (+0x08) | 0x5edb5f, pushed at 0x5edb77 |
| +0xd0 | s32 | Widget data (+0x48). The tab bar compares it with `selected`. | 0x5ed900, 0x5edae7, pushed at 0x5edb76 |

### What it does

1. It allocates `count*0xd4 + 4` bytes and copies all entries into the copy (0x5ed872..0x5ed93a). Pass 1 finds the entry whose +0xd0 == `selected` and keeps its +0xcc as `sel_id` (0x5ed8f9..0x5ed956). If no entry matches, `sel_id` holds a heap pointer (0x5ed87a), so no id matches it.
2. Tabs per top row: when `count` > 5, `split_up` != 0 gives `(count+1)/2`, else `count/2`. When `count` <= 5, all tabs go in the top row (0x5ed96c..0x5ed993).
3. Top row, at tab 0 (0x5ed9cb..0x5eda3d):
   - Background area: `0x55fba0(100, *y_top, 790, *y_top+0x23, 1, 0, 1, 0, flags 2, colour 0, -1)` (0x5ed9fe).
   - Tab area: `0x55fba0(100, *y_top, 790, *y_top+0x23, columns = per_row, 0, 1, 0, flags 1, 0, -1)` (0x5eda31).
   - `*y_top += 0x2d` (0x5eda3a).
4. Bottom row, at tab `per_row` (0x5eda44..0x5edac6): the same two areas at `(100, *y_bottom-0x23, 790, *y_bottom)`, with `count - per_row` columns. Then `*y_bottom -= 0x2d`.
5. One widget per tab, column = position in the row, row 0 (0x5edb75..0x5edbed):

   ```c
   widget_create(D, flags, 0,0,0,0, column, 0, style, BOX, BOX, 0xc, font,
                 text_colour, entry+4, 0, entry.id, entry.data, area);
   ```

   - `BOX` = u16 `[0xb0be36]`, made once by `0x5e4800(0x20, 0, 0x60, 0)` (0x5ed7a4..0x5ed7bb). Inference: RGB (0x20, 0, 0x60), dark purple.
   - Selected tab (`entry.data == selected`): style 0x830, text colour `+2` or `[0xaebdec]` (0x5edae7..0x5edb03). Inference: style bit 0x800 draws the selected look.
   - Same id as the selected tab: style 0x30, text colour `+2` or `[0xae3184]` (0x5edb0c..0x5edb2b).
   - Other tabs: style 0x30, text colour `+2` or `[0xaebde8]` (0x5edb34..0x5edb45).
   - Flags: 0x4002 when `id != 0` and `id != sel_id`. Else 2 (0x5edb5f..0x5edba0, 0x5edbd8). The meaning of flag 0x4000 is unknown. An id of 0 gives a widget that returns no result (the hit test returns -1 for id 0, squad inventory section 2).
6. It frees the copy and returns 1 (0x5edc0b..0x5edc34).

### The ten club tabs (0x454ebb..0x45523b)

Entry k is at `[esp + 0x314 + k*0xd4]` in the club build frame.

| k | Text | Id | Data | Condition | Evidence |
|---|---|---|---|---|---|
| 0 | "Squad" 0x98e88c | 2 | 1 | | 0x454ec2, 0x454edc, 0x454eed |
| 1 | "Transfers" 0x98e880 | 2 | 2 | When `0x53b900(club)` != 0: id 0, data 0, colour `[0xae317e]` (grey) | 0x454f06..0x454f47 |
| 2 | "Next Match" 0x98e874 | 2 | 3 | | 0x454f6f, 0x454f7c |
| 3 | "Fixtures" 0x98e868 | 2 | 4 | | 0x454f8c, 0x454f93 |
| 4 | "Finances & Info" 0x98e858, else "General Info" 0x98e848 | 2 | 5 | Own club (`0x600c90(club,0,1)`) and not `0x53b900` | 0x454f88..0x454fd3, 0x454fdb, 0x454ff4 |
| 5 | "Tactics" 0x98e840 | 3 | 0 | | 0x45500a, 0x455011 |
| 6 | "Training" 0x98e834 | 4 | 0 | Else id 0, grey, when not own club or `0x53b900` | 0x455023..0x45505a, 0x45506f |
| 7 | "Last Match" 0x98e828 | 5 | 0 | | 0x455096, 0x45509d |
| 8 | "<position> in <division>" 0x98e800, or the division name | 0x7e5 (data = encoded name link) or 0x7d0 (data 0) | | `0x444190` finds the league. Else "Division" 0x98e7f4, id 0, grey | 0x4550a4..0x4551e7 |
| 9 | "History" 0x98e7c8 | 6 | 0 | | 0x4551ee..0x45522c |

The call at 0x455243: `tab_bar(entries, 10, (s8) slot4, &y_top = 0x50, &y_bottom = 0x221, 1)` (0x455205..0x45523b). Ten tabs with `split_up` 1 give five tabs on top (y 80..115) and five at the bottom (y 510..545). After the call, `y_top` = 0x7d (125) and `y_bottom` = 0x1f4 (500). The tab body builders get `(y_top, y_bottom)`: case 1 calls `0x456900(y_top, y_bottom)` (0x45525c..0x455266). So arg 2 of 0x456900 (unknown in the squad inventory) is the bottom y, 500 for the club screen.

`0x53b900(club)` returns `club->id >= [0xae235c] - 2*[0xae2350]` (0x53b97a..0x53b993). Inference: true for a national team.

### How the club input switches tab

In `0x4671f0` (0x467701..0x4677b1):

```c
if (id == 2) {
    data = (w != -1) ? widget[w].data : 0;      // +0x48, 0x46772d
    if (data != slot(4)) {                      // 0x467738..0x467746
        set_slot(4, data, 0);                   // 0x7ff270 at 0x467783
        return -0xb;                            // 0x4677ac: rebuild
    }
    // same tab: falls through to the other tests (inference: no effect)
}
```

The id comes from `D + 0xba966 + w*0x18c`. When it is 0, the code uses s16 `[0xdd6bde]` (0x4676f6..0x4676fb). Ids 3, 4, 5, 6 (Tactics, Training, Last Match, History) push other screens and return -4 (0x4677fa..0x467829, 0x46786d, 0x467a93, 0x467c80..0x467cb3). Then the input switches on slot 4 for the tab's own widgets (jump table 0x4689e4, 0x467cbd).

### Recipe: a tab bar with N tabs, tab k selected

Conditions: N <= 5 for one row. Build the entries in emulated memory.

```c
TabEntry e[N];                 // N * 0xd4 bytes, packed
for (i = 0; i < N; i++) {
    e[i].font   = -1;          // +0x00, gives font 1
    e[i].colour = 0;           // +0x02, default colours
    strcpy(e[i].text, name[i]);// +0x04
    e[i].id     = TAB_ID;      // +0xcc, the same id for all tabs
    e[i].data   = i + 1;       // +0xd0, tab number
}
int y_top = 0x50, y_bottom = 0x221;
tab_bar(e, N, k, &y_top, &y_bottom, 1);   // cdecl 0x5ed780, caller pops 0x18
// now y_top = 125. y_bottom stays 545 when N <= 5.
```

Use one shared id for the tabs. Then the unselected tabs get flags 2 and text colour `[0xae3184]`, as the club tabs do. The tab bar copies the entries, so a short-lived buffer is enough (0x5ed918).

For the team views tabs (Best XI = 1, Squad depth = 2, Compare players = 3): the input does `if (id == TAB_ID && data != slot) { set_slot(slot, data, 0); return -0xb; }`.

Do not use ids that the shared code handles: -2, -3, 0x3e8, 0x3fb, 0x402, 0x421, 0x423, 0x424, 0x426, 0x42f, 0x7d0..0x7e7 (start screen helpers section 8, screen-stack inventory `S+0x37f2`). Inference: the club id 2 is safe in a new input function, because only the club input gives it a meaning. A new id such as 0x60 is safer if a wrap ever shares the club input.

## 2. Club heading in 0x454900

### Colours

Node slot 5 and slot 6 hold the two colours. The build reads them at 0x4549bc..0x4549d3 into `[esp+0x4c]` (slot 5) and `[esp+0x28]` (slot 6).

When both are 0, it calls `0x53b640(club, 1, &slot5, &slot6)` and stores the results in slots 5 and 6 (0x454d6e..0x454dbc).

`0x53b640(club, kit, u16 *fg, u16 *bg)`, cdecl, returns 1 (0 with an error box when club is 0):

- Defaults: `*fg = [0xaebdf0]`, `*bg = [0xae3184]` (0x53b640..0x53b666).
- Kit 1: fg record = `club+0x83`, bg record = `club+0x87` (0x53b6ec). Kit 2: +0x8b/+0x8f. Kit 3: +0x93/+0x97. When a kit 2 or 3 pointer is 0, it uses +0x87 for fg and +0x83 for bg (0x53b730).
- Each record pointer gives RGB bytes at +0x37, +0x38, +0x39. The colour is `0x5e4800(r, g, b, 0)` (0x53b740..0x53b7a3).

The heading call at 0x454eab: `heading(S, colour = slot6, text_colour = slot5, 0xde1f64)`. So the bar colour comes from `club+0x87` and the text colour from `club+0x83`. Inference from the names "fg" and "bg": +0x83 is the club's first colour and +0x87 its second.

`0x5e4800(r, g, b, fmt)` is cdecl. It returns 0 when `[0xaebe0c]` != 0. With fmt 0 it uses the pixel format at `0xae3158`. For a green mask 0x7e0 it packs RGB565 (0x5e4821..0x5e4854).

### Text

Before the heading, the build makes two name strings (0x454dfc..0x454e27):

- `0x542100(club, team, buf_a, 0x12c, 3, 1)` into `[esp+0x19c8]`.
- `0x542100(club, team, buf_b, 0x12c, 1, 1)` into `[esp+0x1b0]`.

`0x542100(club, team, buf, buflen, mode, flag)` is cdecl. `buflen` must be >= 0x47 (0x54210a). Mode 3 (0x542197) uses the full name at `club+4`. Mode 1 (0x54243d) uses the short name at `club+0x38`. For team 0 of a plain club it copies the name (0x5422d8). For team != 0 it adds " Reserves" (0x9b784c). National teams get " B", " 'B'", " Under 21's" or " Under 23's" (0x9b7814..0x9b77cc).

Heading text (0x454e2f..0x454e95):

- When slot 7 has 0x800 (Penalty Takers view), slot 4 == 1 and `0x600c90(club, 0, 1)` (own club): `"{}<%s>{} Penalty Takers"` (0x98e894) with the short name.
- Else: the full name, copied into 0xde1f64.

So the heading shows the same text for the own club and for another club. Only the Penalty Takers view of the own squad differs. A slot 7 value with 0x800 on another club is cleared at 0x454dd0..0x454df7.

### Heading extras

- `0x481ef0(club, slot6, slot5, 1, 0x21, 0x1f, 1, team)` at 0x4563fb. For a national team (`0x53b900` != 0, 0x481f12) it makes a small button with no text at (105, 15)-(120, 35) with the club colours (0x481f49). The function has popups with "Main Squad", "'B' Squad", "Reserve Squad" and so on (0x991550..0x991504). The other branches (0x4821fd, 0x482380) were not read in full. Inference: the team selector, ids 0x1f and 0x21 (the input tests them at 0x467444 and 0x467559).
- `0x482a40(club, slot6, slot5)` at 0x45640b. It makes one button at (660, 4)-(785, 24): "Take Control" (id 0x64), "Scout Club", "Invite Club", "Uninvite Club" (ids 0x66..0x69) (0x482a7a..0x482c90). It returns 1 when it made a button.
- When 0x482a40 returns 0, the build makes the "Print" drop-down (0x45641b..0x4566a6). This is a ready pattern for a Formation button (section 3).

## 3. The rest of the frame

Order in 0x454900:

1. Read slots 0..7 (0x454953..0x4549e7). Slot 0 = club, else the current manager's club (0x4549f8..0x454a28).
2. `0x41c250(club, 0, 0)` at 0x454dc4: background picture. Cdecl. It acts only when the title (`0x7fee80`) is "\" or empty (0x41c252..0x41c26e). With `[0x989858]` == 0 the title is ":" (0x41c27e). Else it looks up a club picture (`0x41cad0`), then by `club+0x69`, `club+0x6e` (`0x41cdd0`), `club+0x57` (`0x41cbd0`), `club+0x53` (`0x41c9d0`), and sets the title with `0x7feeb0(path("CM3_PICTURES", name))`. A new club-like screen can call it the same way to get the club's background.
3. Heading `0x803f30` (0x454eab), section 2.
4. Side bar `0x75db00(4, 0)` (0x454eb3). The same arguments as the start screen.
5. Tab bar (0x455243), section 1.
6. Tab body: switch on slot 4 (0x45524b..0x455255, table 0x456724: 1 = 0x45525c, 2 = 0x455273, 3 = 0x456342, 4 = 0x456359, 5 = 0x45636d). An out-of-range slot 4 gives an error box (0x456381).
7. Heading extras `0x481ef0` and `0x482a40`, or "Print" (0x4563e3..0x4566a6).
8. Back/Next bar: `0x5edcc0(0x7fec60(0) == 0, 0x7febb0() == 0)` (0x4566ab..0x4566cc). Area (100, 555)-(790, 590).
9. Destroy the entry array (0x4566f0).

The club build does not call `0x7feeb0` directly. Only 0x41c250 does.

### Screen regions (classic 800x600)

| Part | Rectangle | Evidence |
|---|---|---|
| Side bar | x 0..89, whole height | start screen helpers section 7 |
| Heading | (100, 10)-(790, 70) | 0x803f30 |
| Team selector button | (105, 15)-(120, 35) | 0x481f49 |
| Action button | (660, 4)-(785, 24) | 0x482c90 |
| "Print" button | (710, 15)-(785, 35) | 0x456464 |
| Tab row | (100, 80)-(790, 115) | 0x5ed9fe with y_top 0x50 |
| Tab body | x 100..790, y 125..500 with two tab rows | 0x5eda3a, 0x5edac3 |
| Tab body, one tab row | y 125..545 (y_bottom unchanged) | inference from 0x5eda44 |
| Back/Next bar | (100, 555)-(790, 590) | 0x5edcc0 |

Inference: with one row of tabs, keep the content above y 550, so it does not touch the Back bar.

### Formation drop-down: copy the "Print" pattern

```c
text = "Formation";
W = 0x804030(S, 710, 15, 785, 35, 0x30, colour = slot6,
             0xc, 1, text_colour, text, id 0, data 0);       // 0x456464
// text_colour = [0xaebdea], or [0xae3184] when slot6 == [0xaebdea] (0x45642e..0x45643c)
P = 0x55fba0(D, 0,0,0,0, 0, 0, 0, 0, 0x130, [0xae31aa], owner W); // 0x456491
for each item:
    0x55f990(D, 2, 0,0,0,0, 0, 0, 0x10, [0xae31aa], [0xaebdec], 1, 1,
             [0xaebdea], text, 0, ITEM_ID, item_data, P);    // 0x4564ed
```

Id 0 makes the button a pure popup opener (squad inventory section 2). The Print items use id 0x22 with data 1, 2, 4, 8, 0xf. A Formation drop-down needs its own item id. The heading place (710..785) is free on a new screen when it does not call 0x482a40. Inference: a place in the tab body also works, with `0x8040a0` in an area, as the squad Filter does.

## 4. Back

There are two Back buttons. Both have id -2:

- "<<<" in the side bar, row 1 (start screen helpers section 7, step 8).
- "Back" in the bottom bar `0x5edcc0`, made as a button only when its first argument is not 0 (0x5edd30..0x5edd5d).

The club input ends with this chain (0x468931..0x4689de):

```c
r = 0x482d10(w); if (r) return r;   // club heading extras (inference)
r = 0x7615d0(w); if (r) return r;   // 0x46896c
r = 0x7643d0(w); if (r) return r;   // 0x4689a6
return 0;                           // 0x46748d..0x4674cb
```

In `0x7615d0`, a click (w != -1) goes to 0x76234c, then `0x725ca0(w)` (only id 0x3e8, 0x76238a). With `S+0x3834` == 0 it jumps to 0x762c58 (0x76239a). There, id -2 returns -2 when `0x7fec60(0)` == 0 (0x762c97..0x762cb0). `0x7fec60(0)` returns 0 when the current node has a previous node and its flag is 0 (screen-stack inventory). The main loop handles -2 at 0x7fd423: leave(), current = previous, rebuild.

So an input function that forwards every unknown widget to `0x7615d0` and then to `0x7643d0` gets both Back buttons, the side bar buttons and the clickable names (0x7d0..0x7e7) for free. This is what the club input does. Push the new screen with flag 0, as the club screen does (0x453e2c), so that Back is enabled.

Not traced: the block at 0x761699..0x762070. It runs before the id tests when the current manager has a state record and `S+0x3834` == 0. Inference: manager-state housekeeping that the club screen also passes through, so it does not change the result for a new screen.

The bottom bar needs `0x5edcc0(0x7fec60(0) == 0, 0x7febb0() == 0)` in the build. The side bar needs `0x75db00(4, 0)`. Without them there is no Back button to click.

## 5. Leave function 0x469da0

`void 0x469da0(void)`, plain `ret` (0x469e6b). It frees heap blocks that the club screen keeps in node slots, and clears those slots:

| Slot | Free | Evidence |
|---|---|---|
| 2 | `free` (0x944c94), slot = 0 | 0x469dde..0x469df4. The build stores a 0x4f-byte copy of a list item there (0x454c24..0x454c43). |
| 3 | `free`, slot = 0 | 0x469df9..0x469e11. Same, 0x454d46..0x454d65. |
| 0xd | destructor `0x68d040`, then `free`, slot = 0 | 0x469e16..0x469e35. Where slot 0xd is set: unknown (not in 0x454900..0x4552ff). |
| 0x15 | `0x9452ca`, slot 0x15 = 0 and slot 0x16 = 0 | 0x469e3a..0x469e62. Where it is set: unknown. |

The push calls the current node's leave before it adds a new node, only when it is not 0 (0x7fe86b). A new screen that keeps only numbers in its slots does not need a leave function. Pass 0. Inference: a new screen that stores heap pointers in slots needs a leave of this form, or the blocks leak each time the player leaves the screen.

## Unknown

- The meaning of flag 0x4000 in the tab widgets, and of style bit 0x800 (inference: the selected look).
- Where the club screen sets slots 0xd and 0x15.
- `0x482d10` was not read. Inference: the input of the heading extras.
- The full body of `0x481ef0` (team selector) beyond its first button.
- The block 0x761699..0x762070 of `0x7615d0`.

## Not needed by a new screen

- `0x482a40` (Take Control, Scout, Invite: network and job actions).
- The "Print" items (printing).
- The Penalty Takers heading text.
