# Assistant selection and faster days: inventory

Research for slice 5, part B, of `docs/specs/2026-10-14-quality-of-life-0.2.0-spec.md` (issues #12 and #20). The original is `clean-3968/cm0102.exe` (3.9.68). All addresses are in the original. "Inference" marks a statement that the code does not show directly. "Unknown" marks what I could not find.

Runs: `build/cm0102_recomp` of the main checkout on 2026-10-14, with the smoke steps to a new game (Arsenal), then Go on Holiday, a screen dump every 2 s for 40 s, and `sample` (macOS) for 10 s on holiday. A second session ran builds and smoke runs on the same Mac at the same time, so the times below are slower than on an idle Mac.

## Summary

- The original already has the assistant selection, but only on holiday. On holiday, the match code picks the human club's team with the code for computer-run clubs. Two holiday settings keep the player's tactic ("Use current match tactics") and the player's eleven ("Use current team selection when possible"). Issue #12 can reuse this path. It keeps the tactic.
- That code draws random numbers. The player's own selection draws none. With the option on, the random sequence of the whole game changes. With the option off, nothing changes.
- On holiday, about 60% of the time is a fixed wall-clock hold of 2,000 ms on each held screen (0x76224c). Drawing is under 1%. Faster days can end the hold at once. The hold reads the clock only, so this changes no game state.
- Warning: two runs with the same `-seed 1234` and the same scripted input gave different European qualifying draws by 9.8.01. The game is not deterministic now. The same-save check cannot pass until the cause is found and pinned. The cause is unknown. The first suspect is `srand(time())` at 0x696d51.

## 1. Assistant selection (#12)

### 1a. Where the team is picked

The match code builds each side of a match with `FUN_006db500` (match_man.cpp, asserts at 0xa15da4 lines 0x69 to 0x71). Callers: 0x6b8866 and 0x6b89b9, in `FUN_006b7f20`, which the match-day processing `FUN_006b4360` calls (sample stacks: 0x5cccd0 > 0x6b4360 > 0x6b7f20 > 0x6db500 > 0x6dbc50).

`FUN_006db500` is thiscall on a match-side object (ECX). Ghidra shows 8 stack arguments: (staff, squad list, player slots, side, club, opponent club, fixture, out count). It stores the club at +0x2f, the opponent at +0x33 and the fixture at +0x37. It sets +0x19 = 1 when the club's fixture flag (+0x40 home, +0x41 away) is 0, else +0x19 = 0. Inference: +0x19 = 1 is a reserve-team side. The code at 0x6db9xx asks the "Control Reserve Team" flag (`FUN_005fd480`, manager record +0x9a05, strings 0xa5a638 and 0xa5a8c0, toggle 0x7628ba) only when +0x19 != 0.

Branches of `FUN_006db500` (0x6db500..0x6dbb0f):

| Case | Test | What runs |
|---|---|---|
| Human club, first team, manager not on holiday | `FUN_0053ca90(club)` and `FUN_00600a50(club,0,1,0,0)` != 0, +0x19 == 0 | Copies the human's tactic block (0x18e3 bytes from `FUN_005fbd00(manager)`) into the match side (+0x2b), then `FUN_006dbb80`: takes the player's stored selection slot by slot, drops injured, ineligible and banned players. No random draws in it (callees 0x8968f0, 0x62c400, 0x55d270, 0x7757d0, 0x6e0d00). |
| Human club, reserve side, "Control Reserve Team" off | +0x19 != 0, `FUN_005fd480` == 0 | `FUN_006dbc50(opponent, 1)`: the computer pick. |
| Computer club, or human manager on holiday | `FUN_00600a50(club,0,1,0,0)` == 0 | `FUN_006dbc50(opponent, 2)`: the computer pick. |

`FUN_00600a50(club, allow_holiday, any, manager, p5)` (human_manager.cpp, 0x600a50): with `allow_holiday` = 0, a manager whose holiday flag is set (`FUN_005fd210`, record +0x9a01, or `FUN_005fe040`) does not count, so the club counts as computer-run.

### 1b. The computer pick

`char FUN_006dbc50(int opponent, char mode)`, thiscall on the match side, 0x6dbc50 (1,181 lines of pseudocode). Mode 2 is the full pick, mode 1 the reserve pick. It reads:
- the squad (+0x27 list, 50 players), player condition, injuries (`FUN_0062c400`), bans (`FUN_007757d0`), eligibility (`FUN_0055d270`), club reputation (club +0x80), opponent reputation, competition type (fixture +0x3a),
- position ratings through `FUN_005b2ae0`, `FUN_005b50d0` and `FUN_00717410`/`FUN_00717450` (inference: formation position flags),
- the formation and tactic through `FUN_006e0230` (called at 0x6dd057) and per-player instructions through `FUN_006ddab0` (called at 0x6dd732).

It assigns players with `FUN_006e2000(players, count, positions, ..., flags, club)` (calls at 0x6dc9xx and 0x6dd3xx) and writes the result back to the club's stored selection: `FUN_00896fe0(club, +0x19, 0, 0)` clears it in mode 2 (0x6dbc50, pseudocode line 500), then `FUN_00896a90(club, slot, player, +0x19)` per slot and `FUN_00897760` for the captain (lines 1004 and 1007).

### 1c. Where the selection is stored

The tactics object at `[0xde1d08]` (tactics.cpp, asserts 0xa8b908). Its table at +0x601 has one record of 0x91 bytes per (club, team index), found with `FUN_008982b0(club, index)`. Record +2 + slot*4 is the player index for slots 0 to 19 (-1 is empty). Record +0x52 is the captain. Readers and writers: `FUN_008968f0` (get), `FUN_00896a90` (set), `FUN_00897760` (captain), `FUN_00896fe0` (clear, with an undo copy per human player at +0x30, 16 x 0x5c bytes).

The player's tactic sets live in the human manager record (base `[0xb63c98]`, 0xa6ef bytes per human). `FUN_005fbd00` returns the active set: record + `char[+0x9a69]` * 0x1ecd.

### 1d. The holiday settings already in the original

The "Go on holiday ?" screen `FUN_006b2d80` (string 0xa156f4) shows seven settings, read with `FUN_005fea50(index, manager)` = byte at human record +0x9b23 + index (0x5feadc):

| Index | Text | Address of text |
|---|---|---|
| 0 | Reject all offers for players / below asking price | 0xa156c8, 0xa156a0 |
| 4 | Do not buy any players / Only buy from shortlist | 0xa15688, 0xa15668 |
| 1 | Apply for manager position at higher clubs | 0xa15630 |
| 2 | Use current match tactics | 0xa15614 |
| 3 | Use current team selection when possible | 0xa155e8 |

Readers in the match code, each only when +0x19 == 0, the manager is human and `FUN_005fd210` (on holiday) != 0:
- index 2 in `FUN_006e0230` (0x6e030f): calls `FUN_005fbe20(0, club, manager)` and `FUN_005fbd00(manager)`, then jumps to 0x6e0c02. Inference: it uses the player's tactic block instead of a computer tactic.
- index 2 in `FUN_006ddab0` (0x6ddb3f): returns at once, so no computer per-player instructions.
- index 3 in `FUN_006dbc50` (0x6dc986, 0x6dd315) and in the substitution code `FUN_006e1090` (0x6e13b4) and `FUN_006e1aa0` (0x6e1c01): adds flag 0x400000 to `FUN_006e2000`. Inference: prefer the players of the stored selection.

There is no first-team assistant pick outside holiday. I found no "auto pick" button or string.

### 1e. Can it run for the player's club, keeping the tactic?

Yes, by the holiday path. Design for slice 6 (inference, not tested):
- `RT_WRAP` 0x6db500. When the option is on, the club is the player's club and +0x19 will be 0, set a runtime flag for the call.
- While the flag is set: `FUN_00600a50(club, 0, ...)` returns 0 for that club, `FUN_005fd210` returns 1 for that manager, `FUN_005fea50(2, manager)` returns 1 (keep tactic), `FUN_005fea50(3, manager)` returns 0 (the assistant picks freely).
- Clear the flag on return. Do not set it in the match-engine substitution code (0x6e1090, 0x6e1aa0, 0x6ddab0 called from 0x6e0c4a), so the player's own substitutions stay as now.

The tactic is kept by the original's own branch at 0x6e02b6..0x6e0347. The spec's "stop and ask Max" condition does not apply. The formation also comes from that tactic block (inference).

When: inside match processing, after the last human screen of the day and before kick-off. So the pick is "before the team sheet" in game terms. The player's tactics screen before the match still shows the player's own selection. After the match, the stored selection is the assistant's (it overwrites the record at [0xde1d08], 1b).

Side effects:
- Random numbers: yes. `FUN_006dbc50` has 7 calls to `FUN_0090cfc0`, plus its callees (`FUN_006e0230` draws at 0x6e02a2 in its network branch, other sites not traced). The player's path (`FUN_006dbb80`) draws none. With the option on, every later random draw in the game shifts, so other clubs' results and news also differ. With the option off the wraps do nothing, so the same-save check must pass.
- Morale, news: unknown. I saw no news call in `FUN_006dbc50`. Player morale reacts to selection in later code (inference from strings such as 0x618001), as it does for any selection.
- The stored selection is overwritten (1b). The undo copy of `FUN_00896fe0` is not used (third argument 0).

## 2. Faster days (#20)

### 2a. The day loop

`FUN_005cccd0` (game.cpp, 0x5cccd0), after the intro and Manager Status, loops forever (0x5cd2b4..0x5cde2f). One pass is one slice: `DAT_00ae2c88` is 0 Morning, 1 Afternoon, 2 Evening (strings 0x9b8a84, 0x9b8a8c, 0x9b8a98). Per pass:
1. `FUN_006b3c10(&date, slice)` builds the match day object (match_day.cpp).
2. `FUN_006b42a0`: if there are matches, sets `DAT_00b67a48` = 1, pushes the match-day screen `FUN_00717cc0` and runs the screen loop `FUN_007fc7e0`.
3. `FUN_006b4360`: plays the matches (match engine 0x6b98c0 and callees).
4. Each competition's day function (vtable +0x14), `FUN_0086dda0`, then `FUN_006b42a0` again (results screen).
5. If the date or slice is new: the daily updates (0x5cd3a4..0x5cdbdf, weekly when day % 7 == 0, monthly when day % 30 == 0).
6. The human turn `FUN_005cde40` until it returns 0: pushes the news screen (`FUN_00788010`) or the team screen (`FUN_004542f0`) and runs the screen loop.
7. Next slice. Every third slice moves the date.
8. `FUN_0054ba20` formats the date and slice into 0xde1f64. `FUN_00802f80(text, -1, 0, -1)` sends it as screen message 7 and applies it with `FUN_00564080` (draws the date box). Then the pump `FUN_0068d520`.

During match processing the status bar at the bottom shows "Friendly - Feyenoord 3 Newcastle 3 ... 33 %" (seen in the holiday dumps).

### 2b. What holds the screen on holiday

The node input functions pass "no widget" (-1) to the side bar handler `FUN_007615d0` in every pass of the screen loop (match-day input `FUN_00718930` at 0x718930, news input 0x7892b0). For a manager on holiday (`FUN_005fd210` at 0x762231) it compares:

```
0x762245 call 0x5fdc30        ; holiday stamp, human record +0x9a65 (ms)
0x76224c add esi, 0x7d0       ; + 2000
0x762252 call 0x90c510        ; ms clock (GetSystemTimeAsFileTime, 0x947299)
0x762257 cmp eax, esi
0x762259 jl 0x762389          ; less than 2 s: keep the screen
```

After 2 s it returns -5 at 0x762e75, which ends the screen (Continue), unless free memory is low (0x76218f). The stamp is set by `FUN_005fdd30` at 0x71b444 (match-day screen), 0x761233, 0x7624dc, 0x76255e, 0x7625e3 and at 0x762380 (any input on holiday restarts the hold). `FUN_005fdc30` has one caller, 0x762245.

The hold is a busy loop. No sleep exists in the original (no Sleep, GetTickCount or timeGetTime import). Each pass runs the pump, the input poll and the hit test.

### 2c. Measured

Holiday from Saturday 14.7.01 to Thursday 9.8.01 (26 days) took about 38 s: about 1.5 s per game day. The date box changes only on the days with a held screen (14.7, 18.7, 21.7, 25.7, 28.7, 1.8, 4.8, 7.8, 8.8, 9.8).

Main thread, 10 s sample on holiday (8,443 samples of 1 ms):

| Where | Samples | Share |
|---|---|---|
| Screen loops in the human turn and the two match-day screens (`FUN_007fc7e0` under 0x5cde40 and 0x6b42a0) | 5,255 | 62% |
| of which the input poll and hit test (0x55f620, 0x560490) | 3,821 | 45% |
| Match processing (`FUN_006b4360`) | 2,542 | 30% |
| Drawing: 0x55f300, 0x564080, 0x802f80, present | under 70 | under 1% |
| Screen dumps of the test script | 13 | 0.2% |

So the gain is in the hold. Skipping drawing gains under 1%.

### 2d. Does anything count passes or frames?

- In the 10 s sample, no random draw (`FUN_0090cfc0`) ran under the screen loop. All 110 self samples of it were under match processing or the daily updates.
- Random draws exist in code beside the match-day screen: `FUN_00716e10` (0x71705a, 0x717074) and sites 0x717323..0x717bc9. Whether they run per pass is unknown. The sample saw none.
- The pump tick `DAT_00b67a08` (ms, 0x68d5aa) is read by the hit test for a 300 ms hover (0x560eee..0x560f62) and by 0x5ee451, 0x5eec2d, 0x5eecbd. Inference: tooltips, display only.
- The screen loop's own `time()` checks (0x7fcc54..0x7fccbd, timer at scrman +0x26279c) end a screen after a timeout. +0x26279c is only reset to 0 (0x7fc40a). Inference: network turn timer, unused in single player.
- `FUN_007615d0` has two 5-second `time()` checks (0x762099..0x7620be and 0x7622dd..0x762302). The second runs only when the player slot has no manager (`[0xb74c26 + slot*0x300]` == 0). Inference: network or spectator paths.

Unlike the +/- repeat (user32.c), the hold counts wall-clock time, not passes. Ending it early changes how many passes run. No pass-driven game logic was found, but the list in this section is not a proof.

### 2e. Recommended design

1. Option `faster_days` (default on). When it is on, `RT_WRAP` 0x5fdc30 returns the stored stamp minus 2000. The comparison at 0x762257 then passes in the first pass. The stamp in the record stays as the game wrote it. Nothing else changes. Check the call site 0x762245 in the address list so the build stops if it moves.
2. Optional: while the hold would still run, the runtime need not present more often than 60 times a second. This gains little (2c).
3. Do not skip screen builds or the pump. Screen builds may draw random numbers (2d).

### 2f. The same-save check: what must come first

Two runs with `-seed 1234` and the same script were equal on screen up to 28.7.01. By 9.8.01 a European qualifying draw differed (other pairings). Faster days was not in that build. So the original's results now depend on something outside the seed and the input. Known sources of wall-clock data in the original:

| Source | Address | Effect |
|---|---|---|
| Seed from the clock unless `-seed n` is on the command line | 0x81b963 (strstr "-seed", 0xa801d4), 0x90d0a0 (seed 0 = `time()` at 0x90d0aa) | Use `-seed n`. `main.c` passes extra arguments through. |
| `srand(time())` after a deterministic `srand` | 0x696d51 in `FUN_00696b10`, called from 0x8cdb80 < 0x8cfc90 (how it is reached: unknown) | The C `rand()` state (0xad6c30) reseeds the random table offset at each table wrap (0x90cff1) and feeds `rand()` callers 0x539bcd..0x53a4fc, 0x86367f, 0x8656b4. First suspect. |
| Holiday stamp in the human record | +0x9a65, set from the ms clock | Saved bytes can differ (whether the record is saved: unknown). |
| `DAT_00dd7120`, the ms clock at game creation without `-seed` | 0x81b96d | With `-seed` it holds the seed (0x81b9aa). |

`time()` (0x946ebb) reads GetLocalTime and GetSystemTime. The ms clock (0x947299) reads GetSystemTimeAsFileTime. So a test-only runtime switch can fix the seconds clock (for example 2001-07-14 12:00:00) and leave the ms clock running, which the holds need.

Recommended order for the check:
1. Add a save step to the smoke script and a byte diff that names the differing offsets.
2. Run normal days twice with `-seed 1` and the fixed seconds clock. The saves must be equal. If they are not, the diff shows what to pin next.
3. Then run faster days off against on.

## 3. Estimates

| Build | Sessions | Notes |
|---|---|---|
| Slice 6, assistant selection | 2 | One to wrap 0x6db500 and the four readers and check a match. One to verify the tactic is kept (screenshot of the tactics screen and the match lineup) and to review. |
| Slice 10, faster days | 1 | One wrap and the option. Plus a measure. |
| Same-save check (needed by slices 6 and 10) | 1 to 3 | Save step, diff tool, fixed seconds clock: 1. Finding a further source of drift, if step 2 fails: up to 2 more, unknown. |

## Unknown

- How `FUN_00696b10` (srand from time) is reached, and whether it runs in the first weeks of a game.
- The exact cause of the drift between two seeded runs (2f).
- Whether the human record (+0x9a65) is in the save file.
- Whether `FUN_006dbc50` causes news or morale changes by itself.
- The meaning of `FUN_006e2000` flags other than 0x400000.
