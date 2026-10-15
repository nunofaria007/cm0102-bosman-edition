# Brief for the 0.3.0 build sessions

Spec: `docs/specs/2026-10-15-features-0.3.0-spec.md`. Read it first, then `GLOSSARY.md` (use its words: settings file, option, data set, save backup, assistant selection, clean files) and `docs/HANDOVER.md`. For the REA tools and their traps, read `docs/research/agent-brief.md`. Build only your own slice.

## Shared facts
- Repo `/Users/max/Projects/cm0102/cm0102-mac`, GitHub `maxevans60/cm0102-bosman-edition`. Work in your own worktree on your own branch, cut from `main`. Commit there. Do not push. Do not merge into `main`: the lead session merges after review.
- The original: `/Users/max/Projects/cm0102/clean-3968/cm0102.exe` (3.9.68). Game files: `/Users/max/Projects/cm0102/clean-3968`. Never use the fan pack in `/Users/max/Downloads/Game`.
- Build: configure a `build/` in your worktree with `cmake -S . -B build -DCM0102_EXE=/Users/max/Projects/cm0102/clean-3968/cm0102.exe -DRECOMP_PYTHON=/Users/max/.venvs/cm0102/bin/python`, then `cmake --build build -j 16 --target cm0102_recomp` (about one minute). Game code changes go through the checked mechanisms (`RT_WRAP`, address lists in `tools/recomp/`): the build must stop if an address no longer matches.
- Scripted runs: `RECOMP_SCRIPT` steps with wait steps (`wait setup`, `wait box`, `wait first-news`, `wait match`, `wait screen:0xADDR`, `wait day:D.M.YY`; `src/recomp_rt/win/waits.c`). `RECOMP_SCREEN_LOG=1` prints each screen change with its build address. Never write to Max's save folder: use `RECOMP_USER_DIR=<new temp folder>`.
- Saved test games: `/Users/max/Projects/cm0102/test-fixtures/` (`prematch.sav`; `start.sav` after slice 1). `tests/fixtures/fixtures.py play` loads one. Start from a save instead of a new game whenever you can.
- Known screens: news 0x7881e0, squad 0x454900, "Proceed to the match?" 0x7260f0, match 0x718e90, bid negotiation 0x8e9760. The path from `prematch.sav` to the match is in the handover.

## Speed (Max, 2026-10-15: "move as fast as possible")
- Run test games in parallel. If a parallel run fails on timing, rerun it alone before you call it a bug.
- While you build, run only the test your change touches. Run the full set once at the end: `tests/recomp/test_load.py` (with `/Users/max/.venvs/cm0102/bin/python`), both smoke modes, `--same-save`, `ctest -R options` in `build/`, all at the same time.
- Smoke: `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968 /Users/max/Projects/cm0102/smoke-ref --mode classic` and `--mode widescreen`.
- Send long decompiles to a subagent: REA returns each listing twice.

## Rules
- No feature may change match results or the game's decisions, except the team that is picked. If a design would, stop and report.
- Every option is a line in the settings file, read through the existing reader, with the default from the spec. Add it to `ctest -R options`.
- Never commit the original's code or data, saves, screenshots or the logo.
- Do not run `rm` with a wildcard or on a path in a variable. Make new folders with `mktemp -d`.
- Plain STE English in docs, comments and commit messages. End commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Research: give every fact its address, mark inferences, write "unknown" rather than guess.
- Tick your slice's acceptance checks in the spec on your branch only when they pass.
- Final reply: at most 300 words: the branch name and last commit, the files you touched, what works, what does not, test results, and screenshot paths (about 800 px wide, only of what changed).
