# Assistant pick outside a match: inventory

Research for an "assistant picks the team" action on the club squad screen (build 0x454900). The runtime must pick the human club's first team (11 players and the substitutes) at any time, with no match running. The original is `clean-3968/cm0102.exe` (3.9.68). All addresses are in the original. "Code" facts come from the assembly (objdump of the exe) or the Ghidra pseudocode. "Inference" marks a conclusion that the code does not show directly. "Unknown" marks what I could not find.

Random reach was checked with a call graph built from all direct `call` instructions. It does not see indirect or vtable calls (for example the rule object call in 0x8cf5e0). The error box path (0x5e8290, 0x945934) was left out, because it only runs on a failed assert.

## Summary

- Q1: no. Do not call 0x6db500 or 0x6dbc50 outside a match. They need a real match object of more than 50 KB, a real fixture and the match player records. They exit the game on a failed assert, write into the match engine records and draw many random numbers (7 direct draws, plus about 17 per placed player in 0x6eaea0).
- Q2: yes. `FUN_006e2000` is the assignment core of the original pick. It is cdecl, it takes a player list and a list of position masks, and it accepts NULL for the match side. It draws no random numbers itself. The only random reach is the daily lazy fitness update 0x62c680, which the condition and injury getters run too. It is the best picker. The runtime then writes the result with 0x896fe0, 0x896a90 and 0x897760.

## Q1. Can the original pick run outside match setup?

### The call that the match code makes

`FUN_006b7f20` builds both sides (0x6b8835..0x6b8866 home, 0x6b8987..0x6b89b9 away):

```
side = match + 0x9032 (home) or match + 0x9102 (away)      ECX
FUN_006db500(match, club->manager (+0xcf), &club->squad (+0xd7),
             match + 0x4796 (home) or + 0x6a6e (away),
             side 0/1, club, opponent, fixture = [match + 0x4792], &out_count)
```

`FUN_006db500` is thiscall with 9 stack arguments (`ret 0x24` at 0x6dbb0d). It returns the number of selected players in AL. The caller shows an error message when it is below 11 (0x6b886b). Asserts that end the game (0x945934(-1)) fire when argument 1, 3, 4, 6 or 7 is NULL (lines 0x69, 0x6b, 0x6d, 0x6f, 0x71, 0x6db500..0x6db752). The fixture (argument 8) has no assert. It is read at once (0x6db7c1), so NULL crashes.

### The match side object (0xd0 bytes = 0x9102 - 0x9032)

Set in 0x6db752..0x6db7eb, unless noted.

| Offset | Size | Field | Evidence |
|---|---|---|---|
| +0x00, +0x02, +0x04 | 2 each | manager non-player words (+0xa, +0x8, +0xc of staff+0x69) | 0x6db7f2..0x6db806 |
| +0x06, +0x0a, +0x0e | 4 each | three key players, cleared, filled when flag 0x80000 | 0x6db79c..0x6db7a7, pseudocode of 0x6dbc50 near 0x6dd020 |
| +0x12 | 4 | match object (argument 1) | 0x6db78e |
| +0x16 | 1 | "important match" flag, adds flag 0x800000 | 0x6dbc50 |
| +0x17 | 1 | substitutions allowed = min(fixture+0x4d >> 4, selected - 11) | 0x6dbac0..0x6dbaed |
| +0x18 | 1 | side, 0 home, 1 away | 0x6db778 |
| +0x19 | 1 | team index: 1 when the club's fixture flag (+0x40 home, +0x41 away) is 0, else 0. Inference: 1 = reserve team | 0x6db7c1..0x6db7e7 |
| +0x1a | 1 | rotation level: 0 when +0x19 = 1, else 3. 0x6dbc50 sets 0..2 for computer clubs | 0x6db7dd, 0x6db7e7 |
| +0x1b | 4 | manager staff record (argument 2) | 0x6db75d |
| +0x1f | 4 | manager non-player data (staff+0x69) | 0x6db763 |
| +0x23 | 4 | 20 match player records of 0x1be bytes (argument 4) | 0x6db76e |
| +0x27 | 4 | squad list, 50 staff pointers (argument 3, club+0xd7) | 0x6db77b |
| +0x2b | 4 | tactic block inside the match: match + 0x91e2 + side * 0x18e3 | 0x6db7ad..0x6db7bb |
| +0x2f | 4 | club | 0x6db788 |
| +0x33 | 4 | opponent club | 0x6db7be |
| +0x37 | 4 | fixture | 0x6db796 |
| +0x3b | 4 | captain (`FUN_006e1940` result) | 0x6dc8a6 |
| +0x3f | 0x91 | copy of the stored selection record (`FUN_008983f0`) | 0x6dbaf0..0x6dbafc |

### What the pick reads and writes outside the side object

- Match object: +0 byte (non-zero skips the human branch, 0x6db80a, meaning unknown), +0x1ad + side byte, +0x476a + side*4 float, +0x4792 fixture, +0x4796 player records, +0x8ea5 (in 0x6df460), +0x8ed0 (in the scorer through the player record +0x19e), +0x91e2 tactic blocks. Size more than 0xc3a8 (inference from the tactic offsets).
- Fixture: +0x00 id (-2 has its own branches), +0x14 competition, +0x1c / +0x20 home / away club, +0x3a competition type, +0x40 / +0x41 team flags, +0x4d squad sizes (low nibble + 11 = players in the match squad).
- Opponent: reputation +0x80 (flag choice at 0x6dce16).
- `FUN_006e0d00(slot, player, slot)` places each player in the match engine through `FUN_006eaea0` (thiscall on the 0x1be record). 0x6eaea0 has 17 calls to the random function 0x90cfc0.
- After the pick, 0x6dbc50 changes the tactic block at +0x2b: `FUN_006ddab0` (per-player instructions, 2 random calls) and `FUN_005b50d0` (sets bits of tactic +0x584, 0x6dd810..0x6dd832). It must never get the player's own tactic block.
- `FUN_006e0230` (0x6dd057) chooses a computer tactic into +0x2b, unless holiday setting 2.
- `FUN_0055d270` can clear the player from club +0x1e0 (see Q2).

### What breaks with a fake buffer

| Item | Effect |
|---|---|
| NULL opponent | Fatal assert (0x6db6df). 0x6dbc50 reads opponent +0x80. |
| NULL fixture | Crash at 0x6db7c1. |
| Fake fixture | Needs +0x14 competition pointer, +0x3a, +0x4d. `FUN_008cf880` and `FUN_006e8910` (0x6dd47b, 0x6dd615) apply competition rules through `[0xde1f54]`. Unknown which rules. |
| Zeroed match buffer | Match player records get set up by 0x6eaea0 in the buffer. That code reads the fixture through record +0x19e. A zeroed float at +0x476a changes the tactic switches (0x6dd7f0 region). |
| No players selected | `FUN_006e1940` returns 0 and the game exits (0x6dc8a6..0x6dc92c, 0x6dced9). |
| Network | +0 byte of the match and `FUN_006e0230` network branch (0x6e02a2). Not analysed. |
| Random numbers | 7 direct draws in 0x6dbc50, 17 per player in 0x6eaea0, more in 0x6ddab0 and 0x6e0230. |
| Allocations | `FUN_00945501` and `FUN_009452ca` in pairs, so no leak was seen. |

Calling 0x6dbc50 directly has the same needs. It is thiscall on the side, `(opponent, mode)`, `ret 8` (mode 2 = full pick, mode 1 = reserves).

### Next fixture of a club

Yes. `FUN_005acbe0`, thiscall, ECX = `[0xae2a58]`, 9 stack arguments, `ret 0x24`. 0x6dbc50 calls it at 0x6dbe42..0x6dbe7e:

```
FUN_005acbe0(from_day, from_year, to_day, to_year, club_id (club+0), &list,
             competition = -1 (any), type = -2 (any), team = 1)
```

It returns the number of fixtures (short). It walks day by day from the first date (inference: the first list entry is the earliest). The list is the object that `FUN_0068d020` makes (ECX = list). The caller rewinds it with list+0xa = list+0x2 (0x6dc097), takes fixtures with `FUN_0068d1b0` (ECX = list, returns a fixture or 0) and frees it with `FUN_0068d040`. `team` is compared with fixture +0x40 / +0x41. Inference: 1 = first team. Dates come from `FUN_0054c5b0(&out, days)` with ECX = 0xae2c90 (today + days). 0x6dbc50 uses today+1 to today+7 (0x6dbe1f, 0x6dbe30). The call graph shows no random reach.

### Recommendation

No. The risk is high: a fake match object needs many fields, three asserts end the game, and the pick writes into match engine records and draws many random numbers. Use `FUN_006e2000` (Q2).

## Q2. Data for our own picker

### Squad and records

- Squad: club +0xd7, 50 staff pointers, NULL for empty (game creation inventory 4.5, and 0x6b8850 passes `club+0xd7`). There is no count field. Skip NULL entries and entries with staff +0x61 == 0 (0x6dc6b9 loop, 200 bytes).
- Staff record: base `[0xae23bc]`, 0x6e bytes, count `[0xae2364]`. Index to record = `[0xae23bc] + index * 0x6e` (0x8968f0). Staff +0 is the index.
- Player data: staff +0x61, 70 bytes in `[0xae23c4]` (game creation inventory, 0x5315a0).
- Manager: club +0xcf (0x6db85c).

### Position ratings and ability

- Player data bytes, scale 1 to 20: +0x0f goalkeeper, +0x10 sweeper, +0x11 defender, +0x12 defensive midfielder, +0x13 midfielder, +0x14 attacking midfielder, +0x15 attacker, +0x16 wing back, +0x17 right, +0x18 left, +0x19 centre, +0x1a free role. Evidence: `FUN_005b7d20` (cdecl, `(player_data)`, returns a mask from ratings of 15 and up, then 10 and up) and the scorer 0x6e2f20 (wing back +0x16 for D or DM with a side, free role +0x1a with AM centre). The names right / left for +0x17 / +0x18 are an inference from the 0x6dbc50 rest lists (0x804, 0x84).
- Rating for a slot: the scorer `FUN_006e2f20` (cdecl, 15 arguments, see below). It adds a table value `[0xa15cfc + rating * 4]` for the position, condition, and other terms. It is the original's own "player for this position" value.
- The squad screen position text: 0x53ee50 (columns inventory). Not traced further.
- Current ability: unknown from code. Inference from the community layout: player data +0x05 short. The reputation words +0x09, +0x0b, +0x0d are copied into the match record (0x6eaea0).

### Condition

- `FUN_0062c590(staff, percent)`, thiscall, ECX = `[0xb63d28]`, `ret 8`. With percent = 1 it returns 0..100. With 0 it returns 0..10000. Value = (word +0xa * word +0x8) / 10000 of the fitness record `[[0xb63d28]] + index * 0x1f`. Capped at 9000 when `FUN_0062c400` returns 1.
- Match fitness as a separate value: unknown. The two words +0x8 and +0xa are the only inputs. Which one is "match fitness" is unknown.
- Both getters first run `FUN_0062c680`, the daily update of that record. It returns early when the record date equals today (0x62c680 pseudocode). Its body calls 0x90cfc0 (random) in one branch.

### Age and date

- Today: `DAT_00ae2c90`, 8 bytes: +0 short day of year, +2 short year, +4 leap-year flag (0x54c5b0).
- Age: staff +0x18 short, set every day (game creation inventory, 0x5cc9a0). Or `FUN_0053ab90(staff, &date)`, cdecl, from the birth date at staff +0x10. Under 21: age < 21 (inference, no game code for this was found).

### Availability tests

| Test | Call | Result |
|---|---|---|
| Injured | `FUN_0062c400(staff)`, thiscall ECX = `[0xb63d28]`, `ret 4` | 2 = injured, 1 = recovering, 0 = fit. The first-team pick drops 2 only (0x6dc6f9). The reserve pick also drops 1 (0x6dcb88). |
| International duty | `FUN_007757d0(staff, &date, club, 0, 0)`, thiscall ECX = `[0xb74318]`, `ret 0x14` | The national club that holds the player, or 0. Strings "on international" (0xa62ff4) and "all star squad" (0xa62fa4). It is not a ban test. Use date = 0xae2c90. |
| Ineligible for a competition | `FUN_0055d270(staff, competition (fixture+0x14), 1, 0)`, thiscall ECX = `[0xae28f0]`, `ret 0x10` | 1 = cannot play. Warning: for a human club it clears the player from club +0x1e0 (0x55d270 pseudocode). Needs a competition. |
| Banned | `FUN_008cf5e0(staff, fixture)`, thiscall ECX = `[0xde1f54]`, `ret 8` | Non-zero = cannot play (9, or the result of a rule object call). Inference: suspensions per competition. Needs a fixture. |
| On loan | `FUN_004cf6d0(staff)`, ECX = `[0xae1050]`, then `FUN_004cf5a0` | 0x6dc7dd..0x6dc868 keeps a player whose loan record club is this club or `FUN_00540a50(club,0,1)`. Inference: drops players loaned out. |

Without a fixture, only injury, international duty and loan can be tested. For bans and eligibility, get the next fixture with 0x5acbe0.

### Formation of the active tactic

- `FUN_005fbd00(manager)`, thiscall, ECX = `[0xb63c98]`, `ret 4`. Returns the active tactic block (0x18e3 bytes). The name string is at +0 (compared with "Not set" and "3-5-2 Attacking" at 0x6dd750..0x6dd7b0).
- `FUN_005b2ae0(slot, which)`, thiscall on the block, `ret 8`, slot 0..10. which = 0: short at +0x12d + slot*2 (position mask). which = 1: short at +0x14e + slot*2 (second mask, meaning unknown).
- Mask bits: 0x1 GK, 0x2 sweeper, 0x4 D, 0x8 DM, 0x10 M, 0x20 AM, 0x40 F, 0x80 left, 0x200 centre, 0x800 right (`FUN_005b7d20`). 0x100, 0x400 and 0x1000 appear in the scorer. Their meaning is unknown.
- Other per-slot data passed to the pick: 8 bytes at +0x188b + slot*8 (`FUN_005b5270`), 1 byte at +0x5b4 + slot (`FUN_005b57c0`), and the tactic flags dword +0x584.

### Writing the selection

The store object is `[0xde1d08]` (ECX for all four). The record index comes from `FUN_008982b0(club, team)`. For team 0 it is the club index (club+0).

| Call | Arguments | `ret` | Effect |
|---|---|---|---|
| `FUN_00896fe0` | (club, team, save_undo, restore) | 0x10 | (club, 0, 0, 0) sets slots 0..19 to -1. save_undo = 1 first copies them to an undo slot of the current human. restore = 1 with save_undo = 1 copies them back. |
| `FUN_00896a90` | (club, slot 0..19, staff*, team) | 0x10 | Stores the staff index (`*staff`) at record + 2 + slot*4. |
| `FUN_00897760` | (club, slot, team) | 0xc | Captain = the player in that slot (record +0x52). |
| `FUN_008968f0` | (club, slot, team) | 0xc | Reads a slot. Clears a slot whose player has no player data. |

Slots 0..10 are the eleven. Slots 11 and up are substitutes. A match reads `(fixture+0x4d & 0xf) + 11` slots (0x6dbb80). The store holds 20, so at most 9 substitutes. The number of substitutes without a fixture is unknown from code. Inference: take it from the next fixture's +0x4d.

Team index: 0 = first team, 1 = reserves (inference from +0x19, see Q1).

### `FUN_006e2000`, the assignment core

```
int __cdecl FUN_006e2000(
  staff **players, char n_players,       // candidates, max 50 (local buffer of 50 at the sub recursion)
  short *pos_mask, char n_slots,         // masks of the eleven
  char n_subs,                           // masks of the subs follow in pos_mask
  staff **out,                           // n_slots + n_subs results, 0 = empty
  uint flags, club *club,
  side *side,                            // may be NULL
  staff *manager,                        // may be NULL: then club+0xcf is used
  int p11,                               // 0 in both calls of 0x6dbc50 except the second (EDI)
  uint8_t (*slot8)[8],                   // may be NULL: allocated and zeroed
  uint8_t *slot1,                        // may be NULL
  uint tactic_flags,                     // tactic +0x584
  short *pos_mask2);                     // may be NULL
```

Evidence: the pushes at 0x6dd326..0x6dd37e, `add esp, 0x3c` after the call (0x6dd387), and the NULL tests in the pseudocode of 0x6e2000. Returns 1 when done, 0 when no candidate.

How it works (code):
1. With more slots than players, it fills the best slot for each player. Else each slot takes the best player, then a loop of up to 10,000 passes removes players used twice.
2. A player in a non-GK slot with goalkeeper rating 20 is removed, and a player with goalkeeper rating below 10 in a GK slot is removed.
3. The subs are a recursive call with the unused players and flag 0x200 (0x6e2b50).
4. Up to 100 passes swap pairs of slots when the sum of scores improves.

Scores come from `FUN_006e2f20(player, player_data, match_record or 0, mask, flags, club, side, manager, -1, slot8, slot1, tactic_flags, mask2, holiday_record)`. With side = NULL it uses `manager` or club+0xcf (0x6e2f20 pseudocode lines 87..105). The match record is only used with flag 1 and a record.

Random numbers: 0x6e2000 and 0x6e2f20 have no call to 0x90cfc0. The only reach is 0x6e2f20 > `FUN_0062ed70` > `FUN_0062c680`, and 0x6e2f20 > `FUN_0062c590` > 0x62c400 > 0x62c680. That is the daily fitness update, which also runs when any screen reads condition or injury. Inference: calling the pick on a day after the squad screen showed condition draws nothing new. If no screen read a player that day, the update runs earlier than it would in the original, so the random sequence can shift.

Flags used by the original pick: base 0xca0ca for a club, 0x208a09c for a national team (0x6dcda5..0x6dcdab). Then: 0x100 when the club is much stronger than the opponent (with random draws), else 0x10000 for a club. 0x80000 is cleared on some branches. 0x800000 marks an important match. 0x400000 is for "use current team selection" on holiday. Inference: for a deterministic human first-team pick, pass 0xca0ca | 0x10000. Flag meanings that the scorer shows: 0x400 ignores the position rating, 0x800 skips the condition term, 0x200 marks the subs pass. Others are unknown.

Subs masks in the original: random (`FUN_0090cfc0(10)`: below 3 D, 3..5 M, above 5 F), and the second sub is GK when the match squad has 15 or more, else 1 time in 10 (0x6dd108..0x6dd1a0). A runtime picker should use a fixed list. Inference: GK, D, M, M, F.

Captain: `FUN_006e1940(players, count)`, thiscall on a side, `ret 8`, reads only side +0x2f (club) and +0x1f (manager non-player data, may be 0). It takes the best `FUN_006e1cf0(player, 0, club)` (cdecl). No random reach. A runtime can pass a zeroed 0xd0-byte buffer with +0x2f set, or call 0x6e1cf0 itself.

### Picker steps (inference)

1. Manager = club+0xcf, tactic = `FUN_005fbd00(manager)` with ECX = `[0xb63c98]`.
2. Masks for slots 0..10 from tactic +0x12d, second masks from +0x14e, slot data from +0x188b and +0x5b4, flags from +0x584.
3. Candidates from club+0xd7: player data present, `FUN_0062c400` != 2, `FUN_007757d0` == 0, loan test. Bans and eligibility only with a fixture from 0x5acbe0.
4. `FUN_006e2000(..., side = NULL, manager, ...)` for 11 + n subs.
5. `FUN_00896fe0(club, 0, 1, 0)` (with undo), then `FUN_00896a90` for each non-empty slot, then `FUN_00897760` for the captain.

## Unknown

- Meaning of the match object +0 byte and of the second tactic mask (+0x14e).
- Which fitness word is match fitness.
- Current ability offset (inference only).
- Number of substitutes without a fixture.
- The rules that `FUN_008cf880`, `FUN_008cf5e0` and `FUN_006e8910` apply.
- Indirect calls were not followed in the random check.
- The squad screen build 0x454900 was not traced.
