# Fix brief, 0.2.0 review (2026-10-15)

One Sonnet fixer works from this brief. Worktree: `/Users/max/Downloads/Game/cm0102-mac/.claude/worktrees/agent-a069b80021b6499a7`.

## Rules

- Edit source only. Do not build and do not run the game: a timed test game uses this worktree's binary now.
- Keep the style of the file around each change. Comments in plain STE English, British spelling.
- Commit once at the end, message "Fix the 0.2.0 review findings", ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Do not run `rm` with a wildcard or a variable path.
- Final reply: at most 300 words, the files touched and what you did for each item.

## Items, each with its decision

1. **Fix.** `src/recomp_rt/win/dsound.c` about line 396: the refused-format path of `ds_CreateSoundBuffer` calls `rt_ret_stdcall(DSERR_BADFORMAT, 3)`. The success path pops 4 (this, desc, out, outer). Make every return path of that function pop 4.
2. **Fix.** `src/recomp_rt/win/user32.c` about line 372: while a key click runs, real mouse button events are dropped before the `buttons` state changes. Still drop the event, but update `buttons` first, so later mouse messages carry the right button bits.
3. **Fix.** `src/recomp_rt/win/display.c` about line 79 and `columns.c` `columns_area_after`: when 0x55fba0 unwinds, the scratch bytes are not put back and `cols.pending` stays 2. Make the wrap always call a restore (put the scratch back, clear pending). Set `cols.area` only when the call did not unwind.
4. **Fix.** `tests/smoke/smoke_run.py` `SAME_SAVE_RUNS`: add a run "off" with every switch option off (space_to_continue, shortcuts, sharp_picture, sound, clickable_names, attribute_colours, faster_days, match_columns). Update the docstring list of runs and the time ("about eight minutes" grows by about two).
5. **Fix.** `site/index.html` about line 285: "Your assistant can pick the team" is listed for 0.2.0. Move it to the 0.3.0 list, or remove it if the site has no 0.3.0 list. Then add a short list of the 0.2.0 settings.txt options with their defaults, from `src/recomp_rt/win/options.c`, in the style of the page. Say what each option does in plain words.
6. **Fix, comment only.** `src/recomp_rt/win/columns.c` line 54: say that MATCH 0xb67a64 is known only from a run (inference), with no static evidence yet. `holiday.c` lines 23 to 24 and `dsound.c` about line 461: if the file or a doc in `docs/research/` gives the evidence, cite it. If not, write "evidence: unknown, seen in a run" or "inference".
7. **Leave.** The hand cursor with `clickable_names = off`, a condition of 0 in the tactics columns, `holiday.c` writing its argument slot, the state left in `ratings` and `names.c`. Do not change them.
