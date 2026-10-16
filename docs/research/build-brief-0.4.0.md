# Brief for the 0.4.0 build sessions

Spec: `docs/specs/2026-10-15-features-0.4.0-spec.md`. Read it first, then `GLOSSARY.md` (use its words: Team views screen, best XI, squad depth, polygon analyser, attribute groups, hidden value, printout, HD mode, assistant selection) and `docs/HANDOVER.md`. For the REA tools and their traps, read `docs/research/agent-brief.md`. Build only your own slice.

## Shared facts
- Repo `/Users/max/Projects/cm0102/cm0102-mac`, GitHub `maxevans60/cm0102-bosman-edition`. Work in your own worktree on your own branch, cut from `main`. Commit there. Do not push. Do not merge into `main`: the lead session merges after review.
- The original: `/Users/max/Projects/cm0102/clean-3968/cm0102.exe` (3.9.68). Game files: `/Users/max/Projects/cm0102/clean-3968`. Never use the fan pack in `/Users/max/Downloads/Game`.
- Build: configure a `build/` in your worktree with `cmake -S . -B build -DCM0102_EXE=/Users/max/Projects/cm0102/clean-3968/cm0102.exe -DRECOMP_PYTHON=/Users/max/.venvs/cm0102/bin/python`, then `cmake --build build -j 16 --target cm0102_recomp`. Game code changes go through the checked mechanisms (`RT_WRAP`, `tools/recomp/callbacks.py`, `tools/recomp/callsites.py`): the build must stop if an address no longer matches.
- Scripted runs: `RECOMP_SCRIPT` steps with wait steps (`src/recomp_rt/win/waits.c`). `RECOMP_SCREEN_LOG=1` prints each screen change with its build address. Never write to Max's save folder: use `RECOMP_USER_DIR=<new temp folder>`.
- Saved test games in `/Users/max/Projects/cm0102/test-fixtures/`: `start.sav` (Arsenal 14.7.01), `prematch.sav` (Basel friendly 20.7.01) and `yeovil.sav` (a copy of Max's Yeovil save, 20.8.02; never touch his own save). `tests/fixtures/fixtures.py play` loads one. Start from a save, not a new game.
- The picker (`src/recomp_rt/win/picker.c`) has `picker_compute`, which computes a pick without writing it. Best XI and squad depth use it. `tests/smoke/picker_check.py` is its test.
- Research facts for each slice are in the spec under Decisions. Facts with addresses: the original's comparison screen (side bar item id 0x434, handler 0x7633af, push 0x870650, prompts in 0x8f41b0), the profile builder 0x87a510 (qsort 0x87a786, 36 rows at 0xddf570, counts 0x24 at 0x87a81e and 0x87acb5), the printout writers in 0x7d4460..0x7d9xxx, the Printer path 0x84c830, the print stubs in `src/recomp_rt/win/net.c`, widescreen in `src/recomp_rt/win/display.c` and `tools/recomp/widescreen.py`.

## Slices in this batch
- Slice 3, Team views screen: first test a new screen on the screen stack (push 0x7fe600, `rt_host_fn` in rt.c). If it fails, report before you build the fallback.
- Slice 7, attribute groups.
- Slice 8, print to PDF. Write the PDF with your own small writer (fixed-width text pages) or a library that already ships; ask the lead before you add a dependency.
- Slice 9, HD mode part 1. Widescreen and classic screen dumps must stay identical.
Slices 3, 7 and 9 may touch the same draw files. Keep each change small and in its own function, so the merges stay simple.

## Speed (Max: "move as fast as possible")
- Run test games in parallel. If a parallel run fails on timing, rerun it alone before you call it a bug.
- While you build, run only the test your change touches. Run the full set once at the end: `tests/recomp/test_load.py` (with `/Users/max/.venvs/cm0102/bin/python`), smoke in classic and widescreen (and HD once slice 9 is in), `--same-save`, `--match`, `ctest -R options`, `tests/smoke/picker_check.py`, all at the same time.
- Smoke: `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968 /Users/max/Projects/cm0102/smoke-ref --mode classic` and `--mode widescreen`.
- Send long decompiles to a subagent: REA returns each listing twice.

## Rules
- No feature may change the game's decisions while the player makes the same clicks, except the team that is picked. A change of memory layout counts like opening a screen: the original's results already depend on where its memory blocks sit (Q152). The same-save check proves this with the same steps on and off. If a design would change decisions under the same steps, stop and report.
- No hidden value (current ability, potential ability, or any value the original does not show) may appear on a screen or feed a shown number.
- Every option is a line in the settings file, read through the existing reader, with the default from the spec. Add it to `ctest -R options`.
- Never commit the original's code or data, saves, screenshots or the logo.
- Do not run `rm` with a wildcard or on a path in a variable. Make new folders with `mktemp -d`.
- Test first: write the failing check at the spec's seam, then the code.
- Plain STE English in docs, comments and commit messages. End commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Research: give every fact its address, mark inferences, write "unknown" rather than guess.
- Tick your slice's acceptance checks in the spec on your branch only when they pass. Leave Max's manual checks open.
- Final reply: at most 300 words: the branch name and last commit, the files you touched, what works, what does not, test results, and screenshot paths (about 800 px wide, only of what changed).
