# Build brief: Select League(s)

## Goal
Build the slice of `docs/specs/2026-10-14-select-leagues-spec.md` that your task names. The facts are in `docs/research/2026-10-14-start-new-game-inventory.md`, `docs/research/2026-10-13-database-inventory.md` and `docs/research/2026-10-13-input-inventory.md`. Read the three shipped specs in `docs/specs/` for the house rules, and `GLOSSARY.md` for the terms.

## What exists
- Screen manager `include/scrman.h`, widgets `include/guio.h`, areas `include/area.h`, screen stack `include/screen_stack.h`, input `include/scrman_input.h`, helpers `include/gui_menu.h`, `include/langlib.h`, `include/utils.h`, the start screen `src/cm3/setup.c`.
- Differential test in `tests/diff/` (`run.py`, `original.py`, `scene.py` with `ManagerScene`, `port_scene.c`, `port_stack.c`, `port_start.c`). Use `tests/diff/cases_input.py` and `tests/diff/cases_start.py` as models.
- Break tool `tools/breaks.py` with lists in `tests/diff/breaks/`.

## Rules
- Test first: write the suite, see it fail, then port. Each ported function gets a differential case against the original.
- After the slice, add a break list in `tests/diff/breaks/` and run `tools/breaks.py`. Close every gap that a real screen reaches. Name the equivalent breaks.
- Read the assembly before you trust a signature. Ghidra drops the thiscall ECX and garbles large stack frames.
- Port what "Select League(s)" reaches. Record what you leave out, with the address, in the Decisions of the spec.
- Engine code from `si\code\` goes in `src/si/`, game code from `cm3\code\` in `src/cm3/`. Use the assert strings to tell which. Add every new file to both targets in `CMakeLists.txt`.
- Match the code and comment style of `src/si/scrman.c` and `src/cm3/setup.c`: each ported function names its FUN_ address. Use the glossary terms.
- Run the suites you touch while you build, and the full suite (`~/.venvs/cm0102/bin/python tests/diff/run.py /Users/max/Projects/cm0102/clean-3968/cm0102.exe build/port_driver`) once at the end. Every line must start with "ok".
- Commit in your worktree with messages in the style of `git log`, ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Do not push.
- Tick the checks of your slice in the spec when they pass.
- Never copy the original's executable or data files into the repo. A small fixed table that the code needs may go into the source, as the glyph tables did.
- If a fact is missing and no tool finds it, write "unknown" in the spec and leave that part out. Do not guess.
- Make files only for your slice. Another agent builds another slice at the same time. Keep shared files (`CMakeLists.txt`, `tests/diff/run.py`, `tests/diff/port_driver.c`, `tests/diff/original.py`) to small added lines, so that the merge is easy.
- make compares file times to the second: delete the object file after a quick source change.

## Report
At most 300 words: what works, the break results, what you left out, open doubts, the commits and the files you touched.
- Do not run `rm` with a wildcard or on a path in a variable. A safety check stops it and asks Max, even in bypass mode. To start again with an empty work folder, make a new folder (`mktemp -d` in your scratch area) and use that.
