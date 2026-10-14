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
- The CRT is recompiled with the game, so `sprintf`, `qsort` and `rand` behave exactly as in the original. About 35 KERNEL32 imports carry it (import inventory, section 5).
- The runtime reports Windows NT 5.1, so the CRT heap uses only `HeapAlloc`, `HeapFree`, `HeapReAlloc` and `HeapSize`.
- The CD check: the runtime reports a CD drive `D:\` that holds the player's own `cm0102.exe`. None of the 35 copies of the check is patched.
- Network play: the WSOCK32 imports return the errors that Windows gives with no network. The original then takes its own failure path. The start screen keeps "Network Play".
- Printing (GDI32, WINSPOOL, comdlg32) returns failure for now.
- The game folder: `--game <folder>` names the folder of the original. It is the registry value Location and the base of every path. Default: the first folder above the program with `cm0102.exe` and `Data`, else the current folder. A folder without `cm0102.exe` stops the program with a message that names `--game`.
- Writes go to the user folder `~/Library/Application Support/cm0102`, never to the game folder. Any file that the game opens for writing, creates, renames or deletes, and any folder it creates, lives there under the same relative path. A read looks in the user folder first, then in the game folder. A file search lists both, the user folder winning on equal names. Delete and rename act only in the user folder: a delete of a file that exists only in the game folder fails, and a rename of such a file copies it to the new name in the user folder. A file of the game folder that the game opens for update is copied to the user folder first. A path outside the game folder maps to `_outside/<path>` in the user folder. (src/recomp_rt/win/files.c)
- Commits are local. Nothing is pushed.

## Slices
### 1. Sizing run
A recompiler prototype translates the database load and its callees, links them with a minimal runtime and compares every table with the Unicorn run of the original. It reports the speed of translation and the share of instructions it handles.
- [x] The recompiled load gives the same tables as the original.

### 2. Import inventory
Every import of the original, grouped by library, with its calls, its arguments as the original uses them, and what an SDL replacement must do.
- [ ] Every import is listed with its replacement plan.

### 3. Whole program
All code recompiled, the runtime with all imports, the game starting on SDL.
- [x] The recompiled game shows the start screen.
- Build: `cmake --build build --target cm0102_recomp`, then `build/cm0102_recomp` (or `--game <folder>`). A run without a person: `SDL_VIDEODRIVER=dummy RECOMP_SCRIPT="8000 click 277 177; 20000 dump s.bmp; 21000 quit"` (src/recomp_rt/win/user32.c).
- 2026-10-14, scripted run: Start New Game, Select League(s), the CD check, Enter Name, Select Nationality, Select Team (Arsenal), Take Control, Finish. The game reached Monday 16.7.01 with news. A person has not played it yet.
- [ ] Manual check: a new game starts and a day passes.
