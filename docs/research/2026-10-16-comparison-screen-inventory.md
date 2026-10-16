# "Compare two chosen players": inventory of the original screen

Date: 2026-10-16. Source: `clean-3968/cm0102.exe`, read with `i686-w64-mingw32-objdump` (Ghidra has no function at 0x876520). Every fact has its address. "Inference" marks what the code does not show directly. "Unknown" marks what I could not find.

Terms and helpers from earlier files: area create `0x55fba0` and widget create `0x55f990` (`2026-10-15-team-views-frame-inventory.md`), label `0x804100(area, column, row, style, colour, text_flags, font, text_colour, text)` thiscall on S = 0xb74450, button `0x8040a0`, heading `0x803f30`, side bar `0x75db00`, Back bar `0x5edcc0`, push `0x7fe600(build, input, flag, leave, exit)`, slots `0x7ff010`/`0x7ff270`. The text with links is a comp_text object: constructor `0x4ae590`, set text `0x4ae660`, add link `0x4ae8a0(text, link_type, data)`, clear `0x4afa40`, draw `0x4aeaf0(area, x1, x2, &y, font, colour, flags, 0)`, destructor `0x4ae630`.

## 1. Where the two players are stored

### Storage

The two players live in the scout manager object, not in node slots. The object pointer is the global `[0xb74448]`.

- Getter `0x7f7920(n)`: thiscall on `[0xb74448]`, `ret 4`. It takes the current human manager `[0xb74c26 + s16[0xb77c7a]*0x300]` and computes `k = byte[manager] - byte[0xae2364] + 0x10`. With k in 0..15 it returns `[this + 0x68 + k*4]` for n == 1, else `[this + 0xa8 + k*4]` (0x7f796c..0x7f79a8). Out of range: error box, scout_manager.cpp line 0x304, returns 0.
- So each of the 16 human managers has his own player 1 (array at +0x68) and player 2 (array at +0xa8). The values are staff record pointers (0xae23bc + id*0x6e). Inference: `[0xae2364]` is the staff count and the human managers are its last 16 rows.
- Setter `0x7f7780(staff, manager, which)`: thiscall on `[0xb74448]`, `ret 0xc` (0x7f7875). Manager 0 means the current human manager (0x7f77c7..0x7f77e7). Rules (0x7f782d..0x7f7891):
  - The staff is player 1 and `which` == 2, or he is player 2 and `which` == 1: the two entries swap (0x7f7856..0x7f7862).
  - Else `which` == 1 writes +0x68, any other value writes +0xa8.
  - So the two entries can never hold the same player through this setter.

### The prompt screen 0x8f41b0

`0x8f41b0(staff*, 1, which)`, cdecl, plain `ret`. It pushes a confirm screen: `0x7fe600(build 0x8f4210, input 0x8f44d0, flag 1, leave 0, 0)` (0x8f41b0..0x8f41c5). Then it writes slot 0 = staff id (`[staff]`, 0x8f41de..0x8f41ed) and slot 1 = (`which` == 1) (0x8f41d4..0x8f41fc).

Its build 0x8f4210:

- Heading: the current manager's name `0x53dba0(manager, 0xde1f64, 0x7d0, 1)` in his club's colours `0x53b640(club, 1, ...)` (club = manager+0x39, else +0x24) (0x8f4266..0x8f42d3).
- Sub heading `0x803f80`: "Set as 1st player in comparison?" (0xaa0a48) or "Set as 2nd player in comparison?" (0xaa0a24) (0x8f42d8..0x8f4301).
- Text box `0x5ed070(110, 140, 780, 545, ...)` with "Set <name> as 1st/2nd player in comparison?" (0xaa09dc, 0xaa0994) and a player link 0x7d7 (0x8f431b..0x8f438b).
- Side bar `0x75db00(4, 0)` (0x8f4394).
- Button area (100, 555)-(790, 590), 2 columns (0x8f43d1). Buttons (0x8f4416..0x8f44b1):

| Slot 1 | "No" (column 0) | "Yes" (column 1) |
|---|---|---|
| 1 (1st player) | id 2 | id 1 |
| 0 (2nd player) | id 0xb | id 0xa |

Its input 0x8f44d0: id 2 or 0xb returns -9 (pop) (0x8f451a, 0x8f4559). Id 1 calls `0x7f7780(staff, 0, 1)`, id 0xa calls `0x7f7780(staff, 0, 2)`, then both return -9 (0x8f4594..0x8f45c8, 0x8f4606..0x8f462c). Other ids go to `0x7615d0`, then `0x7643d0` (0x8f462e..0x8f463c).

### Where the prompts come from

Only the player profile. `0x8f41b0` has two callers, both in the profile input 0x888260:

- Id 0x2f: `0x8f41b0(staff, 1, 1)` (0x888e74..0x888e7f).
- Id 0x30: `0x8f41b0(staff, 1, 2)` (0x888ec1..0x888ecc).
- Id 0x31: open the comparison (0x888f0e..0x888ffe). When player 1 and player 2 are both set and differ, it pushes the compare screen directly, with the same arguments as 0x870650: `0x7fe600(0x876520, 0x889c30, 0, 0x88b8a0, 0)` (0x888fcc..0x888fe4). Else it pushes the message screen `0x88c460/0x88c660` (0x888fee).

The profile build 0x870700 makes these items in its "Action" drop-down at (660, 4)-(785, 24) (button `0x804030` at 0x8745cd and 0x8747c1):

- A header item "Compare Players" (0xa85160), id 0 (0x874693..0x8746ef). It owns a sub popup area, flags 0x30 (0x874722).
- "Set as 1st player in comparison" (0xa850f4), or with the current player 1's name (0xa85114), id 0x2f (0x874729..0x874848).
- "Set as 2nd player in comparison" (0xa85088), or with the current player 2's name (0xa850a8), id 0x30 (0x87484d..0x87496b).
- "Compare two chosen players" (0xa5a5e8), id 0x31 (0x874970..0x874a20).
- Condition for the group (0x874658..0x87468d): the staff has player data (+0x61 != 0), his id < `[0xae2364]`, and `byte[[0xae2d0c] + id*0x4f + 0xb] & 0x30` != 0x20. The meaning of that last byte is unknown.

### Side bar handler 0x7633af (item id 0x434)

In the side bar input 0x7615d0 (0x7633af..0x763418):

- Both players set (not 0): `0x870650(manager, 1, 1, 1, 0)`, return -4.
- None or one set: `0x88c440(manager)`, return -4.
- The handler does not compare the two pointers. Inference: the setter swap keeps them different, so this is safe.
- The side bar item is not disabled. It always works and shows a message when a player is missing. Whether the side bar build greys it: not checked.

The message screen `0x88c440` pushes `0x7fe600(0x88c460, 0x88c660, 1, 0, 0)` (0x88c440..0x88c455):

- Heading: the manager's name in club colours. When `0x66f4d0()` returns 0 the heading is "Can not compare players" (0xa8b44c) (0x88c4cc..0x88c513). The meaning of 0x66f4d0 is unknown.
- When player 1 or player 2 is 0: the text box `0x5ed070(110, 140, 780, 545, ...)` says "Two players must be selected to compare against each other" (0xa8b410) (0x88c56b..0x88c5c3).
- Side bar `0x75db00(4, 0)` (0x88c5cc). "Okay" button id 0x38 in area (100, 555)-(790, 590) (0x88c609..0x88c64a).
- Input 0x88c660: id 0x38 returns -9. Else `0x7615d0`, then `0x7643d0`.

## 2. The push 0x870650

`0x870650(manager, a2, a3, a4, a5)`, cdecl, plain `ret` (0x8706ff). The side bar passes `(manager, 1, 1, 1, 0)` (0x7633e9..0x7633f2).

- When player 1 (`0x7f7920(1)`) is 0 and `manager` is 0: error box, staff_screens.cpp line 0x1ac, return (0x870663..0x8706db).
- Else: `0x7fe600(build 0x876520, input 0x889c30, flag 0, leave 0x88b8a0, exit 0)` (0x8706dc..0x8706f4).
- It sets no node slots. Arguments 2 to 5 are not read. The push allocates the node with calloc, so all 60 slots are 0 (screen-stack inventory, push pseudocode).
- Flag 0: Back (-2) works, as on the club screen.

Callers of 0x870650: only 0x7633f2 (the side bar handler). The profile id 0x31 path pushes the same screen inline (0x888fcc).

## 3. The build 0x876520

Frame: SEH, `_chkstk` 0x945610 for 0x2644 bytes (0x87652e..0x87653a). Return at 0x87a509. It is a copy of parts of the profile build: it reads slots 0..8 and 0x14 (0x8765c9..0x876645) and a tab entry array of 0x19 entries (0x9463af at 0x876572, destroyed by 0x9462bb at 0x87a4d8) that it never uses. All slots are 0 on this screen, so the slot tests below always take the "slot is 0" path.

Values read at the start:

- p1 = `0x7f7920(1)`, p2 = `0x7f7920(2)` (0x8765af, 0x8765c2). When p1 is 0 and p2 is 0, the build jumps to the end (0x87668f). When p1 is 0, slot 0 stands in (0x876697). Inference: unreachable.
- Three colours (0x87664a..0x876688):

| Name here | Normal (`[0xb74349]` == 0) | `[0xb74349]` != 0 |
|---|---|---|
| P1 colour | u16 `[0xae31aa]` | the same |
| Equal colour | u16 `[0xaebdec]` (yellow) | u16 `[0xae3152]` |
| P2 colour | u16 `[0xae31a8]` (red) | u16 `[0xaebdec]` |

`[0xb74349]` is set to 1 only at 0x7a3c31. Its meaning is unknown. The RGB of `[0xae31aa]` was not traced (other files call it the popup item colour).

### 3.1 Frame

| Part | What | Evidence |
|---|---|---|
| Background | `0x41c790(p1, 0, 0)`: the staff's picture as the screen title picture (CM3_PICTURES), as 0x41c250 does for a club | 0x8766a6, 0x41c7f4 |
| Heading bar | Area (100, 10)-(790, 70), 1x1, flags 0x30, colour `[0xae31a8]` (red), owner -1 | 0x87678c |
| Heading text area | Area (100, 25)-(790, 55), 1x1, flags 1, colour red | 0x8767da / 0x8767f8 |
| Heading text | "%s ~ %s" (0xa859d0) of `0x53dba0(p, buf, 0xc8, 1)` for p1 and p2. Each name is a link, type 0x7d7, data = staff (`0x4ae8a0`). Drawn by `0x4aeaf0(area, 100, 790, &y, font, [0xae3184] grey, 3, 0)`. Font 7 when `0x5e5ad0(4, text)` < 690, else font 6 | 0x8766bb..0x876819 |
| Side bar | `0x75db00(4, 0)` | 0x876823 |
| Age and nationality line | Area (110, 80)-(780, 110), 1x1, flags 2, colour 0. Text "%s. %s ~ %s. %s" (0xa859b0): "Age <n>" (0xa859c0, byte staff+0x18) and the nationality of each player. Drawn by `0x4aeaf0(area, 110, 780, &y, font 3, [0xaebdec] yellow, 3, 0)` | 0x87684d..0x876bfe |
| Positions line | (110, 515)-(780, 545): "%s ~ %s" of `0x53ee50(player_data, buf, 0x7d0, 1)` for p1 and p2, through `0x803fd0(110, 515, 780, 545, 2, 0, 0xc, 3, [0xaebde8], text, 1)` | 0x876c15..0x876c9a |
| Job line (instead) | When p1 has no player data, or slot 4 == 1: p1's job `0x53ad00(job, 0, 1, 0)` at the club (+0x3d, club +0x39), or the national team (+0x28, +0x24), or "Unemployed" (0x9b6b04), at the same place | 0x876ca4..0x876d58 |
| Back bar | `0x5edcc0(0x7fec60(0) == 0, 0x7febb0() == 0)` | 0x87a492..0x87a4b4 |

Nationality (0x87687e..0x876b0d): `0x6181d0(nation, buf, 0xc8, 0xc)` of staff+0x1a, first letter made upper case (`0x9471cd`). When staff+0x1e is set, differs from +0x1a, byte +0x22 is 0 and `[nation] != [0x9cf434]`, the second nation follows after "/" (0x9b7630). The first nation of each player is a link, type 0x7d5, data `0x53b3d0(nation)` (0x876b36..0x876ba9). Inference: mode 0xc gives the adjective ("English").

The call `0x54a650(p1+0x10, buf, 0xc8, 0xff)` at 0x876844 writes into the name buffer that 0x87784f overwrites later. Inference: dead code.

No heading picture, no tab bar, no "Print" or Action button.

### 3.2 Part A: position suitability (0x876d67..0x878fac)

Runs when p1 has player data (+0x61) and `p1.id < [0xae2364]` (0x876d67..0x876d8a). It reads `p2+0x61` without a test. Inference: p2 always has player data, because the profile menu offers the items only to players.

Step 1. For ten positions it starts a score from the position ratings (player data, 1..20) (0x876d90..0x877415). Rule for one rating byte r (al = 15):

- p1 >= 15 and p2 <= 15: score = 35 (0x23).
- p2 >= 15 and p1 < 16: score = -35 (0xffdd).
- Both > 15: score = 2*(p1 - p2).
- Both < 15 (no natural player): flag = 0, the row shows "n/a".

| Position | Rating test | Score word | Flag byte |
|---|---|---|---|
| Goalkeeper | +0x0f | 0xde1ce4 | 0xde1cfa |
| Centre Back | +0x11 | 0xde1ce6 | 0xde1cfb |
| Sweeper | +0x10 | 0xde1cec | 0xde1cfe |
| Full Back | +0x11 and (+0x17 or +0x18) | 0xde1ce8 | 0xde1cfc |
| Wing Back | +0x16 | 0xde1cea | 0xde1cfd |
| Def. Mid. | +0x12 | 0xde1cee | 0xde1cff |
| Midfielder | +0x13 | 0xde1cf0 | 0xde1d00 |
| Att. Mid. | +0x14 | 0xdda67c | 0xdda67e |
| Winger | (+0x13 or +0x14) and (+0x17 or +0x18) | 0xde1cf2 | 0xde1d01 |
| Striker | +0x15 | 0xde1cf8 | 0xde1d02 |

`[0xde1d04]` = 1 when both goalkeeper ratings are > 15 (0x876e99). Part B uses it.

Step 2. For attributes 1..31 (0x87741e..0x8777f1) it fills the shared cell tables (stride 0x65) and adds the difference d = p1 - p2 to the scores. A masked attribute adds nothing (0x877528).

- Getter: when `[0x9cc270]` != 0, `0x543440(viewer, staff, attr)` (cdecl, viewer = current manager), which returns 0xf9 for a masked attribute (0x5434c6). Else `0x543540(staff, player_data, attr)`, the raw shown value (0x877447..0x87751e). These are the same getters as the profile.
- Weights (×2 or ×1), attribute names by id (1 Acceleration .. 31 Work Rate):

| Score | ×1 | ×2 |
|---|---|---|
| Goalkeeper | Acceleration, Agility, Jumping, Pace, Passing, Positioning, Reflexes, Strength | Handling |
| Defender base 0xde1cf4 | Heading, Jumping, Pace, Positioning, Stamina, Strength | |
| Centre Back | (base) | Marking, Tackling |
| Full Back | (base) | Acceleration, Marking, Passing |
| Wing Back | (base) | Acceleration, Crossing, Passing |
| Sweeper | (base) | Marking, Passing, Tackling |
| Midfield base 0xde1cf6 | Acceleration, Determination, Heading, Jumping, Pace, Stamina, Strength, Teamwork | |
| Def. Mid. | (base) | Marking, Passing, Tackling |
| Midfielder | (base) | Dribbling, Passing, Stamina, Technique |
| Att. Mid. | (base) | Creativity, Dribbling, Long Shots, Off The Ball, Passing |
| Winger | (base) | Creativity, Crossing, Dribbling, Pace |
| Striker | Creativity, Crossing, Dribbling, Flair, Heading, Jumping, Off The Ball, Passing, Set Pieces | Finishing, Long Shots, Pace |

The two bases are added after the loop (0x8777f7..0x87783a).

Step 3. Layout from the bottom up. y values assume `0x5e5cd0(2)` = 18 in classic mode (seen in a run, attribute groups file). Inference for the exact pixels.

| Area | Rectangle | Content | Evidence |
|---|---|---|---|
| Summary | (110, 487)-(780, 505), flags 2; inner 3 columns, weights 3,3,3, 1 row | "<p1> better" (0xa85998) in P1 colour, "Both the same" (0xa85988) in Equal colour, "<p2> better" in P2 colour; font 2, text_flags 0xc | 0x877877..0x8779a2 |
| Legend | (110, 451)-(780, 487), 1 column, 2 rows | "Legend:" (0xa85980) grey; "( - ) indicates that this attribute is masked and requires further scouting" (0xa85934) orange `[0xaebdd4]` | 0x8779a7..0x877a98 |
| Grid | (110, 361)-(780, 451), 4 columns, 5 rows | Name, stars, name, stars. Rows: Goalkeeper / Def. Mid., Full Back / Midfielder, Centre Back / Att. Mid., Sweeper / Winger, Wing Back / Striker. Names grey, font 2 | 0x877a9d..0x878f91 |

The grid's column weights are bytes 3, 3, 3 and a fourth byte the build never writes (0x876587..0x87659b, 0x877af0). Inference: an uninitialised stack byte.

Stars for score s (0x877b55..0x877d1e, the same for every position):

| s | Text | Colour |
|---|---|---|
| < -36 | "  *****" (0xa8591c) | P2 colour |
| -36..-28 | "  ****" | P2 |
| -27..-20 | "  ***" | P2 |
| -19..-13 | "  **" | P2 |
| -12..-7 | "  *" | P2 |
| -6..5 | "  -" (0x98e8c8) | Equal |
| 6..11 | "  *" | P1 |
| 12..18 | "  **" | P1 |
| 19..26 | "  ***" | P1 |
| 27..35 | "  ****" | P1 |
| >= 36 | "  *****" | P1 |
| flag 0 | "  n/a" (0xa858f8) | grey `[0xae3184]` |

After the grid the table bottom is y_grid_top - 10 = 351 (0x878f96..0x878fa6). When Part A does not run, it is 510 (0x876d6a).

### 3.3 Part B: attribute table (0x878fac..0x87a422)

It shows "Need two players to compare." (0xa85718) in a text box `0x5ed070(110, 115, 780, y_bottom, 2, 0, rows, 3, grey, text, 0)` (0x87a424..0x87a479) when any of these holds (0x8790b5..0x8790f4):

- p1 == p2, or p1 or p2 is 0.
- p1 job byte +0x3d == 2.
- p1 is one of the 16 human managers (`[0xae2364]-16 <= id < [0xae2364]`).

It also ends without a table when p1 has no player data (0x8799c8..0x8799d1).

Season statistics (0x8791a6..0x87998f). For each player, for k = 1..6, `0x7c25b0(player_data, k, stat, 0xde1f64, 0x7d0, 1, 0, 1)` (thiscall on `[0xb74350]`, player_stats.cpp). The stat ids come from a list: 0xa84a90 when the player's goalkeeper rating is 20, else 0xa84a78. The build keeps 6 of them:

| Stat id | Row | p1 buffer | p2 buffer |
|---|---|---|---|
| 0x11 | Avg. Rating | esp+0xf48 | esp+0xa98 |
| 1 | Appearances | esp+0xe80 | esp+0xc28 |
| 2 | Goals | esp+0x520 | esp+0x6b0 |
| 3 | Conceded | esp+0x778 | esp+0x5e8 |
| 5 | Assists | esp+0x9d0 | esp+0x908 |
| 0xc | Man of Match | esp+0xcf0 | esp+0xb60 |

`0x7c25b0` first writes "-" (0x98ba0c) or "----" for 0x11, then the value when a record for k exists (0x7c25df..0x7c26c7). The build copies the text on every k, so the k = 6 result wins. An empty "-" becomes "0" (0x99cc54) (0x879342..). Inference: k = 6 is a total over all competitions. Unknown: the meaning of k.

Table:

- `0x8b1750(p1, 0)` thiscall on `[0xde1f44]` (training_manager.cpp) at 0x879a6a, as the profile does at 0x87a720. It writes training data of the player and calls the random number function 0x90cfc0 (0x8b1ece..0x8b213a). A replacement must make the same call.
- Areas: (110, 115)-(780, y_bottom), 1x1, flags 2 (0x879a1b). Then the same rectangle with 6 columns, weights 3,2,3,2,3,2, rows = (y_bottom - 115) / `0x5e5cd0(2)`, flags 1 (0x879a4f). Classic: (351-115)/18 = 13 rows (inference from the row height).
- Cells (0x879a74..0x879b8c), for attr 1..31, stride 0x65:
  - Names from `0x5428a0(attr, text, 100, 0)` into three tables: 0xddf570, 0xdda680, 0xddcdf8.
  - p1 value at 0xddf5d4, p2 value at 0xdda6e4, d = p1 - p2 at 0xddce5c, masked flag (either is 0xf9) at 0xdd7f6c. Same getters as Part A.
  - qsort `0x945333` of each table, 31 cells, comparator 0x88bc50 (the profile's) (0x879b92..0x879bc6). The masked flags are not sorted. Inference: harmless, because the English names sort in id order.
- Extra cells written into 0xddf570 after the 31 (0x879bce..0x879cbb): Preferred Foot 0xff, Avg. Rating 0xfe, Morale 0xfd, On Loan 0xfb, Goals 0xf8 (or Conceded 0xf4 when `[0xde1d04]` != 0), Appearances 0xf7, Assists 0xf6, Man of Match 0xf5. Total 39 cells (0x879ccf), dealt round robin to three columns. The profile's Form and Condition rows are not here.
- Each cell (0x879d0e..0x87a3a6), column pair c = 0, 2, 4:
  - Name: `0x804100(area, c, row, 2, 0, 1, font 2, [0xae3184], "  %s")` (0x879e83..0x879ec1).
  - Attribute value: one number only, `abs(d)` as " %d" (0xa85738) with text_flags 0xc, font 2, in P1 colour when d > 0, Equal colour when d == 0, P2 colour when d < 0 (0x87a281..0x87a2ff). Neither player's own value is shown.
  - Masked: " -" (0xa8573c) in orange `[0xaebdd4]` (0x87a255..0x87a27f).
  - Preferred Foot: " %s ~ %s" (0xa85740) of the foot text of each player. Thresholds on player data +0x30 (right) and +0x3b (left): "LO", "L", "B", "RO", "R" (0xa857f4..0xa85758) (0x879ed8..0x879fef). Orange.
  - Avg. Rating: " %s - %s " (0xa8574c), text_flags 1, orange (0x87a095..0x87a0d5).
  - Goals, Conceded, Appearances, Assists, Man of Match: "%s - %s" (0x988080), orange (0x87a0e2..0x87a1a9).
  - Morale: `0x4cc3a0(p1, p2, buf, 0xc8)` thiscall on `[0xae1050]` gives a text and a byte. The colour depends on the byte (< 3, == 0xfd) (0x87a065..0x87a1eb). Details of 0x4cc3a0 unknown.
  - On Loan: p1 only. A widget `0x55f990(1, 0x22e, 0, 0x30c, 0, 0, row, 2, 0, 0, 0xc, 2, grey, 0, 0, id 0, data 0, area)` (0x879d63). "On loan from <club>" (0xa85834) with a club link 0x7d5, or "Given free transfer" (0xa85820) when `0x8c1870(p1)` (on `[0xde1f54]`), drawn by `0x4aeaf0(area, 0x238, 0x30c, ...)` (0x879d68..0x879e79). It writes the global `[0xae1080]` (0x879d8a).
  - Columns shorter than the row count are padded with empty grey labels (0x87a31e..0x87a378, 0x87a3ac..0x87a420).

### 3.4 What the screen shows that the profile does not

- No current or potential ability. No hidden attributes (Consistency, Important Matches, Injury Proneness, Natural Fitness and so on): only attributes 1..31 (0x8777eb, 0x879b89).
- The raw position ratings (player data +0x0f..+0x18) feed the stars and the ±35 rule. The profile shows only the positions text. Inference: the stars leak rating detail.
- Season statistics per player (Avg. Rating, Appearances, Goals or Conceded, Assists, Man of Match).
- Goalkeepers: the stat list swaps Goals for Conceded for a goalkeeper rated 20 (0xa84a90). The Goals row reads "Conceded" when both goalkeeper ratings are > 15. The attribute list is the same for all players.
- Better value: by colour only (P1 colour, Equal colour, P2 colour) of the difference number and of the stars. No frame or bold.

## 4. The input 0x889c30

It is referenced only by the two compare pushes (0x8706e5, 0x888fd5). It is a copy of a staff screen input. Before the id tests it handles a pending dialog result through `[0xb77c84]`, `[0xdd6be0]` and slot 0xd (0x889c93..0x889d3f). Inference: it does not run on this screen, because slot 0xd is 0.

Ids it tests, in order (0x889d88..0x88b1dc): 0x3c, 0x3d, 1, 0x1f, 0x1e, 2, 3, 0x1d, 0x2c, 0x2e, 0x2d, 0x2a, 0x2b, 0x23, 5, 0x24, 6, 7, 8, 0x19, 0x1a, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x17, 0x16, 0x18, 0x11, 0x13, 0x1b, 0x1c, 0x25, 0x26, 0x29. Examples: id 1 calls `0x8702a0` (open profile), id 2 the award vote (0xa8b320), id 3 the squad move (0xa8b2cc), ids 0xa and 0xb `0x8f4160`. Then `0x88b2f0` (profile tab ids 0x12, 0x27, 0x28, 0x14, 0x20, 0x3e, 0x3f, 0x21, 0x22), then `0x7615d0`, then `0x7643d0` (0x88b1dc..0x88b207).

The compare build makes no widget with any of these ids. Its only clickable widgets are:

- The comp_text links: player names (type 0x7d7, data = staff), nations and the loan club (type 0x7d5). Inference from the clickable names inventory: `0x7615d0` handles 0x7d0..0x7e7, so a click on a name opens that player's profile.
- The side bar and Back (-2), through `0x7615d0` and `0x7643d0`.

Inference: a replacement input can be `0x7615d0` then `0x7643d0` alone. One caution: with no widget (w == -1, the call after a pop), the input uses s16 `[0xdd6bde]` as the id (0x889d79..0x889d85). If that holds an old id from 1..0x3d, the original input could act on it. Unknown whether this happens.

Leave 0x88b8a0 (shared with the profile): frees slot 3 (destructor 0x68d040 + free) and slot 6 (0x9452ca) when they are not 0 (0x88b8a0..0x88b8fa). Both are 0 here, so it does nothing.

## 5. Hook points and call sites for a runtime replacement

- 0x876520 is never called directly. It is referenced only as the build pointer pushed at 0x8706ea (0x870650) and 0x888fda (profile input). So the main loop calls it through the node. A replacement can patch these two push immediates, or the function entry.
- 0x870650 has one caller, 0x7633f2.
- Players: `0x7f7920(1)` and `(2)`, thiscall, ECX = `[0xb74448]`.
- Name: `0x53dba0(staff, buf, len, 1)` cdecl. Positions text: `0x53ee50(player_data, buf, 0x7d0, 1)`. Job: `0x53ad00(job, 0, 1, 0)`. Nation: `0x6181d0(nation, buf, 0xc8, 0xc)`, link data `0x53b3d0(nation)`. Age: byte staff+0x18. The build does not show the club.
- Attribute values: `0x543440(viewer, staff, attr)` when `[0x9cc270]` != 0 (0xf9 = masked), else `0x543540(staff, player_data, attr)`. Names: `0x5428a0(attr, text, 100, 0)`. The profile 0x87a510 uses the same pair.
- Stats: `0x7c25b0` on `[0xb74350]`. Avg. Rating also calls `0x7c1660(player_data, buf, 0xc8)` on `[0xb74350]` (0x87a03a, 0x87a05a). Morale: `0x4cc3a0` on `[0xae1050]`. Loan: `0x4cf6d0`, `0x4cf5a0` on `[0xae1050]`. Free transfer: `0x8c1870` on `[0xde1f54]`.
- Training: `0x8b1750(p1, 0)` on `[0xde1f44]`. It changes game state and uses the random generator.

Globals the build writes (a replacement that skips them leaves other values there): the cell tables 0xddf570, 0xdda680, 0xddcdf8 (shared with the profile), 0xdd7f6c, the scores and flags 0xde1ce4..0xde1d04, 0xdda67c, 0xdda67e, the text buffer 0xde1f64, and `[0xae1080]`. Whether game logic reads any of them later: unknown. Inference: they are display scratch.

## 6. Heap allocations

- The build itself calls no allocator. The large frame is stack (`_chkstk` 0x945610). The tab entry array is on the stack.
- The two comp_text objects (stack, 0x4ae590 at 0x876549 and 0x878fcf) allocate: set text `0x4ae660` calls `operator new` 0x944cf1 and `free` 0x944c94; add link `0x4ae8a0` calls 0x944cf1 and creates widgets with 0x55f990. Their destructors `0x4ae630` (0x87a48d, 0x87a4ec) and the clear `0x4afa40` (0x876b23) free them. So the build makes and frees heap blocks during the build.
- Widgets and labels: the attribute groups file shows that label size changes the heap blocks allocated at return address 0x94557b, and that a different heap layout changes the saved game. Inference: any replacement that makes other widgets, other label sizes or other comp_text calls changes the heap, and can change the game.
- `0x8b1750` sorts with qsort `0x945333` (0x8b2218). Allocations inside it: unknown.
- The push allocates the 0x300-byte node (calloc in 0x7fe600). This is outside the build.

## Unknown

- The meaning of `[0xb74349]`, `[0x9cc270]` (inference: the attribute masking option), `0x66f4d0`, and the byte `[[0xae2d0c] + id*0x4f + 0xb]`.
- The RGB of `[0xae31aa]` and `[0xae3152]`.
- The meaning of k in `0x7c25b0`.
- The details of `0x4cc3a0` (Morale text and colour byte) and `0x7c1660`.
- The fourth column weight of the position grid.
- Whether the side bar build shows item 0x434 grey when players are missing.

## What a port can leave out

- The slot reads, the unused tab entry array, the dead `0x54a650` call, the `slot 4 == 1` and `slot 4 == 2` (`0x7f8610`) paths: all slots are 0 on this screen.
- The pre-id dialog code and the 39 foreign ids of the input 0x889c30.
