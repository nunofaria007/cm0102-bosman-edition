# Bulk recompile
Status: In build

## Problem
The hand port, one tested function at a time, would take weeks to months to reach the game engine. Max chose on 2026-10-14 to recompile the whole original in bulk instead.

## Solution
A recompiler translates every x86 instruction of `cm0102.exe` into C that works on emulated registers, flags and the x87 stack, and on one flat block of 32-bit memory laid out as the original image. The generated C compiles natively on macOS. Replacements over SDL stand in for every function that the original imports from Windows, DirectDraw, DirectSound and the other system libraries. The game then runs as a native Mac program.

## Decisions
- Instruction-level translation, not Ghidra's C. The output is exact by construction and needs no hand fixing.
- The original's code bytes are read from the player's own `cm0102.exe` when the recompiler runs. The generated C is a build product: it is not committed, because it is a translation of the original. The recompiler script and the runtime are committed.
- Memory: one flat block, with the PE sections at their original addresses, the heap and stack inside it. Pointers stay 32-bit.
- Indirect calls and jumps go through a table from original address to generated function. Jump tables come from the analysis.
- Imports: each imported function gets a C replacement with the same calling convention. A missing replacement stops the program with the name of the import.
- The hand-ported, tested C of the earlier specs may later replace generated functions, where clean code helps.
- The differential tracer of `tests/diff` checks the recompiled program against the Unicorn run at fixed points: after the database load, after game creation, after each game day.
- Commits are local. Nothing is pushed.

## Slices
### 1. Sizing run
A recompiler prototype translates the database load and its callees, links them with a minimal runtime and compares every table with the Unicorn run of the original. It reports the speed of translation and the share of instructions it handles.
- [ ] The recompiled load gives the same tables as the original.

### 2. Import inventory
Every import of the original, grouped by library, with its calls, its arguments as the original uses them, and what an SDL replacement must do.
- [ ] Every import is listed with its replacement plan.

### 3. Whole program
All code recompiled, the runtime with all imports, the game starting on SDL.
- [ ] The recompiled game shows the start screen.
- [ ] Manual check: a new game starts and a day passes.
