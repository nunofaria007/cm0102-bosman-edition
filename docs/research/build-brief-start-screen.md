# Build brief: start screen, slices 2 and 3

## Goal
Build slices 2 and 3 of `docs/specs/2026-10-14-start-screen-spec.md`: the helpers and the "Setup Game" build, plus the "Please Confirm" screen that "Exit Game" opens. The facts are in `docs/research/2026-10-14-start-screen-helpers.md` and section 6 of `docs/research/2026-10-13-screen-stack-inventory.md`. Read both specs in `docs/specs/` from 2026-10-13 for the house rules, and `GLOSSARY.md` for the terms.

## What exists
- Screen manager: `include/scrman.h` (create, reset, draw), widgets `include/guio.h`, areas `include/area.h`.
- Screen stack: `include/screen_stack.h` (push, slots, title, main loop `screen_stack_run`).
- Input: `include/scrman_input.h` (hit test and poll, mode 0).
- Differential test: `tests/diff/` (`run.py`, `original.py`, `scene.py` with `ManagerScene`, `port_scene.c`, `port_stack.c`). Read `tests/diff/cases_screen.py` and `tests/diff/cases_input.py` as the model for a new suite.
- Break tool: `tools/breaks.py` with lists in `tests/diff/breaks/`.

## What to build
1. The text function `FUN_0066f4e0` for the plain path, with the `<...>` rule the side bar needs. Leave out the `{...}` and `[...]` tags. The test sets 0x9eb2dc to -1 in the original.
2. The heading, sub-heading and button helpers, the menu builder with its item type, and the Back and Next bar.
3. The background picker: read `Pictures/pics.cfg` as the original reads it (the "England" list). Random choice in the port. The test gives both programs the same title, so the pick itself is not compared.
4. The side bar on the start screen (mode 1, no manager record), with "Version\n3.9.68".
5. The build function of "Setup Game" and its input function for a plain start, the "Please Confirm" screen with its build and input, and the input functions for the side bar and Back and Next as far as the start screen and the confirm screen reach them. Menu items whose screens the port lacks return -4 (rebuild). "Yes" on the confirm screen returns `SCREEN_QUIT` in the port. The original calls `exit(0)` there. Record that difference in the spec.
6. Put screen code that comes from the game part of the original (`cm3\code\`) in `src/cm3/`, and engine helpers (`si\code\`) in `src/si/`. Use the assert strings to tell which. Add every new file to both targets in `CMakeLists.txt`.

## Rules
- Test first: write the suite, then the port. Each function gets a differential case: the same build on both sides, then every field of the screen manager, its areas and widgets, and the screen after the screen draw. Set `DAT_009cc2c4` to -1 in the original (the test already does). Widget +0x48 of a menu item is stack garbage in the original: leave it out of that compare only, and say so in a comment.
- After each slice, add a break list in `tests/diff/breaks/` and run `tools/breaks.py`. Close every gap that a real screen can reach. Say which breaks are equivalent.
- Read the assembly before you trust a signature. Ghidra drops the thiscall ECX.
- Port what the start screen and the confirm screen reach. Record what you leave out in the Decisions of the spec, with the address.
- Match the code and comment style of `src/si/area.c` and `src/si/scrman.c`: each ported function names its FUN_ address. Use the glossary terms.
- Run the suites you touch while you build, and the full suite (`~/.venvs/cm0102/bin/python tests/diff/run.py /Users/max/Downloads/Game/cm0102.exe build/port_driver`) once at the end. All lines must start with "ok".
- Commit each slice in your worktree with a message in the style of `git log`, ending with the line `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Do not push. Do not touch `src/main.c`, `src/si/input.c` or `src/si/display.c`: another agent changes them.
- Tick the slice checks in the spec when they pass.
- Never copy the original or its data into the repo.
- If a fact is missing and no tool can find it, write "unknown" in the spec and leave that part out. Do not guess.

## Report
At most 300 words: what works, the break results, what you left out, open doubts, the commits, and the files you touched.
