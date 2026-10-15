# Contract clauses in memory

Found on 2026-10-15 while removing a release clause from a player in a save (static reads plus a run).

- Contract of a staff member with id `id`: `ci = *(int32 *)([0xae2d0c] + id * 79)`, contract = `[[0xae1050]] + ci * 80` (`FUN_004cf5a0`).
- Clause flags: one byte per clause type at contract +0x1c..+0x20. +0x1d is the minimum fee release clause (readers 0x85f7a0, 0x868020, 0x861b7b).
- The clause fee: dword at +0x21 (the screen rounds it, 210120 shows as £210,000). It is 0 in contracts without the clause.
- +0x10 and +0x14 are the goal and assist bonuses.
- To remove the clause: +0x1d = 0 and +0x21 = 0, then save with the game's own save `FUN_0082e430(path, 0x16)` (src/recomp_rt/win/testrun.c). The save's parts are compressed, so edit through the game, not the file.
- A load and save changes many parts of the file by itself. Compare an edited save with a control save that was loaded and saved without the edit.
