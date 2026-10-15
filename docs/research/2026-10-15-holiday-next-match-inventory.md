# Holiday until next match: inventory

Research for feature #30: a holiday that ends by itself the day before the next fixture of the player's club. The original is `clean-3968/cm0102.exe` (3.9.68). All addresses are in the original. "Inference" marks a statement that the code does not show directly. "Unknown" marks what I could not find. No run was made for this inventory. All facts come from static reads (REA and `objdump`).

## Summary

- Go on Holiday is item id 0x3ef of the left bar popup (build `FUN_0075db00`, 0x75f141..0x75f1d4). The side bar input `FUN_007615d0` pushes the confirm screen (0x7635d8). Yes on the confirm only sets the result globals. The side bar input then starts the holiday on its next pass with `FUN_005fd2f0(1, 0)` and a new stamp (0x7624c1..0x762513).
- The setter `FUN_005fd2f0(value, manager)` (thiscall on the human manager object `[0xb63c98]`, `ret 8`) writes the flag +0x9a01 and the count of humans on holiday. Value 1 also hands the manager's pending news to `FUN_007874b0`. The original itself calls it with 0 from the match processing at 0x6b4663.
- "Return from holiday ?" is not a popup item. On holiday the left bar makes a catch-all widget id 0x3f0 (0x75f218..0x75f272). A click clears the flag at once with `FUN_005fd2f0(0, 0)` (0x76365f) and asks. No sets the holiday again. Yes restores the team selection that was saved at the start (setting 3).
- The next fixture of a club comes from `FUN_005acbe0`, the fixture search that the squad screen uses (0x454d01). It draws no random numbers. It allocates list nodes. It can create a year block of the fixture store when the date range reaches a year that has none (`FUN_005abf10`, 0x5abf5e).
- The holiday has no length and no start date in the game data. The only extra state is the wall-clock stamp +0x9a65 and the count at `[0xb63c98]+0xc`. I found no code that counts holiday days.

## 1. How Yes on "Go on holiday ?" starts the holiday

### 1a. The path

1. The popup item "Go on Holiday" is widget id 0x3ef, built by `0x55f990` at 0x75f1d4 (args at 0x75f141..0x75f185). The item is built only when `FUN_005fd210` (on holiday) returns 0 (0x75f101). It is greyed (id 0, text flags 0x21) when `[0xb74c36 + slot*0x300]` != 0 or a local flag of the build is set (0x75f12f..0x75f1ce). Inference: the first is a network state.
2. The click reaches `FUN_007615d0` (cdecl, one arg: widget s16). At 0x7635d1 id 0x3ef calls `FUN_006b2d60` and returns -4 (0x7635e0).
3. `FUN_006b2d60` pushes the confirm screen: `0x7fe600(build 0x6b2d80, input 0x6b3440, 1, 0, 0)` (0x6b2d60..0x6b2d75).
4. The confirm input `FUN_006b3440` (0x6b3440). Button ids come from the build 0x6b2d80: "No" (0x98e8cc) is id 1, "Yes" (0x98e8d0) is id 2.
   - id 1, No: `[0xdd6be0]` = 8, `[0xdd6be4]` = 1, return -9 (0x6b3490..0x6b34ac).
   - id 2, Yes: `[0xdd6be0]` = 8, `[0xdd6be4]` = 2. Then it writes settings 2 and 3 again with their own values: `FUN_005feb60(2, FUN_005fea50(2, 0), 0)` and the same for 3 (0x6b34e8..0x6b353b). Return -9.
   - id 3, a setting line: a jump table on the widget data 0..6 (0x6b35c1, table 0x6b36c8) toggles the settings with `FUN_005feb60`. Return -0xb (rebuild).
5. After the pop, the screen below runs its input with -1, which calls `FUN_007615d0` again. At 0x7623a6 it reads the result (only when `[0xb77c84]` != 0, 0x76239a, meaning unknown):
   - `[0xdd6be0]` == 8: it clears it. When `[0xdd6be4]` == 2 (Yes):
     - Network case: `[0xb74c24 + slot*0x300]` != 0 and screen depth (`0x7feb70`) <= 1: it asks "Exit the network game as you go on holiday ?" (0xa5aa44) with code 9 (0x7623da..0x7624c0). The answer is handled at 0x762534 with the same calls as below.
     - Otherwise, at 0x7624c1:
       1. `FUN_005fd2f0(1, 0)`: the flag (see 1b).
       2. `FUN_005fdd30(ms clock 0x90c510, 0)`: the holiday stamp +0x9a65 (0x7624cf..0x7624dc).
       3. `0x800000` thiscall on the screen stack 0xb74450 (0x7624e6): it sends display packet 0x11 (0x80008e). Inference: a redraw of the left bar.
       4. When the manager has a club (`[manager + 0x39]` != 0): `0x84abe0(club)` thiscall on `[[0xde1f54] + 0x856]` (0x762507..0x762513). Purpose unknown. It builds a list and calls 0x844660 and 0x8420e0. 0x84abe0 is also a caller of `FUN_005fd210`.
   - Then `[0xdd6be4]` = 0 (0x762520).

### 1b. The flag setter `FUN_005fd2f0`

Thiscall, ECX = the human manager object `[0xb63c98]` (+0 = base of the records, +4 = number of records, +0xc = count of humans on holiday). Stack: (value, manager), `ret 8`. Manager 0 means the manager of the current player slot `[0xb74c26 + s16[0xb77c7a]*0x300]` (0x5fd303..0x5fd310).

- Record index = `[manager] - [0xae2364] + 0x10`, record = base + index*0xa6ef (0x5fd34d..0x5fd38e).
- When the old flag differs, the count at object +0xc goes up (0 to 1) or down (1 to 0) (0x5fd395..0x5fd3c1).
- Record +0x9a01 = value (dword, 0x5fd3c6).
- Value != 0 only: `FUN_007874b0(manager, club)` thiscall on `[0xb7431c]` for `[manager+0x39]` (club) and for `[manager+0x24]` (inference: the national team) (0x5fd3ca..0x5fd3ed). `FUN_007874b0` (news.cpp, assert 0xa693ec) walks the manager's news items and calls `FUN_00784100(item, club)` for each item whose +0xd6 is set (0x787584..0x7875c3). Inference: the assistant deals with news items that wait for an answer.
- Always: when `[0xb67a50]` != 0, `FUN_006b5040` thiscall on it (0x5fd3f2..0x5fd3fc). `[0xb67a50]` is the match day object (match_day.cpp). `FUN_006b5040` first calls `FUN_005fe8d0` on `[0xb63c98]` and then resets per-match fields (0x6b5058..0x6b50be). Inference: it works out again which human watches which match.
- It does not write the stamp +0x9a65 or the settings.

### 1c. The settings and the saved selection

- `FUN_005fea50(index, manager)`: byte at record +0x9b23 + index (0x5feadc), `ret 8`.
- `FUN_005feb60(index, value, manager)`: writes that byte (0x5fec19), `ret 0xc`. When index is 3, value != 0 and the manager has a club, it copies the club's first-team selection from the tactics object `[0xde1d08]` (`0x896660`, `0x8968f0`) into record +0x9b28, 8 bytes per slot (0x5fec2e..0x5fecae). So Yes with "Use current team selection when possible" on saves the selection at the start of the holiday.
- The save writes +0x9a01, +0x9a65, +0x9b23.. and +0x9b28.. with the field writer `0x932c30` (0x5ff1a9, 0x5ff242, 0x5ff3a1, 0x5ff418). So the flag, the stamp, the settings and the saved selection are in the saved game.

## 2. How a holiday ends in the original

### 2a. "Return from holiday ?"

On holiday the left bar build makes no popup item for it. At 0x75f208..0x75f272, when the screen depth `0x7feb70` <= 1, it makes a widget with flags 1, rect 0, text "", id 0x3f0, area -1 (args 0x75f21e..0x75f25b) and passes it to `0x800160` on the screen stack (display packet 0x13, 0x8001e6). Inference: this makes it the default widget of the screen, so a click or key anywhere on holiday selects it.

`FUN_007615d0` at 0x763622, id 0x3f0:
1. `FUN_005fd2f0(0, 0)` at 0x76365f: the holiday ends at once (flag 0, count down, `FUN_006b5040` when there is a match day object). The stamp, the settings and the saved selection stay.
2. The generic confirm `FUN_00591840(..., "Please Confirm", "Return from holiday ?", 1, colours, code 0x2a)` (0x7636b6..0x7636ea). Its input 0x591dc0 writes `[0xdd6be0]` = the code and `[0xdd6be4]` = the button (0x591e89..0x591e8e). No is id 1, Yes is id 2 (build 0x591d37..0x591da0).
3. On the next pass, `FUN_007615d0` at 0x76259a, `[0xdd6be0]` == 0x2a:
   - No (1): `FUN_005fd2f0(1, 0)`, a new stamp `FUN_005fdd30`, `0x800000` (0x7625c3..0x7625ed). The holiday goes on. `FUN_007874b0` runs again inside the setter.
   - Yes (2), when the manager has a club and setting 3 is on (0x7625f7..0x762610): it restores the selection saved in 1c. `FUN_005fed50(manager)` gives record +0x9b28 (0x5fedd1). `0x896fe0(club, 0, 0, 0)` clears the stored selection. Then for each of 16 slots, when the player index is > -1 and the player's club (`[0xae23bc] + index*0x6e`, +0x39) is still this club, `0x896a90(club, slot, player, 0)` (0x762612..0x76267b). All thiscall on `[0xde1d08]`.
   - Then `[0xdd6be4]` = 0.

So the full end of a holiday is: `FUN_005fd2f0(0, manager)`, then the restore block when setting 3 is on. Nothing else is reset.

### 2b. Other ends in the original

Callers of `FUN_005fd2f0`: 0x6b4360, 0x7615d0, 0x827110, 0x82a9c0.
- `FUN_006b4360` (match processing) at 0x6b4663: for a fixture of a club whose manager is not in control, when `FUN_005fe290(fixture+4, manager)` is true and the manager is on holiday, it ends the holiday with `FUN_005fd2f0(0, manager)` and calls `0x727f00` (0x6b4633..0x6b4685). `FUN_005fe290` compares the fixture with a copy of a fixture in the record at +0x9ad4 (0x4f bytes: date +0x9afc/+0x9afe, slice +0x9b13, clubs +0x9ae0/+0x9ae4, 0x5fe330..0x5fe39e). Inference: this is a match that the manager chose to watch. The holiday ends when that match is played. This is the original's own precedent for an end of holiday from the day loop.
- 0x827cc9 in `FUN_00827110` and 0x82ba04 in `FUN_0082a9c0`: not read. Inference from the address range: game load or network code. Unknown.

### 2c. Can the runtime call `FUN_005fd2f0(0, manager)` safely?

- From the human turn wrap (`RT_WRAP(005cde40, ...)` in `src/recomp_rt/win/testrun.c`), before `o_005cde40()`: yes (inference). The original calls it from the same place, because the side bar input runs inside the screen loop of the human turn. The day's matches and daily updates are done (assistant-and-days inventory, 2a). It is thiscall: ECX = `[0xb63c98]`, two stack args, `ret 8`. Use a thiscall helper as `call_this` in `columns.c`. When the flag is clear before `o_005cde40()` runs, the screens of that turn are built without the holiday hold and without widget 0x3f0, so the player has control.
- Do also the restore block of 2a (setting 3) with `rt_callback`: `0x5fed50` (thiscall `[0xb63c98]`, arg manager), `0x896fe0`, `0x896a90` (thiscall `[0xde1d08]`).
- From the day loop: the original does it inside `FUN_006b4360` (2b), but that is inside the fixture loop of a match day. A runtime call there would need a wrap inside that loop. The human turn is simpler and runs before the next slice's matches. I do not recommend the day loop.
- `FUN_006b5040` runs only when `[0xb67a50]` != 0. Whether that object exists during the human turn is unknown. The original's own Return path has the same condition.

## 3. The next fixture of a club

### 3a. Where the squad screen gets it

The squad screen build `0x454900` (no function in Ghidra, read with objdump) finds the last and the next match and puts copies into node slots 2 and 3. The "Next Match" tab (tab 3, string 0x98e874 at 0x454f55) is drawn by `0x45ccf0`, which reads node slot 3 (0x45cd80..0x45cd8e) and the fixture date at +0x28/+0x2a (0x45ce78).

Next match, 0x454c4c..0x454d65:
1. End date = today + 8 days, or + 24 days when `FUN_0053b900(club)` is true (0x454c56..0x454cc9). Inference: `FUN_0053b900` is true for a national team. The date add is `0x54c770` thiscall on the game date 0xae2c90, args (out, days).
2. `FUN_005acbe0(today_day, today_year, end_day, end_year, club_id, &list, -1, -2, team_flag)` thiscall on `[0xae2a58]` (0x454cd1..0x454d01). `club_id` = `[club]`. `team_flag` = 1 when node slot 1 == 0 (`sete` at 0x454cd6). Inference: 1 = the first team, from the fixture fields +0x40/+0x41.
3. It walks the list from the head (`[list+2]` into the cursor `[list+0xa]`, then `0x68d1b0`) and takes the first fixture with +0x43 == -1 (0x454d15..0x454d33). Inference: +0x43 is the result, -1 = not played yet.
4. It copies it (0x4f bytes, `0x944cf1` malloc) into node slot 3 (0x454d46..0x454d65).
5. The list is freed by `0x68d040` at 0x456704.

### 3b. `FUN_005acbe0`

Thiscall, ECX = the fixture store `[0xae2a58]`, 9 stack args (pseudocode signature at 0x5acbe0):

| Arg | Meaning |
|---|---|
| 1, 2 | start day of year (u16, < 0x16e), start year |
| 3, 4 | end day, end year (inclusive) |
| 5 | club id, or -1 for all clubs |
| 6 | list object to fill (0 = only count) |
| 7 | competition filter (fixture +0) or -1 |
| 8 | filter on fixture +0x42, or -2 for any |
| 9 | team flag that the club's side must have (+0x40 home, +0x41 away), or -1 |

Return: s16 count of matching fixtures. Order: day by day, slice 0..2 within a day, then by competition type (loop 0x5acd82). So the first unplayed fixture in the list is the next match.

Fixture record (0x4f bytes, from 0x5acbe0, 0x5fe290, 0x5fe5df and 0x45ce78): +0 competition, +0xc home club id, +0x10 away club id, +0x28 s16 year, +0x2a s16 day of year, +0x3f slice, +0x40/+0x41 team flags, +0x42 unknown, +0x43 result (-1 not played). The game date is s16 day at 0xae2c90 and s16 year at 0xae2c92 (0x5fe5df..0x5fe5f8). The slice is `[0xae2c88]`.

List object (0xe bytes): +0 s16 count, +2 head, +6 tail, +0xa cursor. Constructor `0x68d020`, destructor `0x68d040` (frees each node with `0x68d010` and `0x944c94`), append copy `0x68d0d0(data, size)`, next `0x68d1b0`, previous `0x68d1d0`. Node: +0 data, +6 next, +0xa previous.

### 3c. Side effects

- Random numbers: none. The callees of `0x5acbe0` are 0x53b900, 0x48dee0, 0x5af5b0, 0x48dd70, 0x5abf10, the list functions and the error box. None calls `0x90cfc0`. The year block constructor 0x5a9cf0 and 0x5aa2d0 were not traced to the end.
- Allocations: one list node and one 0x4f copy per fixture found (`0x68d0d0`). They are freed by `0x68d040`. They move later heap addresses. Inference: no effect on game state, because the squad screen does the same at every build.
- Writes to the store: for each (day, slice, type) list it reads, it sets that list's cursor (+0xa) to its head (+2) (pseudocode, after the `FUN_005abf10` call). Inference: harmless outside a walk of the same list. Do not call it while the game walks fixtures, for example inside `FUN_006b4360`.
- Year blocks: `FUN_005abf10(year, day, slice, type)` walks the chain of year blocks (+0x1130 s16 year, +0x112c next, the first block is the store object itself). When the year is missing it allocates a new block of 0x1184 bytes and runs its constructor 0x5a9cf0 (0x5abf5e..0x5abf9a). That is a lasting change to the store. Whether it is saved, and whether the original makes these blocks elsewhere ahead of time, is unknown. The runtime should walk the chain itself and end the search range at the last year that exists.

## 4. The popup that holds Go on Holiday

### 4a. The build

`FUN_0075db00(mode, ?)` builds the whole left bar (Menubar.cpp, assert 0xa5a51c). Each popup item is one `0x55f990` widget create, thiscall on the Display `[0xb74c4c + slot*0x300]`, 18 args, `ret 0x48` (display inventory). The items near Go on Holiday, in build order:

| Text | Id | Create |
|---|---|---|
| News | 0x3e9 | 0x75ee97 |
| Player & Staff Search | 0x3eb | 0x75eefb |
| Compare two chosen players | 0x434 | 0x75ef5f |
| Manager Stats | 0x42e | 0x75efc3 |
| Job Information | 0x41e | 0x75f027 |
| Transfers | 0x415 | 0x75f08b |
| History | 0x3ec | 0x75f0f4 |
| Go on Holiday | 0x3ef (0 when greyed) | 0x75f1d4 |
| Send Abuse To ... | 0x414 | 0x75f2e6.. |
| Retire | 0x3f2 | 0x75f50b |

Which items belong to the popup that the click at 45,165 opens: inference, from News to Retire. Not checked in a run.

Args of the Go on Holiday create (0x75f141..0x75f185), in call order: `(2, 0,0,0,0, 0,0, 0x10, u16[0xae31aa], u16[0xaebdec], 1, 1, u16[0xaebdea], text 0xde1f64, 0, 0x3ef, 0, area ESI)`. Greyed: text flags 0x21 and id 0 (0x75f18d..0x75f1ce). Inference (squad drop-down inventory): rows follow the order of creation, so a widget made right after this one appears right below it.

### 4b. How to add "Holiday until next match"

1. Add a call site to `tools/recomp/callsites.py`: `"holiday_item": (0x75F1D4, 0x55F990)`. The return address is 0x75f1d9.
2. In the widget create wrap (`display.c`, `RT_WRAP(0055f990, widget_create)`), when the return address is `rt_site_holiday_item`: copy the 18 args before `o_0055f990()` (the callee pops them), let the original make Go on Holiday, then make a second widget with the same args, the new text, and the new id (or id 0 when the copied id is 0, so it is greyed in the same cases). Both go through the scaling of the wrap. Keep EAX of the first call for the caller. Text: `rt_alloc_str` once, as the squad drop-down does.
3. The item exists only when the manager is not on holiday, as Go on Holiday.

### 4c. How the click is dispatched

The input of each screen calls `FUN_007615d0(widget)` (cdecl). It reads the id at `D + 0xba966 + w*0x18c` (0x7634a5..0x7634ab) and falls back to `s16[0xdd6bde]` when the id is 0. It compares the id with each left bar id and returns -4 after an action. Wrap it: `RT_WRAP(007615d0, ...)`. When w != -1 and the id is the new id, set the runtime flag "pending", call `0x6b2d60` (no args, plain `ret`) and return -4 without the original. All other cases call the original.

New id: 0x4b7. No instruction in the exe has 0x4b7 as an immediate (objdump of the whole exe). The ids handled by `FUN_007615d0` are 0x3e8..0x434, by `FUN_007643d0` 0x426 and 0x7d0..0x7e7.

## 5. Does anything count holiday days?

- The record holds the flag +0x9a01, the stamp +0x9a65 (ms clock, written by `FUN_005fdd30`, read only at 0x762245) and the settings +0x9b23... `FUN_005fd2f0` writes no date. I found no holiday start date and no day count.
- The count at `[0xb63c98]+0xc` is the number of humans on holiday. Its readers are 0x82092b and 0x820b34: a screen that keeps it in node slot 4 and rebuilds when it changes. Inference: display only.
- The 31 callers of `FUN_005fd210` (list from REA) read the flag only. The ones read here: the human turn `FUN_005cde40` (0x5cdf0b: a human on holiday gets no news screen in its first branch), the match code (assistant-and-days inventory, 1a and 1d), the left bar build (0x75f101) and input (0x762231 hold).
- So a holiday differs from Continue Game only while the flag is set. Once the flag is clear, nothing marks that a holiday took place, except the stamp and the selection snapshot, which only holiday code reads. This is not a proof: the other callers of `FUN_005fd210` were not read in full.

## 6. Recommended design (inference, not tested)

Wraps:
1. Widget create `0x55f990` at call site 0x75f1d4: add the item (4b).
2. Side bar input `0x7615d0`: id 0x4b7 sets `pending` and pushes the original confirm `0x6b2d60`, so the player sees and sets the same settings. Id 0x3ef clears `pending`. When `[0xdd6be0]` == 8 and `[0xdd6be4]` == 1 (No) on entry, clear `pending`.
3. Holiday setter `0x5fd2f0` (thiscall, `ret 8`): after the original, value 1 with `pending` set gives `auto[slot]` = 1 and clears `pending`. Value 0 clears `auto[slot]`. So Return from holiday and the match-watch end (2b) also end the auto mode.
4. Human turn `0x5cde40` (testrun.c): before `o_005cde40()`, when `auto[slot]` is set and `FUN_005fd210` is true for the current manager:
   - Club = `[manager+0x39]` (else `[manager+0x24]`). Run `0x5acbe0` from today to today + N days, with the end clipped to the last year block (3c), team flag 1. Take the first fixture with +0x43 == -1. Free the list.
   - When that fixture's date is today or tomorrow (`0x54c770(out, 1)` on 0xae2c90 gives tomorrow), end the holiday: `FUN_005fd2f0(0, manager)`, then the restore block of 2a when setting 3 is on. Clear `auto[slot]`.
   - When no fixture is found, the holiday goes on and the check runs again at the next turn.

Why the human turn and "today or tomorrow": the day loop plays the slice's matches before the human turn of that slice (assistant-and-days inventory, 2a). A check at the first human turn of the day before the match leaves the player at least the turns of that day. A check on the match day itself can be too late for a morning match.

Decisions for Max:
- `auto` is runtime state. It is not in the saved game. After a load during such a holiday, the holiday goes on with no end, as in the original. A field in the record would change the save format.
- Holiday started on the day before the match or on match day: end at the next human turn (my pick), or grey the item.
- N, the search range (for example 60 days), and what to do when the season has no more fixtures.

Same-save check: with the feature not used, none of the wraps changes anything. With it used, the human turn calls `0x5acbe0`, which allocates on the emulated heap. Inference: no game result changes, but heap addresses later in the run do.

## Unknown

- The meaning of `[0xb77c84]`, which gates the confirm handling at 0x76239a.
- The purpose of `0x84abe0` at the start of a holiday.
- Whether `[0xb67a50]` is set during the human turn.
- Whether the year blocks that `FUN_005abf10` can create are saved, and when the original makes them.
- `FUN_00827110` and `FUN_0082a9c0` (callers of `FUN_005fd2f0`).
- The exact items of the popup at 45,165, and the row of the new item, until a run with `RECOMP_SCREEN_LOG=1` checks them.
- The fixture fields +0x42 and the full meaning of +0x40/+0x41.
