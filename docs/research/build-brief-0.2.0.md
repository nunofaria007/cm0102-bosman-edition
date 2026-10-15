# Brief for the 0.2.0 build agents

Spec: `docs/specs/2026-10-14-quality-of-life-0.2.0-spec.md`. Read it first, then `GLOSSARY.md` (use its words: settings file, option, assistant selection, clickable name, attribute colour) and `docs/HANDOVER.md`. For the REA tools and their traps, read `docs/research/agent-brief.md`.

## Shared facts
- Repo `/Users/max/Projects/cm0102/cm0102-mac`, GitHub `maxevans60/cm0102-bosman-edition`. Work in your own worktree on your own branch. Fast-forward it to `main` first. Commit there. Do not push or merge.
- The original: `/Users/max/Projects/cm0102/clean-3968/cm0102.exe` (3.9.68). Game files: `/Users/max/Projects/cm0102/clean-3968`. Never use the fan pack in `/Users/max/Downloads/Game` for tests.
- Build: `cmake --build build -j --target cm0102_recomp`. Runtime: `src/recomp_rt/`. Game code changes go through the recompiler's checked mechanisms (`RT_WRAP`, `tools/recomp/widescreen.py` style address lists): the build must stop if an address no longer matches.
- Unattended runs: `SDL_VIDEODRIVER=dummy RECOMP_NO_BOX=1 RECOMP_USER_DIR=<new temp folder> RECOMP_SCRIPT=<steps>`; `tests/smoke/smoke_run.py` shows how to reach a new game. Never write to Max's save folder.
- Test speed (Max, 2026-10-15): while you build, run only the test that your change touches, once. Run the full set (test_load.py, both smoke modes, --same-save) once, at the end of the slice. Run test games one at a time, never in parallel. When `tests/fixtures/` exists, start from its saved games instead of a new game.
- Tests: `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968 /Users/max/Projects/cm0102/smoke-ref --mode classic` and `--mode widescreen`; both must pass at the end of every slice.

## Rules
- No feature may change match results or the game's decisions (spec, Decisions). If a design would, stop and report.
- Every option is a line in the settings file, read through the reader from slice 1. Defaults as in the spec.
- Never commit the original's code or data, screenshots or the logo.
- Do not run `rm` with a wildcard or on a path in a variable. Make a new folder with `mktemp -d` instead.
- Plain STE English in docs, comments and commit messages. End commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Research agents: write only the one file your task names. Give every fact its address. Mark inferences. Write "unknown" rather than guess.
- Final reply: at most 300 words, the branch name, the files you touched, what works, what does not, and screenshot paths (about 800 px wide).
