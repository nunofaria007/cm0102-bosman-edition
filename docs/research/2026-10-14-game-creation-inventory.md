# From "Select League(s)" to the first screen of a running game

Scope: one local player, English, the shipped data, default settings. The path starts with the click on Next of "Select League(s)" with England selected, and ends at the first screen of the running game.

Method:

- objdump disassembly of the original (Intel syntax) and REA (Ghidra 12.1.4) for pseudocode and function sizes.
- An emulator run of the original in Unicorn, built on `tests/diff/original.py` (scratchpad script, not in the repo). It runs `FUN_00524d50` (database load), `FUN_00827d50`, `FUN_00667150` and `FUN_00828110` (game creation) with the shipped data, a fixed random seed and stubs for Windows calls. It records the first entry of every function and the progress-box texts. Section 6 gives the results and the limits of this run.

Each fact gives the address that shows it. "Inference" marks a conclusion that the code does not show directly. "[emu]" marks a fact from the emulator run. "[data]" marks a fact from the files in `Data/`. Numbers are hex unless the text says otherwise. `S` is the screen stack object at 0xb74450. All structs are packed to 1 byte.

Corrections to earlier inventories:

- `2026-10-13-database-inventory.md` swaps the player and non-player tables. Table id 9 (68 bytes, `DAT_00ae23c0`) is the non-player table. Table id 10 (70 bytes, `DAT_00ae23c4`) is the player table. Staff +0x61 points to the player table and staff +0x69 to the non-player table. Evidence: the resolve pass (0x5315a0) maps +0x61 to `DAT_00ae23c4 + id * 0x46` with the error text "Staff %ld has invalid player data" (0x9b60b4), and +0x69 to `DAT_00ae23c0 + id * 0x44` with "Staff %ld has invalid non player data" (0x9b605c). The staff pass (0x52d44b) logs "No PLAYER Data" when +0x61 is 0 for job 11.
- The same inventory reads staff +0x5f values 1, 2 and 3 as "player only", "non-player only" and "both". With the swap above, the tally fits bit 0 = non-player data and bit 1 = player data (inference). In the 16 manager rows, the code uses +0x5f as a slot state (section 4.1).
- `2026-10-14-start-new-game-inventory.md` section 1d says Next with one league "pushes Manager Status". It pushes Manager Status and then `FUN_00820d40` pushes "Enter Name" on top. The player sees "Enter Name" first.

## Summary

1. Next on "Select League(s)" calls `FUN_00828110(0)` (game creation) inside the input function (0x81e712). Game creation shows four progress texts in the progress box: "Player Initialisation" (100 steps), "Player histories" (771,692 steps), "Initialising game data" (44 steps) and "Initialising Leagues" (one step per day) [emu]. No screen is pushed during game creation.
2. Game creation does not reload the database when it is in memory (0x828258). It looks for the CD first: it calls `GetLogicalDriveStringsA` and `GetDriveTypeA` and needs `cm0102.exe` on a CD-ROM drive, or it shows "Insert CD" in a loop with no exit (0x8281c0..0x828239). The port must leave this out.
3. The game date starts at 1 Jan 2001 (`FUN_0081b890`, 0x81b938). Game creation sets the start date `DAT_00dd6ec8` from the league table: England starts on the Saturday nearest 10 Aug 2001, which is 11 Aug 2001, less 4 weeks: 14 Jul 2001 [emu: day 194 of 2001]. `FUN_005cc850` then runs the full game day by day from 1 Jan 2001 to 14 Jul 2001, 194 days with three slices each, under "Initialising Leagues". So game creation runs the whole simulation engine.
4. After game creation the screens are: "Enter Name" (text entry), "Select Nationality" (scrolled list), "Select Team" (scrolled list), "Please Confirm" (Yes or No), "Manager Status" (Finish). Finish ends the setup loop. The first screen of the running game is the news screen "<manager name> News" (build 0x7881e0, news_screens.cpp).
5. The database load is fully readable from the assembly and is about 1,500 lines of C (section 2). It has 22 table entries, a 157 to 110 byte staff conversion, a 52-byte preferences table, 23,000 reserve staff rows, 16 manager rows, 127 reserve competition rows, an id-to-pointer pass and about 1,100 key ids matched by name.
6. All random numbers on this path come from one generator: `FUN_0090cfc0(n)`, a table of 51,000 numbers in the exe plus an offset from MSVC `rand()`. MSVC `rand()` is also called directly. Three globals hold the whole state (section 5).
7. Game creation reaches at least 1,300 functions of the original [emu, section 6]. Static reach from `FUN_00828110` is 3,333 functions and 4 MB. The port has none of the game logic. This is the largest part of the port so far, and the slice plan (section 7) has to cut it.

## 1. Screens the player sees

### 1a. Next on "Select League(s)", input 0x81dc30

Next (-3), 0x81e6a3. With `DAT_00dd7118` = 0 (no game yet) and `DAT_00ae2d04` = 1 (one selected nation):

```c
DAT_00dd7124 = 1;                                  /* new game, 0x81e708 */
if (FUN_00828110(0) == 0) {                        /* 0x81e712 */
    if (DAT_00dd7134) push(S, 0x826aa0, 0x826c20, DAT_00dd7118, 0, 0), set_slot(S, 0, 0, 0);
    else if (DAT_00dd7138) push(S, 0x826d20, 0x826e60, DAT_00dd7118, 0, 0), set_slot(S, 0, 0, 0);   /* "No Memory" */
    else error_box("Setup.cpp", 0xa0c), exit(-1);
    return -4;
}
if (DAT_00dd712c && FUN_008388e0(0, 0xdd70dc)) return that;   /* 0x81e81a, quick start. 0 on a plain start */
if (push(S, 0x81ff10, 0x820950, 0, 0, 0))          /* "Manager Status", 0x81e84a */
    for (i = 0; i < 5; i++) set_slot(S, i, 0, 0);
FUN_00820d40();                                    /* add a manager, pushes "Enter Name", 0x81e898 */
return -4;
```

`DAT_00dd7134` is set when `FUN_007a1820(0)` < 25 and `DAT_00dd7138` when free memory (`FUN_007a1710`) < 50 MB at the start of game creation, or < 25 MB between its steps (0x8281.., 0x82895d). The port can treat memory as large. `DAT_00dd712c` is 0 at the start screen (0x81ba14). Inference: it is the quick start flag.

During game creation the player sees the "Select League(s)" screen with the progress box over its bottom bar, the same box as "Loading database" (`FUN_00802f80` → `FUN_005e76f0`, start-new-game inventory 2c). The texts, in order [emu]:

| Text | Steps | Source |
|---|---|---|
| "Player Initialisation" (0x9b6414) | 0..99 of 100 | `FUN_00535a80` |
| "Player histories" | 1, 501, 1001, ... of 771,692 (2 × 385,846, the staff history count) | inside step 13, `FUN_007bfd50` (constructor of `DAT_00b74350`) |
| "Initialising game data" (0x9cc2ac) | 1..43 of 44 (0x2c) | `FUN_00828110` |
| "Initialising Leagues" (0x9cd244) | 0..193 of 194 | `FUN_005cc850`, one step per day. Total = days from the game date to the start date (0x5cc9d0) |

### 1b. "Enter Name", build 0x820f50, input 0x821520

`FUN_00820d40` (0x820d40..0x820f15) runs before the push:

```c
pl = 0xb74450 + cur_player * 0x300;               /* cur_player = (short)DAT_00b77c7a */
if (*(staff **)(pl + 0x7d6) == NULL) {             /* 0xb74c26 */
    staff *m = NULL;
    for (i = 0; i < 16; i++) {                      /* the 16 manager rows at the end of the staff table */
        staff *s = &staff_table[staff_count - 1 - i];
        if (s->b5f == 0) { m = s; break; }          /* 0x820de5 */
    }
    *(staff **)(pl + 0x7d6) = m;
    m->first_name->name[0] = 0; m->second_name->name[0] = 0; m->common_name->name[0] = 0;
    date_set(&m->born, 1, 0, 1966, -1);             /* 1 Jan 1966, FUN_0054a130, 0x820e2a */
    m->w18 = 0x23;                                  /* 35, inference: age */
    m->nation = nation_table + key_nation_for_language(FUN_0066f4d0()) * 0x122;  /* jump table 0x820f18. 0 or 1: DAT_009cf2e4 England */
    m->national_club = NULL; m->club = NULL;
    m->b5f = 5;                                     /* slot being made */
    FUN_005fb810(DAT_00b63c98, ...);                /* thiscall on the manager list, 1,264 bytes */
}
push(S, 0x820f50, 0x821520, 1, 0, 0); set_slot(S, 0, 0, 0); set_slot(S, 1, 0, 0);
```

What it shows (strings from 0x820f50..0x821520): heading "Championship Manager 2001/02", side bar (`FUN_0075db00(4, 0)`), sub-heading "Enter Name" (0xa80730), four rows "   First Name", "   Second Name", "   Password (Optional)", "   Re-Type", each a label (`cell_label` 0x804100) and an entry widget made by `FUN_0055f990` (0x8210f7, 0x8211e4, ...). Buttons "Cancel" (0x98a1a0) and "Next" (0x9880a0). This screen needs text entry. The first and second name are written into the manager's first-name and second-name rows (the 16 empty name rows at the end of the name tables, section 2e). The password goes to the manager list object (`FUN_005fde40` and `FUN_005fdf40` return the two password buffers, 0x82165c).

Input 0x821520:

| Id | Action | Evidence |
|---|---|---|
| 0x1a | Clear the re-typed password buffer. If the password is not empty, `set_slot(3, 0)`. Return -11 | 0x821577..0x8215c2 |
| 0x1b Cancel | Manager row +0x5f = 0. If this player has more than one manager (`DAT_00b77c5a[cur]` > 1): `FUN_007ff990(S, row, -1)`, return 0. Else clear `pl+0x7d6`, return -9 (pop) | 0x8215c3..0x821634 |
| -3 Next | First name and second name must not be empty, else a message box (`FUN_00591840`) "The first and second names entered are blank." / "...first name..." / "...second name..." and return -4. If a password is set and the two passwords differ: clear both, `set_slot(2, 0)`, message "The password and re-typed password do not match.", return -4. Else push "Select Nationality" (0x821900, 0x822040) with slots 0 = 0, 1 = 0, 2 = 1, 3 = 0, return -4 | 0x821635..0x8218d6 |
| other | `FUN_007615d0(id)` (side bar, 11,776 bytes) then `FUN_007643d0(id)` (3,136 bytes) | 0x8218d7 |

### 1c. "Select Nationality", build 0x821900, input 0x822040

Shows sub-heading "Select Nationality" (0xa8084c), a "Filter" row with " All Nations" (0x990da4) and " Major Nations" (0xa8083c), and a scrolled list of nations as " %s" rows (0x821eb9). The list area scrolls (slot 0 holds the first row, kept through `FUN_00803320` at 0x822061, the same as on "Select League(s)").

| Id | Action | Evidence |
|---|---|---|
| 0x1c | `set_slot(3, 1)` (Major Nations), return -11 | 0x8220b7 |
| 0x1d | `set_slot(3, 0)` (All Nations), return -11 | 0x82210b |
| 0x1e | Manager +0x1a = the nation in the widget data, return -11 | 0x82214c..0x822192 |
| -3 Next | Push "Select Team" (0x822260, 0x8228f0) with slot 0 = `DAT_00ae2d08` (the first selected nation, England), slots 1, 2 = 0, return -4 | 0x8221d6..0x822237 |

The manager's nation starts as England (1b), so Next works without a click.

### 1d. "Select Team", build 0x822260, input 0x8228f0

Shows sub-heading "Select Team" (0xa80860), a "Nation" row (0x9ca010) and a scrolled list of clubs as " %s" rows (0x822745). Slot 0 holds the nation whose clubs are listed.

| Id | Action | Evidence |
|---|---|---|
| 0x1f | If the widget data differs from slot 0: slot 0 = widget data (a nation), slot 1 = 0, return -11 | 0x822957..0x8229fa |
| -3 | Return 0 | 0x822a37 |
| other | Side bar functions | 0x822a41 |

A click on a club runs a widget callback at 0x482fc6 (in a club-list module, not read). It calls `FUN_00822a60`, which pushes "Please Confirm" (0x822aa0, 0x822c50) with slot 0 = the club (0x822a60..0x822a91). Whether the nation row is a popup area was not read: unknown.

### 1e. "Please Confirm", build 0x822aa0, input 0x822c50

Shows "Please Confirm" (0x9909b8), the text "Take control of {}<%s - club name>{} ?" (0xa8086c), "No" (0x98e8cc) and "Yes" (0x98e8d0). Inference from the size (432 bytes) and the slot-free build: a small box over "Select Team".

| Id | Action | Evidence |
|---|---|---|
| 0x20 No | Return -2 | 0x822cce |
| 0x21 Yes | If `FUN_00600a50(club, 1, 1, 0, 0)` (a human already runs it): message "Already Controlled", return -4. If a match runs (`DAT_00b67a50` and `FUN_006b59a0`): message "Playing In Match", return -4. Else manager +0x5f = 1, `FUN_00826f20(manager, club)` (496 bytes, take control), and when attribute masking is on (`DAT_009cc270`, 1 by default) `FUN_005afd70(DAT_00ae2a60, manager)`. `DAT_00dd6be4` = 0. Return -10 (pop the chain back to "Manager Status") | 0x822d18..0x822e93 |

### 1f. "Manager Status", build 0x81ff10, input 0x820950

Shows heading, side bar, sub-heading "Manager Status" (0xa5a47c). When no row of the 16 manager rows has +0x5f = 1, it shows the text "There are currently no human managers. Click on 'Add Manager' ..." (0xa8063c) through `FUN_005ed070`. Else one row per manager with the name (`FUN_0053dba0`, " %s"), club and nation (`FUN_00542100`), and a state "On Holiday", "Not Playing", "Playing" or "Waiting" (0x820363..0x820509). Bottom: "Back" (0x9880a8) and "Finish" (0xa80558), or "Fortsätt" (0xa80560) when `FUN_0066f4d0()` == 10 (0x82065b). The network lines ("Server IP Address", 0x81ffd4) appear only when `DAT_00b77c90` != 0.

Input 0x820950. It first compares five slots with the live state and returns -11 (rebuild) when one differs: slot 0 = count of manager rows with +0x5f = 1, slot 1 = `S` record count, slot 2 = manager list +8, slot 3 = count of manager rows with +0x5f = 1 and `FUN_007ffe40` true, slot 4 = manager list +0xc (0x8209c9..0x820b52). Then:

| Id | Action | Evidence |
|---|---|---|
| 0x18 | Click on a manager row: password box (0x822ec0, 0x8230b0) or "Resume control" box (0x823420, 0x823660). Network and saved-game case | 0x820b9d..0x820c6a |
| -2 Back | Only when `DAT_00dd7118` == 0: return 0 | 0x820caf |
| 0x19 Finish | `S+0x262a80` (0xdd6ed0) = 1, return -5 | 0x820cf5..0x820d12 |
| other | Side bar functions | 0x820d13 |

Return -5 ends the screen loop of `FUN_0081b890` (screen-stack inventory: -5 sets done for the only manager). `FUN_0081b890` then sets `DAT_00dd7118` = 1, registers "Manager Status" with `FUN_007feb20(S, 0x81ff10, 0x820950, 0, 0, 0, 0xdd7148, 5)` and returns 1 (0x81ba74..0x81baa8).

Inference: "Add Manager" is a side bar item (`FUN_007615d0`). It is not on the one-player path.

### 1g. From Finish to the news screen

`FUN_005cccd0` (4,368 bytes, the main loop, called at 0x5cc6ba) continues at 0x5cd118 after `FUN_0081b890` returns 1:

1. Copies the game date `DAT_00ae2c90` to a local. If `DAT_00dd7124` (new game): for each manager row with +0x5f = 1, a club and a division (+0x57), it walks the division's fixture object (`DAT_00adadfc[division id]`) and calls `FUN_00683ee0` and `FUN_0068d0d0` (0x5cd138..0x5cd241). Inference: first news items for the new manager.
2. While the manager list count (`[DAT_00b63c98]+8`) > 0: for each player slot with `DAT_00b77c5a[i]` > 0, push "Manager Status" again (`FUN_0081fea0`) and run the screen loop (0x5cd26b..0x5cd2b2). Inference: this is for managers added later. For one player after Finish the count is unknown.
3. The day loop (0x5cd2b4 onwards). At 0x5cd38a it compares the game date and slice with the last processed ones (`DAT_00ae2c78`, `DAT_00ae2cfc`). Equal: go straight to the human turn at 0x5cdbf4. Else process one slice (`FUN_006b3c10` match day, competitions, staff ages, ...).
4. Human turn `FUN_005cde40` (544 bytes, 0x5cdbf9): for each manager row with +0x5f != 0, `FUN_00788010(manager, 0)` (4,720 bytes). It pushes the news screen: build 0x7881e0 (4,304 bytes), input 0x7892b0 (1,264 bytes), exit 0x7897a0 (0x788010). The heading is "<%s - Manager Name(e.g.Walter Smith)> News" (0xa69560). Categories: "All", "Messages", "Competitions", "Injuries and Bans", "Contracts and Media", "Transfers", "Jobs", "Records", a "Filter :" row and "Next Unread". Then the screen loop runs (0x5ce051).

So the first screen of the running game is the news screen. Whether one slice of 14 Jul 2001 runs before it depends on `DAT_00ae2c78` after creation: unknown (section 8).

## 2. The database load

### 2a. Call and arguments

`FUN_0081ce00` → `FUN_00524d50(dir, 1)` → `FUN_00528540(dir, 1, 0)` (start-new-game inventory 2a). Game creation calls `FUN_00524d50` again only when `DAT_00ae23a8` (nation table) is 0 (0x828258). On this path the database is already loaded, so the load runs once.

Arguments: `dir` = the data folder, `shared` = 1, `from_stream` = 0. The return is 1, or 0 on a short read.

With `shared` = 1 the loader uses the "shared" sizing branch (0x52990e). Both branches give the same tables. The shared branch adds the reserve rows. Only the shared branch is on the new-game path. The non-shared branch (0x5289c7..0x52b378) is for saved games and can wait.

### 2b. Tables after the load

Counts are from the emulator run after the load [emu], and match the code. "Rows" is the count global after the load. "Alloc" is the number of rows that memory has room for.

| Index entry | File | Id | Rec size disk / memory | Disk count | Rows (count global) | Alloc | Table global | Reader |
|---|---|---|---|---|---|---|---|---|
| 0 | club.dat | 0 | 581 / 581 | 13,214 | 13,640 with nat_club (`DAT_00ae235c`) | 13,640 | `DAT_00ae23b4` | 0x54db50 |
| 1 | nat_club.dat | 1 | 581 | 426 | 426 (`DAT_00ae2360`) | in club block | `DAT_00ae23b8` = club base + 13,214 × 581 | 0x54db50 |
| 2 | colour.dat | 2 | 58 | 34 | 34 (`DAT_00ae2394`) | 34 | `DAT_00ae23ec` | 0x550710 |
| 3 | continent.dat | 3 | 198 | 6 | 6 (`DAT_00ae234c`) | 6 | `DAT_00ae23a4` | 0x54d5e0 |
| 4 | nation.dat | 4 | 290 | 213 | 213 (`DAT_00ae2350`) | 213 | `DAT_00ae23a8` | 0x54d740 |
| 5 | officials.dat | 7 | 43 | 4,567 | 4,567 (`DAT_00ae2390`) | 4,567 | `DAT_00ae23e8` | 0x5505c0 |
| 6 | stadium.dat | 5 | 78 | 8,917 | 8,917 (`DAT_00ae2358`) | 8,917 | `DAT_00ae23b0` | 0x54da00 |
| 7 | staff.dat A | 6 | 157 / 110 | 169,342 | 192,358 (`DAT_00ae2364`) | 193,683 | `DAT_00ae23bc`. Temporary 157-byte table `DAT_00ae2404` (count `DAT_00ae2400`), freed after conversion | 0x54e780 |
| 8 | staff.dat B | 8 | - | 0 | - | - | none, no case | - |
| 9 | staff.dat C, non-player | 9 | 68 | 30,865 | 53,881 (`DAT_00ae2368`) | 53,881 | `DAT_00ae23c0` | 0x54e880 |
| 10 | staff.dat D, player | 10 | 70 | 139,769 | 139,769 (`DAT_00ae236c`) | 141,094 | `DAT_00ae23c4` | 0x54f030 |
| 11 | staff_comp.dat | 11 | 101 | 543 | 543 (`DAT_00ae2388`) | 543 | `DAT_00ae23e0` | 0x550380 |
| 12 | city.dat | 21 | 56 | 6,682 | 6,682 (`DAT_00ae2354`) | 6,682 | `DAT_00ae23ac` | 0x54d8a0 |
| 13 | club_comp.dat | 12 | 107 | 433 | 579 with nation comps and 127 reserve rows (`DAT_00ae2378`) | 579 | `DAT_00ae23d0` | 0x54feb0 |
| 14 | nation_comp.dat | 16 | 107 | 19 | 19 (`DAT_00ae2380`) | in club comp block | `DAT_00ae23d8` = club comp base + 433 × 107 | 0x54feb0 |
| 15 | first_names.dat | 13 | 60 | 44,281 | 44,297 (`DAT_00ae2398`) | 44,297 | `DAT_00ae23f0` | 0x550860 |
| 16 | second_names.dat | 14 | 60 | 104,395 | 104,411 (`DAT_00ae239c`) | 104,411 | `DAT_00ae23f4` | 0x550860 |
| 17 | common_names.dat | 15 | 60 | 13,256 | 13,272 (`DAT_00ae23a0`) | 13,272 | `DAT_00ae23f8` | 0x550860 |
| 18 | staff_history.dat | 17 | 17 | 385,846 | 385,846 (`DAT_00ae2374`) | 385,846 | `DAT_00ae23cc` | 0x54fd60 |
| 19 | staff_comp_history.dat | 18 | 58 | 2,609 | 2,609 (`DAT_00ae238c`) | 2,609 | `DAT_00ae23e4` | 0x5504d0 |
| 20 | club_comp_history.dat | 19 | 26 | 10,766 | 11,039 with nation comp history (`DAT_00ae237c`) | 11,039 | `DAT_00ae23d4` | 0x550230 |
| 21 | nation_comp_history.dat | 20 | 26 | 273 | 273 (`DAT_00ae2384`) | in the history block | `DAT_00ae23dc` = base + 10,766 × 26 | 0x550230 |
| - | (built) preferences | 22 | - / 52 | - | 169,358 (`DAT_00ae2370`) | 169,358 | `DAT_00ae23c8` | built at 0x52cb95 |

After the load in shared mode, `DAT_00ae2344` = 127 and `DAT_00ae2348` = 16 (0x52c687). They are the counts of reserve competition rows and manager rows (inference from their use). In the file header both are 0.

Shared tables: the club and nat-club rows are one array. The same holds for club comps with nation comps and the 127 reserve rows, and for the two comp history files. A nation comp or a nat club is a row of the shared array, so an id field that points into "club comps" can point to a nation comp (inference: the ids in the data already count from the start of the shared array).

### 2c. Pseudocode of `FUN_00528540` for a new game

The assembly is the evidence. Ghidra garbles the function. The port should keep table rows as structs and ids as ints, and resolve ids to row indexes (section 2d), because a 64-bit port cannot keep pointers in 4-byte fields.

```c
int load_database(const char *dir, int shared /* 1 */, int from_stream /* 0 */)
{
    uint8_t extra_byte = 0;                                   /* esp+0x23 */
    free_tables();                                            /* FUN_00528080, 0x5285a8 */
    progress("Loading database", 0, 46);                      /* 0x5285d4 */
    copy *.lng from dir to CM3_TEMP;                          /* 0x52861f..0x5286d2, port: leave out */
    if (!lng_exists(settings->language)) set_language(0);    /* FUN_00535930, 0x5286e4 */

    FILE *f = open(dir, "index.dat");                         /* 0x528722 */
    read(&hdr1, 4); read(&hdr2, 4);                           /* DAT_00ae2344, DAT_00ae2348: 0, 0 */
    read(index, 0x43, 22);                                    /* 0xae1a80, 67-byte entries */
    read(&extra_byte, 1);                                     /* end of file: stays 0 */
    if (extra_byte) { ... }                                   /* 0x528848, not for shipped data */
    DAT_009b5984 = 0;                                         /* language of the names, 0x528a30 */
    fclose(f);

    int tail = 16 - hdr2;                                     /* ebp = 16, 0x52898d */
    progress(.., 1, 46);
    DAT_00ae2340 = shared;

    /* Sizing, shared branch 0x52990e. One progress step per entry, jump table 0x52d9d8 */
    for (e in index[0..21]) {
        progress(.., step++, 46);
        n = e.count;
        switch (e.id) {
        case 0:  clubs += n; break;                           /* DAT_00ae235c */
        case 1:  natclubs = n; clubs += n; break;
        case 2:  colours += n; break;
        case 3:  continents += n; break;
        case 4:  nations = n; break;
        case 5:  stadiums = n; break;
        case 6:  staff = n + tail + (extra_byte ? 0x52d : 0x5f05);         /* 0x529b31 */
                 if (e.format == 1) { staff157 = n; alloc staff157 * 157; } /* DAT_00ae2404 */
                 break;
        case 7:  officials += n; break;
        case 9:  nonplayers = n + tail + (extra_byte ? 0 : 23000); break;  /* 0x529bc7, 0x59d8 */
        case 10: players = n + 0x52d; break;                               /* 0x529bf8 */
        case 11: staffcomps += n; break;
        case 12: clubcomps += n + 127; break;                              /* 0x529a88 */
        case 13: firsts += n + tail; break;
        case 14: seconds += n + tail; break;
        case 15: commons += n + tail; break;
        case 16: clubcomps += n; nationcomps += n; break;
        case 17: staffhist = n; break;
        case 18: staffcomphist += n; break;
        case 19: comphist += n; break;
        case 20: comphist += n; nationcomphist += n; break;
        case 21: cities += n; break;
        case 22: prefs = n + tail; alloc; break;                           /* no entry 22 in the index */
        }
    }
    allocate every table, rows as counted above;              /* 0x529cc6..0x52a046 */
    nationcomp_base = clubcomp_base + (clubcomps - nationcomps - 127) * 107;
    natclub_base    = club_base + (clubs - natclubs) * 581;
    nationcomphist_base = comphist_base + (comphist - nationcomphist) * 26;
    staff -= 0x52d; players -= 0x52d;                         /* 0x52a04b: 1,325 spare rows stay allocated */

    /* 127 reserve competition rows: the last 127 rows of the club comp array, 0x52a068 */
    for (i = 1; i <= 127; i++) {
        r = &clubcomp[clubcomps - i];
        r->id = clubcomps - i; r->name[0] = 0; r->short_name[0] = 0; r->b53 = 0; r->b57 = 0; r->b58 = 0;
        r->i59 = r->i5d = r->i61 = r->i65 = -1; r->w69 = 0;
    }

    /* 23,000 reserve staff rows, each with its own non-player row, 0x52a1c2 */
    if (!extra_byte) for (i = 1; i <= 23000; i++) {
        s = &staff[staff_count - i - tail];   np = &nonplayer[nonplayer_count - i - tail];
        s->id = index of s; s->first = s->second = -1; s->common = 0;
        s->born = DEFAULT_DATE; s->w18 = 1971; s->nation = s->nation2 = -1; s->b22 = s->b23 = 0;
        s->national_club = -1; s->national_job = 0; s->d29 = s->d31 = DEFAULT_DATE;
        s->club = -1; s->club_job = 0; s->d3e = s->d46 = DEFAULT_DATE; s->i4e = s->i52 = 0;
        memset(&s->b56, 0, 9); s->b5f = 6; s->b60 = 0; s->player = -1; s->prefs = -1;
        s->nonplayer = index of np; s->b6d = 0;
        np->id = index of np; np->w04..w0c = 0; np->b0e..b22 = 0; np->i23..i3f = -1; np->b43 = 0;
    }
    /* 16 manager rows at the very end, 0x52a9d3 */
    for (i = 1; i <= tail; i++) {
        s = &staff[staff_count - i];
        s->id = staff_count - i;
        s->first = firsts - i; s->second = seconds - i; s->common = commons - i;
        /* same fields as a reserve row, but: */
        s->b5f = 0; s->prefs = prefs_count - i;     /* prefs_count is still 0 here: -i, so NULL after resolve */
        s->nonplayer = nonplayer_count - i;
        first[firsts - i]   = { name = "", id = firsts - i, nation = -1, count = 1 };   /* 0x52add4 */
        second[seconds - i] = { name = "", id = seconds - i, nation = -1, count = 1 };
        common[commons - i] = { name = "", id = commons - i, nation = -1, count = 1 };
        nonplayer[nonplayer_count - i] = blank as above;
    }

    /* Read, index order, 0x52b37c. One progress step per entry, jump table 0x52da34 */
    for (k = 0; k < 22; k++) {
        progress(.., step++, 46);
        e = index[k];
        f = open(dir, e.name); fseek(f, e.offset, SEEK_SET);
        ok = reader[e.id](f, from_stream, table_for(e.id), &e);  /* fread(table, size, e.count, f) */
        if (!ok) return 0;
        fclose(f);
    }
    /* id 18 (staff_comp_history): when the file is missing the table is freed and its count set to 0, 0x52bcea */
    /* id 16: when hdr1 > 0, a second read into the 127 reserve rows. Not for shipped data, 0x52bbd4 */
    progress(.., step, 46);                                       /* 0x52c671 */
    hdr1 = 127; hdr2 = tail;                                      /* 0x52c687, shared only */

    /* Staff conversion 157 → 110 and the preferences table, 0x52caa1 */
    prefs_count = staff157 + hdr2;                                /* 169,358 */
    prefs = alloc(prefs_count * 52);
    for (i = 0; i < staff157; i++) {
        uint8_t *src = staff157_table + i * 157; staff *dst = &staff[i];
        memcpy(dst, src, 0x10);                                   /* id, first, second, common */
        dst->born = src[0x10..0x17]; dst->w18 = src[0x18]; dst->nation = src[0x1a]; dst->nation2 = src[0x1e];
        dst->b22 = src[0x22]; dst->b23 = src[0x23]; dst->club = src[0x39]; dst->club_job = src[0x3d];
        dst->d3e = src[0x3e]; dst->d46 = src[0x46]; dst->national_club = src[0x24]; dst->national_job = src[0x28];
        dst->d29 = src[0x29]; dst->d31 = src[0x31]; dst->i4e = src[0x4e]; dst->i52 = src[0x52];
        dst->b56..b5f = src[0x56..0x5f];
        dst->player = *(int *)(src + 0x91);
        dst->prefs = i;
        dst->nonplayer = *(int *)(src + 0x99);
        dst->b60 = src[0x60];
        int *p = (int *)(prefs + i * 52);
        p[0] = i; for (j = 0; j < 12; j++) p[1 + j] = *(int *)(src + 0x61 + 4 * j);
    }
    for (i = 1; i <= hdr2; i++) {                                 /* 16 blank preference rows, 0x52d037 */
        int *p = (int *)(prefs + (prefs_count - i) * 52);
        p[0] = prefs_count - i; p[1..12] = -1;
    }
    free(staff157_table);

    resolve_database_indexes(0);                                  /* FUN_005315a0, 0x52d1aa, section 2d */
    DeleteFileA("duff.out");                                      /* 0x52d205, port: leave out */
    staff_pass();                                                 /* 0x52d20a..0x52d81c, below */
    if (!extra_byte) national_club_pass();                        /* 0x52d82c..0x52d8f8, below */
    set_key_ids();                                                /* FUN_00601a40, 0x52d8fa, section 2e */
    return 1;
}
```

`DEFAULT_DATE` is `{day = 40, year = 1972, leap = 1, 0}` (`FUN_0054a110`), which is 10 Feb 1972.

The 1,325 spare rows (0x52d) after the staff and player counts are allocated but not counted and not set. Inference: they are for new staff during the game.

Staff pass (0x52d225). For each staff row `s` with `s->id < staff_count - 16` and `s->b5f != 6`:

```c
if (s->club && s->club->nation == NULL) {                     /* "extinct club", 0x52d23c */
    log("Staff %s (%s) belongs to extinct club (%s)");
    FUN_0053c190(s->club, s, 0, 1, 0);                        /* 1,024 bytes, remove from club */
    s->club = NULL;
}
if ((s->b5e & 2) && s->club && !FUN_00542740(s->club))       /* 0x52d29e */
    s->b5e = (s->b5e & ~2) | 1;
if (s->b23 && !s->b22) s->b23 = 0;                            /* 0x52d2c4 */
if (!s->club && !s->national_club && s->nonplayer) {          /* 0x52d2d1 */
    if (date_less(s->d3e, date(1, 0, 1998)))                  /* compare year +0x40, then day +0x3e */
        s->d3e = date(1, 8, 1997);                            /* 1 Sep 1997 */
}
if (s->club_job == 7) s->club_job = 6;                        /* 0x52d33f */
if (s->national_job == 7) s->national_job = 6;
if (s->national_job == 5 && !s->nonplayer) {                  /* 0x52d353 */
    if (s->national_club) s->national_club->manager = NULL;   /* club +0xcf */
    s->national_job = 0; s->national_club = NULL;
}
if (s->national_club && s->national_job == 0) s->national_club = NULL;
if (!s->nation) {                                             /* 0x52d37e */
    s->nation = s->club ? s->club->nation
              : s->national_club ? s->national_club->nation
              : &nation[random(nation_count)];                /* FUN_0090cfc0, 0x52d3aa */
    log("'%s (%ld)' has no nation set");
}
switch (job_kind(s->club_job)) {                              /* table 0x52daa0: 1..10 → A, 11 → P, 12, 13, 15 → D, 14 → none */
case A: if (!s->nonplayer) { log("No NON_PLAYER Data"); s->club_job = 0; } break;
case P: if (!s->player) { log("No PLAYER Data"); s->club_job = 0;
            if (s->club) for (j = 0; j < 50; j++) if (s->club->squad[j] == s) s->club->squad[j] = NULL; } break;
case D: if (!s->nonplayer) { log(...); s->club_job = 11; }
        if (!s->player) { log(...); s->club_job = 0; } break;
}
for (name in first, second, common)                           /* 0x52d52e..0x52d63e */
    if (s->name == NULL) s->name = &common[first_empty_common()];   /* see below */
switch (job_kind(s->national_job)) { same as above, on national_job; case P clears s->club->squad }  /* 0x52d641 */
if (!from_stream && s->nonplayer && s->national_club && s->national_job != 5) {   /* 0x52d780 */
    FUN_0053c190(s->national_club, s, 0, 1, 0);
    s->national_job = 0; s->national_club = NULL;
}
if (s->national_job in 1..13 or 15) {                         /* 0x52d7af */
    if (!s->nation) { s->national_club = NULL; s->national_job = 0; log("(NEW) No staff nation set (despite national job)"); }
    else if (!s->national_club) { s->national_club = NULL; s->national_job = 0; }
}
```

`first_empty_common()`: the index is cached in `DAT_009b59b4` (-1 in the file). When it is -1, the code scans the common-name table for the first row whose name is empty and caches its index. If no row is empty it takes `random(common_count)` without caching (0x52d533..0x52d57f). All three name fields use the common-name table (0x52d548, 0x52d59c, 0x52d5fb). Inference: a bug of the original. A port must copy it to give the same pointers. With the shipped data the 16 manager name rows are empty, so a row is always found.

The log texts go to 0xde1f64 and are not shown. `FUN_0053dba0` (1,440 bytes) only builds the name for the log. The port can leave out the log and `FUN_0053dba0`, but not the changes.

National club pass (0x52d82c):

```c
for (i = clubs - nations; i < clubs; i++)                     /* the last 213 rows of the club array */
    club[i].w80 = FUN_0053b3d0(club[i].nation)->w80 / 2;      /* signed, rounds toward 0 */
for (i = 0; i < clubs; i++)                                   /* 0x52d896 */
    if (club[i].division && strcmp(club[i].division->name, "Reserve") == 0)
        club[i].division = NULL;
```

`FUN_0053b3d0(nation)` returns `&club[nation->id + clubs - 2 * nations]` (0x53b3d0), the first 213 rows of the nat-club block. Inference: the nat-club file has 213 senior national teams then 213 more teams (inference: under-21 teams), and the second group gets half the reputation of the first.

### 2d. `FUN_005315a0`, `resolve_database_indexes()`

Ghidra reads this function correctly (spot check of the assembly at 0x5315a0..0x531760). It replaces each id with a pointer to the row with that index, in place. A negative id gives NULL. A too high id gives NULL, sometimes with a log line. The bound test is `id > count` for some fields and `id >= count` for others, as the table shows. A port that keeps ids should store -1 where the original stores NULL, and keep the same tests.

| Table | Field | Target | NULL when | Evidence (Ghidra line in 0x5315a0) |
|---|---|---|---|---|
| club | +0x53 | nation | id < 0 or id > nations (log "high club nation index") | 0x5315d6 |
| club | +0x57 | club comp (division) | id < 0 or id > clubcomps | 0x531625 |
| club | +0x5b | club comp (last division) | id < 0. When id > clubcomps: logs and sets **+0x60** to 0, +0x5b keeps the raw id (bug of the original, 0x531686) | 0x531657 |
| club | +0x60 | club comp | id < 0 or id > clubcomps | 0x5316a8 |
| club | +0x69, +0x6e | stadium | id < 0 or id > stadiums | 0x5316e0 |
| club | +0x83, +0x87, +0x8b, +0x8f, +0x93, +0x97 | colour | id < 0 or id > colours | |
| club | +0x9b, +0x9f, +0xa3, +0xa7, +0xab, +0xaf | staff | id < 0 or id > staff_count | |
| club | +0xb3, +0xb7, +0xbb | club | id < 0 or id > clubs | |
| club | +0xd7, 50 entries | staff (squad) | id < 0, or (param 0) id >= staff_count. Log when id > staff_count + 0x52d | |
| club | +0x1b3, 7 entries | staff (scouts) | id < 0 or id >= staff_count (log) | |
| club | +0x19f, 5 entries | staff (coaches) | same | |
| club | +0x1cf, 3 entries | staff (physios) | same | |
| club | +0xc3, 3 entries | staff (directors) | same | |
| club | +0xcf | staff (manager) | same | |
| club | +0xbf | staff (chairman) | same | |
| club | +0xd3 | staff (assistant manager) | same | |
| club | +0x1e0, 20 entries | staff ("team member selected") | same | |
| nation | +0x71 | continent | id < 0 | |
| nation | +0x7a | city | id < 0 | |
| nation | +0x80 | stadium | id < 0 | |
| nation | +0x90, +0x94, +0x98, +0x9c, +0xa0, +0xa4 | colour | id < 0 | |
| nation | +0x110, +0x114, +0x118 | nation | id < 0 | |
| officials | +0x04 | first name | id < 0 | |
| officials | +0x08 | second name | id < 0 | |
| officials | +0x16 | nation | id < 0 | |
| officials | +0x1a | city | id < 0 | |
| stadium | +0x38 | city | id < 0 or id > cities | |
| stadium | +0x48 | stadium | id < 0. When id > stadiums: sets **+0x38** to 0 and keeps +0x48 raw (bug) | |
| preferences | +0x04..+0x18 (6) | club | id < 0 | |
| preferences | +0x1c..+0x30 (6) | staff | id < 0 | |
| non-player | +0x23..+0x3f (8) | staff | id < 0 or id >= staff_count | |
| staff | +0x04, +0x08, +0x0c | first, second, common name | id < 0 | |
| staff | +0x1a, +0x1e | nation | id < 0 | |
| staff | +0x39 | club | id < 0 | |
| staff | +0x24 | club (national team) | id < 0 | |
| staff | +0x61 | player (70-byte) | id < 0. id >= players: error box (line 0x18f6), NULL | |
| staff | +0x65 | preferences | id < 0. id >= prefs: error box (0x1907), NULL | |
| staff | +0x69 | non-player (68-byte) | id < 0. id >= nonplayers: error box (0x1918), NULL | |
| staff comp | +0x53 | continent | id < 0 or id >= continents | |
| staff comp | +0x57 | nation | id < 0 or id >= nations | |
| staff comp | +0x5b, +0x5f | colour | id < 0 or id >= colours | |
| staff history | +0x04 | staff | id < 0 | |
| staff history | +0x0a | club | id < 0 | |
| city | +0x1f | nation | id < 0 | |
| comp history | +0x04 | club comp | id < 0 | |
| comp history | +0x0a, +0x0e | club | id < 0 | |
| comp history | +0x12, +0x16 | club | id < 0 or id > clubs | |
| club comp | +0x5d | nation | id < 0 | |
| club comp | +0x59 | continent | id < 0 | |
| club comp | +0x61, +0x65 | colour | id < 0 | |

The staff comp history table (+0x04 and others) is not touched.

### 2e. Key ids, `FUN_00601a40` (index.cpp)

`FUN_00601a40` (41,024 bytes, source `index.cpp`, 0x9d93cc) works in this order:

1. Sets about 1,100 globals from 0x9cf178 to 0x9d0524 to -1 (0x601a4a..).
2. Colours: for each colour row `i`, compares the name (+0x04) with a fixed list through `FUN_00951c50` (inference: `_stricmp`). The first equal string sets its global to `i` (an else-if chain, 0x60386c..0x603c49).
3. When `DAT_009b5984` != 0 it reads `CM3_TEMP\eng.lng` (0x603c52). Not on this path (`DAT_009b5984` = 0).
4. Nations: `FUN_0060d810(nation[i].name, i)` for each row (6,080 bytes, 0x60421a).
5. Club comps: `FUN_0060efd0(clubcomp[i].name, i)` for `i < clubcomps - 127` (9,744 bytes, 0x604251). The nation comps are in this range.
6. Continents: "Africa", "Asia", "Europe", "North America", "Oceania", "South America" (inference for the last) set 0x9cfa08..0x9cfa1c (0x604289).
7. Staff comps: `FUN_006115e0(staffcomp[i].name, i)` (12,496 bytes, 0x604360).
8. Clubs: `FUN_006146b0(club[i].name, i)` (10,352 bytes, 0x604391).
9. For each key id that is still -1: log "Unable to find the %s index" and an error box (index.cpp line 0x16b1 and on). The rest of the 41 KB is these checks (0x6043ac..).

Each matcher has the shape:

```c
void match_nation(const char *name, int row) {
    if (!_stricmp(name, "Argentina")) DAT_... = row;
    else if (!_stricmp(name, "Australia")) DAT_... = row;
    ...
}
```

A port can hold each matcher as a table of `{name, global}` pairs. The names and globals can be read from the exe with a script, because each pair is a `push <string>; push edi; call 0x951c50; ...; mov [<global>], esi` block. Inference: the error check of step 9 never fires with the shipped data.

### 2f. Readers

Every reader has the form `int reader(FILE *f, int from_stream, void *table, index_entry *e)`, by the push order at 0x52b44a (Ghidra names the arguments wrongly). It asserts the format (+0x3f of the entry) and freads `e->count` records of the record size. Only the club reader changes data: a club with +0x80 = 0 gets +0x80 = 1 (`FUN_0054db50`). The staff A reader `FUN_0054e780` reads 157-byte records and asserts format 1. The city reader `FUN_0054d8a0` does not check the format (0x54d8a0).

## 3. Game creation, `FUN_00828110`

10,416 bytes, source Setup.cpp. Ghidra reads its structure. The frame is 0xb58 bytes, so the stack locals in the pseudocode are not reliable. The steps below are checked against the assembly where an address is given.

### 3a. Order

```c
int create_game(void)                                          /* returns 1 or 0 */
{
    if (FUN_007a1820(0) < 25) { DAT_00dd7134 = 1; return 0; }
    if (DAT_009cd1fc && free_mb() < 50) { DAT_00dd7138 = 1; return 0; }
    cd_check_loop();                                           /* 0x828196..0x828239, see 3b */
    if (nation table == NULL) { load_database(CM3_DATA, 1); FUN_005d8480(settings); }   /* not on this path */
    FUN_00541e60();                                            /* 0x8282f1, squads: see 3c */
    command line "-verifypics", "-verifyhist", "-verifyplcfg", "-verifyeurocfg": test tools, leave out
    FUN_00539750();                                            /* 912 bytes, pictures config (reads .pct files) [emu] */
    FUN_00539ae0();                                            /* 176 bytes, comp history: keep rows of selected nations, qsort */
    DAT_00dd7114 = settings->i04 & 7;
    FUN_00524d70(DAT_00dd7114);                                /* 6,736 bytes, database trim, 3c */
    FUN_005267c0(DAT_00dd7114);                                /* 2,688 bytes */
    if (nation[DAT_009cf31c].flags) { FUN_00839920(DAT_009cf86c, 14); FUN_00839920(DAT_009cf870, 14); }
    FUN_00544970();                                            /* 768 bytes */
    DAT_00ae2d1c = malloc(clubs * 4);  FUN_0053db40(1);        /* club ids, sorted by qsort with 0x540020 */
    FUN_00830520();                                            /* start date, 3d */
    DAT_00dd6ec8 = DAT_00dd6ec8 - 4 weeks;                     /* FUN_0054cab0(4), 0x828480 */
    DAT_00ae2d0c = malloc(staff_count * 0x4f); init each row; /* 3c */
    DAT_00ae2d14 = malloc(clubs * 6); each row = {-1, 0, 0};
    DAT_00dd7144 = time(NULL); DAT_00dd7140 = 0;
    for each club, for each of 50 squad entries: if (p && !p->club) p->club = this club;
    for each club: club->i1db = -1;
    FUN_00535a80(&DAT_00ae2c90);                               /* 12,608 bytes, "Player Initialisation", 0x828894 */
    if (DAT_00b74348 || !DAT_009b5981) FUN_00532e00();         /* "Use Real Players: No" */
    for (pass = 1; pass <= 2; pass++) {                        /* pass 1 only sets the total 44 */
        if (pass == 2) the 44 steps of 3e;
    }
    FUN_005cc850();                                            /* run days to the start date, 3f, 0x829fd9 */
    DAT_00dd711c = 1; DAT_00dd7130 = 0;
    if (!file_exists("auto_setup.cfg")) "CD Not Required" box when a CD is found;  /* leave out */
    return 1;
}
```

### 3b. CD check

`GetLogicalDriveStringsA(200, buf)`. For each drive with `GetDriveTypeA` == 5 (CD-ROM), look for `<drive>cm0102.exe` (`FUN_004c0aa0`, the string is built by `sprintf("%s%c%c...", drive, 'c','m','0','1','0','2','.','e','x','e')` at 0x82819b). When found and the database is in memory, go on. Else show "Insert CD" / "Please insert the CM01/02 CD and..." (0x987df4, 0x987dc4) through `FUN_005e8290(.., .., 1)` and try again with no limit. The port must leave this out. A differential test must stub these two imports [emu].

### 3c. Tables that game creation changes

| Change | Function | Evidence |
|---|---|---|
| Club byte +0x244 = 1 when `FUN_00540a50(club)` finds a league with a nation, else 0. For such clubs with too many players: sort the squad (qsort comparator 0x5402f0) and remove players from the end with `FUN_0053c190` until 50 remain, keeping at least 3 with player +0x0f > 14 | `FUN_00541e60` (304 bytes) | 0x541e60 |
| Comp history rows for nations without flags removed, table shrunk (`realloc`), sorted with comparator 0x540780 | `FUN_00539ae0` | 0x539ae0 |
| Staff, player, non-player and preferences tables trimmed and shrunk in place by `realloc`; random choices; logs "Staff table has %d(+%d Virtual) entries" and "Player table has %d entries". It asserts that `realloc` returns the same address (Database.cpp lines 0x18e..0x191) | `FUN_00524d70` (6,736 bytes) | 0x525ff0..0x5260c3, 0x5267.. |
| Not read | `FUN_005267c0` (2,688), `FUN_00544970` (768) | |
| Club +0x57 (division) fixed so that the two club comps `DAT_009cf86c` and `DAT_009cf870` have 14 clubs each, only when Greece (`DAT_009cf31c`, matched at 0x60d8..) has flags. Not on the English path | `FUN_00839920` | 0x839920 |
| Staff +0x39 (club) = the club whose squad holds it, when it was NULL | `FUN_00828110` | 0x8287e5.. (Ghidra lines 300..320) |
| Club +0x1db = -1 | same | |
| Player attributes and other staff fields, with random numbers | `FUN_00535a80` | strings "Player Initialisation", "Nation '%s' has no reputation set" (0x9b63f0) |
| Club +0x6d = 0 when the name starts with "Bright" or "Wimble", else 1 | step 0x23 | 0x82992b..0x82995c |
| Club +0x80 when <= 0: `(char)(random(4000) + 1)` (truncated to a signed byte, a quirk). Never true after the reader fix | step 0x23 | 0x829960 |
| Club +0x73 when <= 0: `rep * rep / 2000` (or `/ 1000` when `FUN_0053b900(club)` != 0), at least 250. `rep` = club +0x80 | step 0x23 | 0x829984 |
| Club +0x77 when <= 0: `rep*rep/10000` at least 75, or `rep*rep/3350` at least 750 | step 0x23 | |
| Club +0x7b when <= 0: (`rep*rep/1000` at least 12,500, or `rep*rep/500` at least 3,250) + `random(7500)` | step 0x23 | |
| Club +0x7f when not 1..20: `rep / 500` clamped to 1..20 | step 0x23 | |

`FUN_0053b900(club)` (160 bytes) returns non-zero for national teams (inference from its use in `FUN_00542740` and `FUN_00839920`).

New tables:

| Global | Size | Content | Evidence |
|---|---|---|---|
| `DAT_00ae2d1c` | clubs × 4 | Club ids sorted by comparator 0x540020 | `FUN_0053db40` |
| `DAT_00ae2d0c`, count `DAT_00ae2d10` | staff × 0x4f | Per staff: +0 = -1, +4 = -1, +8 = 0, +0xb &= 0x80, +0xa = a value from the date of birth, random numbers and the nation (inference: a run-time rating), +0xd = 0, 5 × {int -1, byte 0} at +0xe, 2 × 5 ints -1 at +0x27 | Ghidra lines 205..270 |
| `DAT_00ae2d14`, count `DAT_00ae2d18` | clubs × 6 | {int -1, byte 0, byte 0} | |
| `DAT_00adadfc`, count `DAT_00adadf8` = clubcomps | 4 × 579 | Competition object by comp id | `FUN_008386f0` |
| about 40 manager objects | see 3e | | |

### 3d. The start date

`FUN_00830520` (248 bytes, 0x830520..0x830617):

```c
for (i = 0; i < 35; i++) {
    entry *e = &league[i];                                     /* 0xb63d60, 0x48 bytes */
    if (!e->nation) continue;
    if (e->nation == DAT_00ae2d08 || (!DAT_00ae2d08 && (e->nation->flags & 2))) {
        DAT_00dd6ec8 = e->start;                               /* +0x2a */
        if (e->nation->continent->id == DAT_009cfa10 /* Europe, by 6 */ && DAT_00dd6ec8.year == 2001
            && DAT_00dd6ec8 > date(1, 8, 2001))
            DAT_00dd6ec8 = date(1, 8, 2001);                   /* 1 Sep 2001 */
        return 1;
    }
}
return 0;
```

`DAT_009cfa10` is set by the continent matcher for the third name, "Europe" (0x6042dc).

`FUN_00667150` writes the England entry (index 14, 0xb64150): +0x2a = `date(10, 7, year, 5)`, which is 10 Aug, moved to the nearest Saturday, and +0x39 = `date(31, 4, year, ...)` (0x667dd6..0x667e41). `year` is `DAT_00ae2c92`, the year of the game date, 2001.

Date helpers:

- A date is `{int16 day_of_year, uint16 year, uint16 leap, uint16 0}`. `day_of_year` is 0 for 1 Jan.
- `date(day, month, year, weekday)` = `FUN_00549f70` (thiscall on the result): `day_of_year = table[month] + day` with `table` = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333} (0x9b8448) or the leap table (0x9b8460, +1 from March). `leap` uses the Gregorian rule. Day must be 1..31 and month 0..11, else an error box. When `weekday` != -1, `FUN_0054a2d0` moves the date to the nearest day with that weekday, at most 3 days away (0 = Monday ... 6 = Sunday).
- Weekday (0x54a2d0): `y = year - 1600; n = y*365 + y/4 - y/100 + y/400 + day_of_year - leap; r = n % 7; wd = r == 0 ? 6 : r - 1`.
- `FUN_0054c8e0(n)` subtracts n days, `FUN_0054c5b0(n)` adds n days (inference from the use in 0x5cc850 and 0x54a2d0), `FUN_0054cab0(n)` subtracts n weeks.

Result for England [emu]: 10 Aug 2001 is a Friday, so the entry date is Saturday 11 Aug 2001. Less 4 weeks: 14 Jul 2001. `DAT_00dd6ec8` = {194, 2001, 0}.

### 3e. The 44 steps

The progress total is 44 (0x2c, 0x829fc6). Each step checks memory first. The steps make the manager objects, most through `operator new` and a constructor:

| Step | Global | Constructor | Object size |
|---|---|---|---|
| 1 | - | `FUN_0090cd50("*.tmp")`: delete temp files | - |
| 2 | - | `FUN_00669140`: number the league entries with a nation flag or no nation (+0x14), count in `DAT_00b64738` | - |
| 3 | `DAT_00de1d08` | `FUN_00893dc0` (9,248, tactics.cpp, formations) | 0x609 |
| 4 | `DAT_00b63c98` | `FUN_005fb430` (manager list) | 0x10 |
| 5 | `DAT_00b74448` | `FUN_007f6760` | 0xe8 |
| 6 | `DAT_00b7431c` | `FUN_00785b20` | 0x14 |
| 7 | `DAT_00ae2c74` | `FUN_005c1f80` | 0x3507 |
| 8 | `DAT_00de1f58` | `FUN_00903360` | 0x698 |
| 9 | `DAT_00ae1000` | `FUN_004c5fe0` | 0x75 |
| 10 | `DAT_00dd7f00` | `FUN_00868390` | 0x18 |
| 11 | `DAT_00b6b7c0` | `FUN_00729000(0, n)` (match stats), n = 5,000, 10,000 or 20,000 by settings bits 0x400, 0x800 | 0x20 |
| 12 | `DAT_00ad9dac` | `FUN_00444520` | 0x30 |
| 13 | `DAT_00b74350` | `FUN_007bfd50` (4,016, "Player histories") | 0x48 |
| 14 | `DAT_00ad9da8` | `FUN_00442ab0` | 0x10 |
| 15 | `DAT_00b5e134` | `FUN_005f97c0` | 6 |
| 16 | `DAT_00ae28f0` | `FUN_005554a0` | 0x2a |
| 17 | `DAT_00ad9c60` | `FUN_00413180` | 0xe0 |
| 18 | `DAT_00ae2a3c` | `FUN_005a4fc0` | 0xc |
| 19 | `DAT_00ad9c6c` | `FUN_00418440` | 0x10 |
| 20 | `DAT_00ae2a30` | `FUN_005697e0` | 8 |
| 21 | `DAT_00b7433c` | `FUN_00794ad0` | 0x1070 |
| 22 | `DAT_00b63d28` | `FUN_0062a680` | 0x2c |
| 23 | `DAT_00b74318` | `FUN_0076a4e0` | 0x3a |
| 24 | `DAT_00de2f70` | `FUN_0090d140` | 0x6a |
| 25 | `DAT_00dd7ef4` | `FUN_0085b670` | 0x19 |
| 26 | `DAT_00ae2a58` | `FUN_005a9cf0(year, 0, 1)` | 0x1184 |
| 27 | `DAT_00b74434` | `FUN_007e5fc0` | 0xab |
| 27 | - | `FUN_00669180`: **makes the competitions**, below | - |
| 28 | `DAT_00adadfc` | `FUN_008386f0`: competition objects by comp id, then `FUN_005ca030` | - |
| 29 | `DAT_00dd7ef0` | `FUN_00859aa0` | 2 |
| 30 | - | `FUN_007e61c0` | - |
| 31 | `DAT_00ae2a38` | `FUN_0059a370` | 4 |
| 32 | `DAT_00ae1050` | `FUN_004c7040`, then `FUN_004c7560` | 0x3c |
| 33 | - | `FUN_0059cf50` | - |
| 34 | `DAT_00de1f54` | `FUN_008bba30` | 0x94a |
| 35 | - | club finance fields (3c) | - |
| 36 | `DAT_00b7434c` | `FUN_007a5380` | 0x60 |
| 37 | `DAT_00b67a44` | `FUN_0068dbf0` | 0x1c |
| 38 | `DAT_00b6b7c8` | `FUN_0072b1d0` | 0x18 |
| 39 | `DAT_00ae2a60` | `FUN_005af630` | 0x40 |
| 40 | `DAT_00b74340` | `FUN_0079b580` | 0x10 |
| 41 | - | `FUN_00859dc0` | - |
| 42 | `DAT_00dd7ee4` | `FUN_0084b940` | 8 |
| 43 | `DAT_00de2f74` | `FUN_009226c0` (weather.cpp, reads weather.cfg) | 0xc |
| 44 | `DAT_00de1f44` | `FUN_008b0ec0` | 0x688f |
| 44 | `DAT_00b74430` | `FUN_007da190(DAT_009b5984)` | 10 |

Step numbers are the progress values (0x829.. pushes). The names of most objects are unknown. The assert strings in each constructor name its source file. A later inventory can list them.

Competitions, `FUN_00669180` (272 bytes):

```c
for (i = 0; i < 35; i++) {
    entry *e = &league[i];
    if ((e->nation == NULL || e->nation->flags) && e->setup) {
        if (!e->setup(e)) { error_box("key_nation.cpp", 0x1dc); return 0; }
        (void)date(e->start...); (void)date(e->end...);       /* results unused */
    }
}
```

The setup functions are 35 functions at 0x830620..0x8386f0 (all inside the Ghidra procedure `FUN_00830520`, which Ghidra did not split). Entries 0..7 have no nation and always run (inference: world and continental competitions). England is entry 14, setup 0x833750 (1,440 bytes). It makes ten competition objects [code 0x833913..0x833c66]:

| Object size | Constructor | Inference |
|---|---|---|
| 0xee | `FUN_00573380` | league |
| 0xee | `FUN_00571680` | league |
| 0xee | `FUN_005754a0` | league |
| 0xee | `FUN_00576dd0` | league |
| 0xee | `FUN_0056db60` | league |
| 0xb5 | `FUN_0056f040` | cup |
| 0xb3 | `FUN_0056c2a0` | cup |
| 0xba | `FUN_0056aa30` | cup |
| 0xb2 | `FUN_0056d380` | cup |
| 0xb2 | `FUN_00570c00` | cup |

Inference: five English leagues (Premier, First, Second, Third Division and Conference) and five cups. The constructors reach a strongly connected group of about 560 functions (fixtures, news, transfers), so each competition class pulls in most of the game. The fixture lists are made inside these objects. Their layout was not read.

### 3f. `FUN_005cc850`, run to the start date

1,152 bytes. Ghidra reads it. It runs the game slice by slice until the game date reaches the start date:

```c
void run_to_start(void)
{
    FUN_007a30c0(0x100, 0); FUN_007a30c0(0x1000, 0); FUN_007a30c0(0x800, 0); FUN_007a30c0(0x200, 0);
    int last_year = 0, step = 0;
    for (;;) {
        if (DAT_00dd6ec8 <= DAT_00ae2c90) {                    /* start date reached */
            (void)date_minus(DAT_00ae2c90, 1);
            DAT_00ae2cfc = 2;
            FUN_0083a1b0(); FUN_007a30c0(0x203b, 0); FUN_0062a920();
            return;
        }
        if (year != last_year) { FUN_005aa2d0(year); last_year = year; }
        DAT_00b67a50 = new match_day(&DAT_00ae2c90, DAT_00ae2c88);   /* FUN_006b3c10, 0x38 bytes */
        FUN_006b4360();                                         /* thiscall on DAT_00b67a50, 3,296 bytes */
        FUN_005aaf00(); FUN_0086dda0(&DAT_00ae2c90);
        for (c in competitions) c->vtbl[0x14/4](&DAT_00ae2c90, 0);
        delete DAT_00b67a50 (FUN_006b4190); DAT_00b67a50 = 0;
        FUN_004c7ac0(&DAT_00ae2c90, DAT_00ae2c88, 1);
        if (DAT_00ae2c88 == 2) {                                /* end of the day */
            for (s in staff) s->w18 = FUN_0053ab90(s, &DAT_00ae2c90);   /* age */
            FUN_00922ee0();
            progress("Initialising Leagues", step++, days(DAT_00dd6ec8 - DAT_00ae2c90 at entry));
            FUN_00556240(&d); FUN_0062b7b0(); FUN_009035a0(1); FUN_00413540(&d);
            for (c in competitions) c->vtbl[0x14/4](&DAT_00ae2c90, 1);
            FUN_005aaf00(); FUN_005fa850(&d); FUN_005d5ae0(); FUN_005d5f20();
            FUN_0076b100(&d); FUN_0059b9c0(&d); FUN_0079bf40(); FUN_0068f9c0(); FUN_0085b820(&d);
        }
        FUN_00448e30();
        if (++DAT_00ae2c88 > 2) {
            DAT_00ae2c88 = 0;
            DAT_00ae2c90 = DAT_00ae2c90 + 1 day;                /* FUN_0054c5b0 */
            if (DAT_009b5980 && !DAT_00ae233c) { ... FUN_0058a890 ... }   /* a fixed event date DAT_00ae2330 */
        }
        FUN_0068d520();
    }
}
```

The progress total is the day count from 1 Jan 2001 to 14 Jul 2001, 194 [emu]. So a new game simulates 194 days, about 582 slices, before the player sees a screen. Each slice runs the match day, every competition and the daily updates.

The game date after game creation is the start date, 14 Jul 2001 (inference from the loop condition; the emulator run did not reach the end, section 6).

## 4. Record layouts

Field names in the tables below are working names. "Code" gives the evidence. Fields with no code evidence are "unknown". The data-only guesses of the earlier inventory are not repeated.

### 4.1 Staff, 110 bytes in memory (`DAT_00ae23bc`)

| Offset | Size | Field | Meaning and evidence |
|---|---|---|---|
| 0x00 | 4 | id | Row index. Reserve init 0x52a1e6 |
| 0x04 | 4 | first_name | Id → first name row (0x5315a0). Empty name filled from the common names (staff pass) |
| 0x08 | 4 | second_name | Id → second name row |
| 0x0c | 4 | common_name | Id → common name row. Reserve rows: 0 |
| 0x10 | 8 | born | Date. Manager rows: 1 Jan 1966 (0x820e2a). Inference: date of birth |
| 0x18 | 2 | w18 | Set every day to `FUN_0053ab90(staff, date)` (0x5cc9a0). Reserve init 1971, manager 35. Inference: age |
| 0x1a | 4 | nation | Id → nation. Filled by the staff pass. The manager's nationality (0x822187) |
| 0x1e | 4 | nation2 | Id → nation. Inference: second nationality |
| 0x22 | 1 | b22 | If 0, +0x23 is cleared (0x52d2c4) |
| 0x23 | 1 | b23 | |
| 0x24 | 4 | national_club | Id → club (a nat-club row) |
| 0x28 | 1 | national_job | Job at the national team, same codes as +0x3d. 5 keeps a non-player in the national team (0x52d795). Inference: 5 = manager |
| 0x29 | 8 | d29 | Date. Compared with the start date in game creation |
| 0x31 | 8 | d31 | Date |
| 0x39 | 4 | club | Id → club |
| 0x3d | 1 | club_job | 1..10 need non-player data, 11 needs player data, 12, 13 and 15 need both, 14 needs none (table 0x52daa0). 7 becomes 6. Inference: 11 = player, 12, 13, 15 = player and coach |
| 0x3e | 8 | d3e | Date. Inference: contract end (staff pass sets 1 Sep 1997 for unemployed non-players with an earlier date) |
| 0x46 | 8 | d46 | Date |
| 0x4e | 4 | i4e | |
| 0x52 | 4 | i52 | |
| 0x56 | 1 | b56 | Used by game creation in the `DAT_00ae2d0c` value (`*(char*)(staff+0x56)` at Ghidra line 228) |
| 0x57..0x5d | 7 | | |
| 0x5e | 1 | b5e | Bit 1 cleared and bit 0 set when the club fails `FUN_00542740` (0x52d29e) |
| 0x5f | 1 | b5f | Reserve rows 6. Manager rows: 0 free, 5 being made, 1 human manager (0x820de5, 0x820ec1, 0x822e5a) |
| 0x60 | 1 | b60 | From file 0x60 |
| 0x61 | 4 | player | Id → player row (70 bytes). From file 0x91 |
| 0x65 | 4 | prefs | Id → preferences row (52 bytes) |
| 0x69 | 4 | nonplayer | Id → non-player row (68 bytes). From file 0x99 |
| 0x6d | 1 | b6d | |

File layout (157 bytes): 0x00..0x60 as above, 0x61..0x90 twelve ints (go to the preferences row), 0x91 player id, 0x95 unused, 0x99 non-player id (0x52cc45..0x52d012).

Row groups after the load: rows 0..169,341 from the file, 169,342..192,341 reserve rows (+0x5f = 6), 192,342..192,357 manager rows. The staff pass and the trim skip the reserve rows (+0x5f = 6) and the last 16 rows.

### 4.2 Player, 70 bytes (`DAT_00ae23c4`, id 10)

| Offset | Size | Evidence |
|---|---|---|
| 0x00 | 4 | id. Boundaries from the format-1 converter `FUN_0054f150` (database inventory 3.3) |
| 0x04 | 1 | |
| 0x05, 0x07, 0x09, 0x0b, 0x0d | 2 each | |
| 0x0f..0x45 | 55 × 1 | Bytes. +0x0f > 14 counts a player as wanted in a squad trim (`FUN_00541e60`, 0x541ef0). Meanings unknown |

### 4.3 Non-player, 68 bytes (`DAT_00ae23c0`, id 9)

| Offset | Size | Evidence |
|---|---|---|
| 0x00 | 4 | id |
| 0x04..0x0d | 5 × 2 | |
| 0x0e..0x22 | 21 × 1 | |
| 0x23..0x3f | 8 × 4 | Ids → staff (resolve, NULL when >= staff count) |
| 0x43 | 1 | |

The trim `FUN_00524d70` copies 10 random non-player rows over the last 10 rows (0x5260f4.., Ghidra lines 1043..1052).

### 4.4 Preferences, 52 bytes (`DAT_00ae23c8`, built)

`+0x00` id, `+0x04..+0x18` six club ids, `+0x1c..+0x30` six staff ids (resolve, error text "Preferences"). Inference: favourite clubs and favourite staff.

### 4.5 Club and nat club, 581 bytes (`DAT_00ae23b4`)

| Offset | Size | Field | Evidence |
|---|---|---|---|
| 0x00 | 4 | id | |
| 0x04 | 51 | name | Compared with "Bright", "Wimble" (0x82992b) |
| 0x38 | 26 | short name | Database inventory |
| 0x53 | 4 | nation | Resolve, log "club nation index". NULL means an extinct club (staff pass) |
| 0x57 | 4 | division | Club comp. Set to NULL for "Reserve". Set by `FUN_00839920` |
| 0x5b | 4 | last division | Club comp (log "last division") |
| 0x60 | 4 | | Club comp |
| 0x69 | 4 | stadium | Resolve, log "stadium" |
| 0x6d | 1 | b6d | 0 for Brighton and Wimbledon, else 1 (step 35). Inference: owns its stadium |
| 0x6e | 4 | | Stadium (inference: second stadium) |
| 0x73, 0x77, 0x7b | 4 each | | Set from the reputation when <= 0 (step 35). Inference: money values |
| 0x7f | 1 | | 1..20, from reputation / 500 |
| 0x80 | 2 | w80 | 0 becomes 1 (reader). Inference: reputation (used for the money values; nat-club pass halves it) |
| 0x83..0x97 | 6 × 4 | colours | Resolve |
| 0x9b..0xaf | 6 × 4 | staff | Resolve. Meaning unknown |
| 0xb3, 0xb7, 0xbb | 3 × 4 | clubs | Resolve. Inference: rivals |
| 0xbf | 4 | chairman | Resolve log text |
| 0xc3 | 3 × 4 | directors | |
| 0xcf | 4 | manager | Cleared by the staff pass |
| 0xd3 | 4 | assistant manager | |
| 0xd7 | 50 × 4 | squad | Staff ids |
| 0x19f | 5 × 4 | coaches | |
| 0x1b3 | 7 × 4 | scouts | |
| 0x1cf | 3 × 4 | physios | |
| 0x1db | 4 | | Set to -1 by game creation |
| 0x1e0 | 20 × 4 | "team member selected" | Staff ids |
| 0x244 | 1 | b244 | 1 when the club is in a league with a nation (`FUN_00541e60`) |

### 4.6 Club comp and nation comp, 107 bytes (`DAT_00ae23d0`)

`+0x00` id, `+0x04` name (51), `+0x38` short name (27), `+0x53` byte, `+0x57`, `+0x58` bytes, `+0x59` continent, `+0x5d` nation, `+0x61`, `+0x65` colours, `+0x69` short (compared between divisions in `FUN_00839920`; inference: level). Evidence: the reserve-row init (0x52a068) and the resolve pass.

### 4.7 Nation, 290 bytes (`DAT_00ae23a8`)

As in the start-new-game inventory 3a, with these additions from the resolve pass: +0x7a city, +0x80 stadium, +0x90..+0xa4 colours, +0x110..+0x118 nations. +0x88 is set to 0 for every nation by the trim (Ghidra line 1057). +0x8e is read as a short in the trim (line 129).

### 4.8 Name tables, 60 bytes (first, second, common)

`+0x00` name (51), `+0x33` id, `+0x37` nation id, `+0x3b` byte, 1 for the blank rows. Evidence: the manager-row init 0x52add4..0x52aee0. Inference: +0x3b is a use count.

## 5. Random numbers

### 5a. Generators

1. MSVC `rand()` (0x946a04): `holdrand = holdrand * 214013 + 2531011; return (holdrand >> 16) & 0x7fff`. State `DAT_00ad6c30`. `srand` is 0x9469fa.
2. `FUN_0090cfc0(n)`, the main generator (3,772 calls):

```c
int random(int n) {                                             /* 0x90cfc0 */
    if (n == 0) return 0;
    DAT_00de1f60 += 4;                                          /* pointer into the table */
    if (DAT_00de1f60 > DAT_00de2798) {                          /* end, 0xad4130 */
        DAT_00de1f60 = 0xaa2454;
        DAT_00de1f5c = rand() & 0xffff;                         /* offset */
    }
    if (n <= 0xffff && n >= -0xffff) {
        int r = (DAT_00de1f5c + *DAT_00de1f60) % n;             /* C remainder, signed */
        return n < 1 ? -r : r;
    }
    int lo = (int)(((DAT_00de1f5c + *DAT_00de1f60) % 0xffff) * (double)n * K);   /* K at 0x970a70, __ftol */
    int hi = random(n * 10 / 0xffff);
    int v = lo + (hi + 5) / 10;
    return v > n - 1 ? n - 1 : v;
}
```

The table is 51,000 ints at 0xaa2454..0xad4130 in the exe's data section (first values 25136, 18392, 48707). The port can copy the table from the user's exe at start, or carry it as a generated file.

3. Seeding, `FUN_0090d0a0(seed)` (0x90d0a0):

```c
if (seed == 0) seed = time(NULL);
srand(seed);
sprintf(0xde1f64, "Random Number seed: %d", seed);             /* log only */
DAT_00de1f60 = 0xaa2454 + 4 * (rand() % 51000);
DAT_00de2798 = 0xad4130;
DAT_00de1f5c = rand() % 0xffff;
```

It runs once, in `FUN_0081b890` when the start screen opens (0x81b9b2): seed = the number after "-seed" on the command line (0x81b97e..0x81b99e), else 0, which means the time. `DAT_00dd7120` holds the "-seed" value, else the value of a CRT clock call (`FUN_00947299` at 0x81b95e). `FUN_005cc6b0` also calls it with 0 (0x5cc6b2).

Other seeds: `srand` is also called at 0x686b34 (year + `DAT_00dd7120`), 0x696b8c, 0x90de58 and 0x90dfff with values made from records. These change `holdrand` only, not the table pointer. They are not on the creation path as far as read (unknown for the 194-day run).

### 5b. Use on this path

- The load: `random(nation_count)` for a staff row with no nation and no club (0x52d3aa), and `random(common_count)` when no empty common name exists. Inference: neither runs with the shipped data, so the load uses no random numbers. The emulator run gives the same tables with seed 12345 as the code predicts [emu].
- Game creation: `FUN_00535a80`, `FUN_00524d70`, the `DAT_00ae2d0c` init, step 35, and the 194-day run use `random()` many times, and `rand()` directly in some places (`FUN_00524d70` line 1107).

### 5c. For a differential test

Give both sides the same three values before a call: `DAT_00ad6c30` (holdrand), the table index `(DAT_00de1f60 - 0xaa2454) / 4`, and `DAT_00de1f5c`. The end pointer `DAT_00de2798` is a constant 0xad4130 after seeding. After the call, compare the three values too: the count of `random()` calls must match. Seed both sides with `FUN_0090d0a0(fixed seed)`, never 0.

## 6. Functions the port lacks

The port has no game logic. `src/cm3/database.c` is a 38-line stub. None of the functions below exist in the port.

### 6a. The emulator run

The run (scratchpad script `gc/trace/trace.py`, based on `tests/diff/original.py`) records the first entry of each function, per phase. It replaces these with host code: `malloc`, `operator new`, `calloc`, `realloc` (in place when shrinking, needed by `FUN_00524d70`), `fopen` (writes go to a sandbox folder), `fread`, `fwrite`, `fseek`, `ftell`, `fgetc`, `fclose`, `_access`, `_makepath`, `sprintf`, `time`, the findfirst wrappers, the progress box, the error box, free memory, and the Windows imports `GlobalAlloc`, `GetLogicalDriveStringsA` ("D:\"), `GetDriveTypeA` (5), `HeapSize` and some more.

RESULTS_PLACEHOLDER

### 6b. Grouped list

GROUPS_PLACEHOLDER

## 7. Proposed slice order

SLICES_PLACEHOLDER

## 8. Open doubts

DOUBTS_PLACEHOLDER
