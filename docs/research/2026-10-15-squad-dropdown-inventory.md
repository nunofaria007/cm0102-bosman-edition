# Squad screen: Clear Squad button and Filter drop-down (inventory)

Date: 2026-10-15. Source: `clean-3968/cm0102.exe`, read with capstone (the recomp venv) because REA had no function at 0x454900 and timed out on string search. Every fact has its address. "Inference" marks what the code does not show directly.

## 0. The screen

- The screen is pushed at 0x453e2c and 0x454444: `0x7fe600(build 0x454900, input 0x4671f0, flag 0, leave 0x469da0, extra 0)`, ECX = scrman 0xb74450.
- Build 0x454900 (club_screens.cpp, assert string 0x98e598) reads node slot 4 at 0x4549a7 and keeps its low byte. At 0x455205..0x455255 it switches on that byte (jump table 0x456724, cases 1..5). Case 1 (0x45525c) calls the squad list builder `0x456900(arg1, arg2)`, cdecl, at 0x455266. So slot 4 = 1 is the squad tab. Inference: the other cases are the other club tabs (the tab bar is 0x5ed780 at 0x455243).
- 0x456900 is one function of 25,173 bytes (0x456900..0x45cb54). Arg 1 is the y of the next row: it reads `[esp+0x3b4]` (0x4589f0) and uses it as y1, and y1 + 0x14 as y2 (0x458a03). Arg 2: unknown.
- Input 0x4671f0 is cdecl with one argument: the clicked widget index (s16) at `[esp+0x1150]` (0x46736b), after a frame of 0x1130 + 7 pushes. It ends with a plain `ret` (0x4674cb) and returns the result in EAX.

Node slots that 0x456900 reads (0x456b2d..0x456bf9) and the input reads (0x467231..0x467296, 0x467cd3..0x467d44):

| Slot | Meaning | Evidence |
|---|---|---|
| 0 | Club record pointer | builder `[esp+0x14]`, passed to 0x600c90 at 0x458a34 and to 0x897250 at 0x458a57; input `[esp+0x20]`, passed to 0x896fe0 at 0x46811c |
| 1 | Team index. 0 = first team. Not 0 = reserves (inference: 0x45956d shows " Include Seniors" when it is not 0, " Include Reserves" when it is 0) | builder `[esp+0x40]`; input `[esp+0x14]` |
| 4 | Tab, 1 = squad | 0x455205, 0x467cbd |
| 7 | View bits. 0x800 = the Penalty Takers view (item " Penalty Takers", id 7, data 0x800 at 0x4572c7..0x4572dd) | builder ebx at 0x4589ec; input ebx at 0x467d06 |
| 0x18 | Filter bits (see section 2) | builder `[esp+0x2c]`; input ebp at 0x467d32 |

## 1. The Clear Squad button

The button sits in a small area of 2 columns and 1 row, built at 0x458a27:

`0x55fba0(x1 0x212, y1 edi, x2 0x30c, y2 edi+0x14, columns 2, weights 0, rows 1, weights 0, flags 1, colour 0, owner -1)`. ECX = Display `[0xb74c4c + s16[0xb77c7a]*0x300]` (0x458a09..0x458a21). The area index goes to EBP (0x458a35).

Column 0 holds Clear Squad. Column 1 holds Filter.

The logic (0x458a2c..0x458ad0):

```
if (0x600c90(club, 0, 1)) {                                  // 0x458a37, cdecl
    if (!(slot7 & 0x800) && tactics->0x897250(club, team)) { // 0x458a47, 0x458a58
        text = "Undo Clear" (0x98ec0c); data = 1;            // 0x458a61..0x458a94
    } else {
        text = "Clear Squad" (0x98ec00); data = 0;           // 0x458a96..0x458ac3
    }
    // the text goes into the shared buffer 0xde1f64 through 0x66f4e0 (inference: sprintf with the comment tags removed)
    0x8040a0(area, column 0, row 0, style 0x30, colour u16[0xae317e], text_flags 0xc,
             font 1, text_colour u16[0xae3184], text 0xde1f64, id 0x25, data);   // call at 0x458ad0
}
```

- Both texts use the one call at **0x458ad0** (return address 0x458ad5).
- Widget id: **0x25**. Data (widget +0x48): 0 for Clear Squad, 1 for Undo Clear.
- `0x8040a0` is thiscall on scrman 0xb74450, 11 arguments, `ret 0x2c`, and returns the widget index in AX. It calls `0x55f990(flags 2, 0,0,0,0, column, row, style, colour, colour, text_flags, font, text_colour, text, 0, id, data, area)` on the current Display (0x8040a0..0x8040fd).
- `0x897250` is thiscall on the tactics object `[0xde1d08]`, args (club, team), `ret 8`. It returns 1 when the undo copy of the current human manager exists for this club and team (0x897250..0x8972b7). The manager comes from the record key `[0xb74c26 + chan*0x300]` and `[0xae2364]`.

The click handler is in the input function, case 1 of the tab switch (jump table 0x4689e4, case 1 = 0x467cd3). At 0x4680bd..0x468127:

```
if (id == 0x25) {
    if (slot7 & 0x800) {                                     // Penalty Takers view
        for (i = 0; i < 50; i++) [0xb63c98]->0x601560(i, 0); // 0x4680d2..0x4680f3, thiscall
    } else {
        data = 0x468b90(widget);                             // widget +0x48
        [0xde1d08]->0x896fe0(club /*slot 0*/, team /*slot 1*/, 1, data);   // 0x46811d
    }
    return -0xb;                                             // edi = 0xfffffff5, 0x468122, exit 0x4687db
}
```

`0x896fe0` is thiscall on the tactics object `[0xde1d08]`, 4 arguments, `ret 0x10`, returns 1 (0 when the record is missing):

- arg 1 club. An error box when it is 0 (0x896ff0, tactics.cpp line 0x3d5).
- arg 2 team index. The record comes from `0x8982b0(club, team)` (0x89706b).
- arg 3 save or restore flag, arg 4 undo flag.
- arg 4 = 0 (clear): when arg 3 != 0, it copies the 20 slots (record + 2, 0x50 bytes) into the undo entry of the current human manager (16 entries of 0x5c bytes at +0x30, count at +0x5f0) (0x8970aa..0x897112). Then it sets all 20 slots to -1 (0x897114..0x897133).
- arg 4 != 0 (undo): when arg 3 != 0, it copies the undo entry back if club and team match, and clears the entry (0x897147..0x8971db). When arg 3 = 0, it only drops the undo entries for this club and team (0x8971f4..0x89722d).

The Clear Squad handler passes arg 3 = 1, so Clear saves an undo copy and Undo Clear restores it.

## 2. The Filter drop-down

Built right after Clear Squad, in the same area:

1. Button, call at 0x458b10: `0x8040a0(area, column 1, row 0, style 0x30, u16[0xae317e], 0xc, 1, u16[0xae3184], "Filter" (0x98ebf8 via 0xde1f64), id 0, data 0)`. Id 0 makes it a pure popup opener: the hit test 0x560490 opens the popup of a widget with +0x7c != -1 and returns -1 when the widget id is 0 (input inventory, section 2.2).
2. Popup area, call at 0x458b45: `0x55fba0(0,0,0,0, 0, 0, 0, 0, flags 0x130, colour u16[0xae31aa], owner = widget index from step 1)`. ECX = Display. The area index goes to ESI. Inference: the area create with an owner sets the owner's +0x7c (input inventory, +0x7c table). The popup rectangle comes from the owner (inference, display inventory doubt 152).
3. One `0x55f990` per item, area = ESI. Example " All Positions" unticked (0x458bb6..0x458bf4):
   `0x55f990(flags 2, 0,0,0,0, column 0, row 0, style 0x10, colour u16[0xae31aa], highlight u16[0xaebdec], text_flags 1, font 1, text_colour u16[0xaebdea], text 0xde1f64, +0x40 0, id 8, data 1, area ESI)`.
   A ticked item uses flags 0x2002 (0x458baf). Inference: 0x2000 draws the tick.
4. A separator is a widget with flags 1, style 0x1000010 and no text (0x458ea0..0x458ee7, 0x4591c1, 0x4593f3, 0x459549).

Items (all id 8, text into 0xde1f64 by 0x66f4e0, test of the filter bits = slot 0x18):

| Text | String | Test | Data |
|---|---|---|---|
| All Positions | 0x98ebe8 | bit 0x1 | 1 |
| Goalkeepers | 0x98ebd8 | 0x2 | 2 |
| Defenders | 0x98ebcc | 0x4 | 4 |
| Midfielders | 0x98ebbc | 0x8 | 8 |
| Attackers | 0x98ebb0 | 0x10 | 0x10 |
| (separator) | | | |
| All Sides, Left Sided, Central, Right Sided | 0x98eba4, 0x98eb98, 0x98eb8c, 0x98eb7c | 0x20, 0x40, 0x80, 0x100 | the same bits (inference from the pattern) |
| (separator) | | | |
| Available Only | 0x98eb6c | 0x1000 | 0x800 ticked, 0x1000 not (0x459211, 0x45925b) |
| Unavailable | 0x98eb40 | 0x2000 | 0x800 ticked, 0x2000 not (0x4592d0, 0x459321) |
| (separator) European Squad | 0x98eb30 | 0x8000 | not read |
| (separator) Include Seniors / Include Reserves | 0x98eb1c / 0x98eb08 | 0x400 | 0x200 ticked, 0x400 not (0x4595a8, 0x4595fe). Only when 0x600c90(club, 0, 1) (0x45950e) |

How a chosen item reaches the handler: in popup mode, the hover handler 0x560c30 returns the widget under the mouse (display inventory). The screen loop gives that widget index to the input function. The input reads the id with `0x468b40(widget)` (thiscall scrman, widget +0x08, s16; when the id is 0 it returns s16 `[0xdd6bde]`, 0x468b79..0x468b81) and the data with `0x468b90(widget)` (widget +0x48). For id 8 it tests and sets the filter bits in slot 0x18 with `0x7ff270(0x18, bits, 0)` and returns -0xb (0x467e93..0x468073). The Display widget table is `D + 0xba95e + w*0x18c`; id at +0x08 (`D + 0xba966`), data at +0x48 (`D + 0xba9a6`).

The View drop-down (id 7, data = view bit, 0x456c9a onwards) works the same way.

## 3. Least invasive change

Two wraps. No change to the game code or its data.

### 3a. Wrap the cell button 0x8040a0 at one call site

- Add to `tools/recomp/callsites.py`: `"squad_clear": (0x458AD0, 0x8040A0)`. The runtime compares `rd32(g_mem, cpu.esp)` with `rt_site_squad_clear` (0x458ad5), as columns.c does for 0x804100.
- `RT_WRAP(008040a0, cell_button)`. Signature: thiscall, ECX = 0xb74450, 11 stack args: `(area, column, row, style, colour, text_flags, font, text_colour, text, id, data)`, `ret 0x2c`, widget index in AX. `rt_arg(8)` = text, `rt_arg(9)` = id, `rt_arg(10)` = data.
- At the site, when slot 7 has no 0x800 (the Penalty Takers view: leave the button as it is, decision for Max):
  1. Keep the original data (0 Clear Squad, 1 Undo Clear).
  2. Write "Assistant" into emulated memory and put its address into arg 8. Set arg 9 (id) to 0. Call `o_008040a0()`. The result `(int16_t)cpu.eax` is the opener widget W.
  3. Display D = `rd32(0xb74c4c + (int16_t)rd16(0xb77c7a) * 0x300)`. Call `0x55fba0` thiscall on D with 11 args `(0,0,0,0, 0, 0, 0, 0, 0x130, rd16(0xae31aa), W)`. Result (s16) = popup area P.
  4. Four calls of `0x55f990`, thiscall on D, 18 args: `(2, 0,0,0,0, 0, row, 0x10, rd16(0xae31aa), rd16(0xaebdec), 1, 1, rd16(0xaebdea), text, 0, id, data, P)`. Row 0 is what the Filter items pass. Inference: the popup lays out its rows from the order of creation. Items:
     - "Pick best team": new id, data 1.
     - "Pick most match-fit team": new id, data 2.
     - "Pick under-21 team": new id, data 3.
     - "Clear squad": **id 0x25, data = the original data**. The original handler then runs unchanged, with undo. Inference: when the original was Undo Clear (data 1), the item should read "Undo clear" (decision for Max).
  5. Put the registers back and return W in EAX with `ret 0x2c` semantics. The simplest way: let `o_008040a0` do the stack return in step 2, then make the extra calls with `rt_callback` and a saved `cpu`, as `call_this` in columns.c does, and set `cpu.eax` to W at the end.
- The calls through `rt_callback` go through the display.c wraps of 0x55fba0 and 0x55f990, so widescreen scaling applies as for the Filter popup. The rects are 0, so nothing moves.

Text in emulated memory. The widget create copies the text: the screen writes "Clear Squad" to 0xde1f64 and then "Filter" to the same buffer before any draw (0x458a96, 0x458ad5). So a short-lived buffer is enough. Two ways that the runtime already uses:
- `rt_alloc_str(const char *)` in `src/recomp_rt/heap.c` (line 227). It allocates on the emulated heap and copies. testrun.c line 48 uses it. Allocate the five strings once and keep them.
- The scratch end of the text buffer, `SCRATCH = 0xde1f64 + 1600` with `scratch_keep`/`scratch_restore` and `win_put_str` (columns.c). Do not use the start of 0xde1f64: at the wrap it holds the caller's text.

### 3b. Wrap the input function 0x4671f0

- `RT_WRAP(004671f0, squad_input)`. Signature: cdecl, one arg, widget index s16 in `rt_arg(0)`. Returns EAX.
- It is called through the node pointer. RT_WRAP catches indirect calls too: columns.c wraps the node build 0x899ea0 the same way.
- In the wrap: `w = (int16_t)rt_arg(0)`. When w != -1, D as in 3a, `id = rd32(D + 0xba966 + w*0x18c)`, `data = rd32(D + 0xba9a6 + w*0x18c)`. When id is the new id and node slot 4 == 1, read club = node slot 0 and team = node slot 1 with `0x7ff010` (thiscall scrman, 1 arg, `ret 4`, columns.c `node_slot`). Call the picker. Then `rt_ret_cdecl(0xfffffff5)` and do not call `o_004671f0`. In all other cases call `o_004671f0()`.
- Note: w = -1 arrives after a pop (screen-stack inventory, -9 case). Pass it through.

New id: the squad tab handles ids 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x23, 0x24, 0x25, 0x7d7, 0x7dc (0x467d9d..0x4686b6). Before the tab switch it tests 1..6, 0x1f, 0x21, 0x22 (0x4673b5..0x467c80). 0x468c10 tests 0x1d, 0x1e, 0x26, 0x27. Names use 0x7d5..0x7e7 (0x7643d0, names.c). The left bar uses 0x3e8 and -2. An unknown id falls to 0x4687c1 and gives -0xb. Inference: an id such as 0x60 is free. The wrap catches it before the original, so the original never sees it.

Picker writes (from 2026-10-14-assistant-and-days-inventory.md): all thiscall on `[0xde1d08]`. `0x896fe0(club, team, 1, 0)` clears and saves an undo copy. `0x896a90(club, slot, player, team)` sets one slot. `0x897760` sets the captain. Inference: a picker that first calls `0x896fe0(club, team, 1, 0)` gets "Undo Clear" for free after a pick, because the undo copy then exists.

## 4. Redraw after a change

The input returns -0xb (0xfffffff5) after Clear Squad (0x468122) and after every filter or view change. The main loop handles -0xb as "rebuild the current screen" (0x7fd3a9, screen-stack inventory). The rebuild runs 0x454900 again, which reads the new selection. No flag or refresh call is needed: the wrap returns -0xb.

## 5. Which club, and whose

- The club is node slot 0 (builder 0x456b2d, input 0x467cd3).
- The button exists only when `0x600c90(club, 0, 1)` is not 0 (0x458a37). 0x600c90 is cdecl, in human_manager.cpp (assert string 0x9cf11c). It returns true when the club's manager pointer at club +0xcf equals the key of the current channel record `[0xb74c26 + s16[0xb77c7a]*0x300]` (0x600caf..0x600cd2). Further branches for arg 2, arg 3 and the reserve control flag (0x5fd480) were not read in full (unknown). Inference: on another club's squad screen the button is not built, so the Assistant button is also absent there, and the wrap of 3a never runs.
- The same test gates "Include Seniors/Reserves" (0x45950e) and the save of the view settings (0x456c0a).

## Unknown

- Arg 2 of 0x456900.
- The exact layout rule of popup rows (all Filter items pass column 0 and row 0).
- The full conditions of 0x600c90 beyond the club +0xcf test.
- Whether the hover handler 0x560c30 closes the popup before the input runs. The Filter items work, so the same path works for new items (inference).
