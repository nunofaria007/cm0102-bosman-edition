# Handover, 2026-10-14

Read this first in the next session. Then read `GLOSSARY.md` and `docs/specs/2026-10-14-recompile-spec.md`.

## Goals of the next session

Max wants four things:

1. A GitHub Pages website where other people can download the game easily.
2. A Windows version: one executable that starts on a double-click and plays, with no tools to install.
3. A proper Mac app: a `.app` that starts from Finder and the Dock.
4. A push of the repo to GitHub.

The aim of 1 to 3 is the same: download, double-click, play.

## State of the repo

- Repo: `/Users/max/Downloads/Game/cm0102-mac`, GitHub `maxevans60/cm0102-mac`. Branch `main`.
- 60 local commits are not pushed. The last push was `f132a53` on 2026-10-13.
- The working tree is clean. Agent worktrees sit in `.claude/worktrees/`, which git ignores.

### The recompiled game (the main path)

- `tools/recomp/` (Python, capstone) translates every x86 instruction of `cm0102.exe` into C on a flat 32-bit memory.
- `src/recomp_rt/` is the runtime. `src/recomp_rt/win/` holds the replacements for the Windows, DirectDraw, DirectSound, network and registry imports, over SDL2.
- CMake target `cm0102_recomp`. The build runs the recompiler on the player's own `cm0102.exe` (CMake variable `CM0102_EXE`, by default the first `cm0102.exe` in a folder above the repo). The generated C lives in `build/recomp` and stays out of git.
- Build: `cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew && cmake --build build -j --target cm0102_recomp` (about 40 s with `-j`).
- Run: `build/cm0102_recomp`. With no options it looks upwards from itself for the folder with `cm0102.exe` and `Data/`. `--game <folder>` names it.
- All writes go to `~/Library/Application Support/cm0102`. Reads look there first, then in the game folder. The game folder stays untouched.
- Max played it on 2026-10-14: setup, a new game and days pass. Sound is silent (DirectSound is a stub). Network and printing return failure.
- Unattended runs: `SDL_VIDEODRIVER=dummy`, `RECOMP_NO_BOX=1`, and `RECOMP_SCRIPT` with steps. The script has a `dump` step that saves the screen. Read `src/recomp_rt/win/main.c` for the step syntax.

### The hand port (beside it)

- Clean C for the screen manager, screen stack, input, text entry, the start screen, Select League(s), dates, random numbers and the full database load. Targets `cm0102` and `port_driver`.
- The differential test checks it against the original in Unicorn: `~/.venvs/cm0102/bin/python tests/diff/run.py /Users/max/Downloads/Game/cm0102.exe build/port_driver`. 91 suites, all "ok". `tools/breaks.py` with the lists in `tests/diff/breaks/` checks the tests.
- It is not on the path to the four goals. Leave it as it is.

## What each goal needs

### 1. Mac app

- A `.app` bundle for `cm0102_recomp` with an icon, `Info.plist` and SDL2 inside the bundle (`Contents/Frameworks`, with `install_name_tool` or `dylibbundler`), so that it runs on a Mac without Homebrew.
- A universal binary (arm64 and x86_64) if older Macs should run it. The generated C is plain C, so this needs only a second build.
- The game data must be found: inside the bundle, or next to it, or chosen by the player at first start. This depends on decision D1 below.
- Signing: ad hoc signing works on Max's Mac. Other people get a Gatekeeper warning unless the app is signed with a Developer ID and notarised, which needs a paid Apple developer account (decision D3).
- The runtime already writes to `~/Library/Application Support/cm0102`. Keep that.

### 2. Windows executable

- Build `cm0102_recomp` for Windows from the same generated C and runtime. Two ways: cross-compile on the Mac with mingw-w64 or `zig cc`, or build on a Windows runner of GitHub Actions.
- Link SDL2 statically, or put `SDL2.dll` next to the exe, so that a double-click works.
- The runtime has POSIX parts: the case-insensitive file mapping, the user folder (`HOME`, `Library/Application Support`), file search. On Windows these need Windows forms: `%APPDATA%\cm0102` for writes, and the native file calls.
- The flat 32-bit memory works in a 64-bit Windows process, as it does on the Mac.
- The result needs no DirectDraw or old Windows settings, which is why it is easier than the original on modern Windows.
- Windows SmartScreen warns about an unsigned exe. Signing needs a code-signing certificate (decision D3).

### 3. GitHub Pages website

- A small site with a download button for each platform, short install steps, screenshots and a note on the legal status.
- `docs/` already holds the specs and research. Put the site in its own folder (for example `site/`) and publish it with a GitHub Actions workflow, or use a `gh-pages` branch.
- The downloads themselves belong in GitHub Releases, not in the site or the repo.

### 4. Push

- Before the push, check that git holds nothing of the original: no code, no data, no names. `src/cm3/key_ids_table.c` holds only addresses. `tools/recomp/functions.txt` holds only function addresses. The generated C and all `build/` folders are ignored.
- Max must confirm the push in the session.

## Decisions for Max at the start of the next session

Run the grilling skill on these before any build.

- **D1. What the download holds.** The recompiled program is a translation of the original's code, and the game also needs the original's data (`Data/`, `Pictures/`). The repo so far follows Max's rule: never commit the original or its data. A ready-to-play download breaks that rule unless the license allows it. Eidos made CM 01/02 a free download in December 2008. I don't know the exact terms of that release, and they must be checked before anything is published. The options are:
  - A: the download holds the program and the data (ready to play, needs the license to allow it).
  - B: the download holds the program only, and the player points it at a copy of the free release (one extra step).
  - C: the download holds the recompiler and runtime only, and builds on the player's machine (not double-click).
- **D2. Where the builds happen.** On Max's Mac, with the releases uploaded by hand, or in GitHub Actions. Actions needs `cm0102.exe` at build time, so it ties to D1.
- **D3. Signing.** Unsigned with warnings and install notes, or a paid Apple Developer ID and a Windows code-signing certificate.
- **D4. Name and branding.** The site and app must not look like an official Sports Interactive or Eidos product. A plain name such as "CM 01/02 for Mac and Windows (unofficial)".

## Rules that still apply

- Max's CLAUDE.md: interview (grilling) before a multi-step build, a spec in `docs/specs/`, plain STE English in replies.
- The memory notes on speed and on approvals: parallel agents in worktrees with one brief, one review pair per spec, the break tool where tests exist, no approval asks for routine steps, ask only for interview decisions, manual checks and pushes.
- Agents must not run `rm` with a wildcard: a safety check stops it even in bypass mode.
- Never copy the original's code or data into the repo.
