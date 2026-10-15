# Brief for the release build agents

Spec: `docs/specs/2026-10-14-re-signed-edition-release-spec.md`. Read it first, then `GLOSSARY.md` and `docs/HANDOVER.md` ("The recompiled game"). Use the glossary's words.

## Shared facts
- Repo: `/Users/max/Projects/cm0102/cm0102-mac`. You work in your own git worktree on your own branch. Commit there. Do not push. Do not merge.
- Build: `cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew -DCM0102_EXE=<exe> && cmake --build build -j --target cm0102_recomp`.
- Unattended run: `SDL_VIDEODRIVER=dummy RECOMP_NO_BOX=1 RECOMP_SCRIPT=<steps> RECOMP_DUMP=<png>`. Step syntax: `src/recomp_rt/win/user32.c` and `main.c`.
- Free release: `/Users/max/Downloads/Archive/CM0102.iso`. Official patch: `/Users/max/Downloads/Archive/cm0102_3968/cm0102_3968.exe`. Treat both as read only.
- Clean files output folder (slice 2 makes it): `/Users/max/Projects/cm0102/clean-3968/`. Until it exists, use the fan pack `/Users/max/Downloads/Game` for local runs only, never in a download.
- Tools installed by Homebrew: `x86_64-w64-mingw32-gcc`, `makensis`, `unshield`, `dylibbundler`. CrossOver is in `/Applications/CrossOver.app`.

## Decisions per slice
- Slice 2, clean files: the tool lives in `tools/clean_files/`. It checks both input hashes, extracts the free release, applies the official patch, and writes the clean files and a manifest (path, size, SHA-256). If `update.dat` has an unknown format, find it out from the patch's `setup.exe` with the REA tools, or run `setup.exe` once under CrossOver in a scratch bottle as a fallback, then record which way worked. The manifest goes in the repo; the files do not.
- Slice 3+4, widescreen: research first, written to `docs/research/2026-10-14-widescreen-inventory.md`, then the build. 1280x800 default, 800x600 classic mode from one line in a settings file in the save folder. The wide background comes from the official 800x600 background at run time. No fan-made files, and no bytes copied from the fan loader. You may read `/Users/max/Downloads/Game/CM0102LoaderCustom.ini` for hints only.
- Slice 5, Mac app: CMake target or script that makes `CM 01-02 Bosman Edition.app`, universal (arm64 and x86_64 via `lipo`), SDL2 in `Contents/Frameworks`, the clean files in `Contents/Resources/game`, ad hoc signature, and a `.dmg` with an Applications link. Add `project(... VERSION 0.1.0)` and feed it to `Info.plist`, the window title and the file names. The runtime looks for the game in the bundle first.
- Slice 6, Windows: replace the POSIX parts of the runtime behind small host functions, so the Mac build keeps its behaviour. Cross-compile with mingw-w64, SDL2 static. Saves in `%APPDATA%\cm0102`. An NSIS script makes `CM0102-Bosman-Edition-<version>-Setup.exe` with Start menu and desktop shortcuts and an uninstaller. Test it under CrossOver.

## Rules
- Never commit the original's code or data, the logo or the clean files. Check `git status` before each commit.
- Do not run `rm` with a wildcard or on a path in a variable. Make a new folder with `mktemp -d` instead.
- Run the tests your change touches. `tests/recomp/test_load.py` must still pass if you touch the runtime.
- Write plain STE English in docs and commit messages. End commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Stop and report if a spec decision proves impossible. Do not choose a new design on your own.
- Final reply: at most 300 words, the branch name, the files you touched, what works, what does not, and any screenshot path (about 800 px wide).
