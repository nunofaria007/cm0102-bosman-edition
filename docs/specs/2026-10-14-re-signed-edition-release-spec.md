# Re-Signed Edition 0.1.0 release
Status: In build

## Problem
The recompiled game runs only on Max's Mac, from a build folder, with the fan pack beside it. Other people cannot download it and play. It draws only at 800x600.

## Solution
A player opens the site, clicks the button for Mac or Windows, installs, double-clicks and plays. The download holds the recompiled game and the clean files. The game draws in widescreen mode by default.

## User stories
1. As a Mac player, I want one `.dmg` with an app that I drag to Applications, so that I play without tools.
2. As a Mac player on an Intel Mac, I want the same app to run, so that I need no second download.
3. As a Windows player, I want an installer that puts the game in the Start menu, so that I play without tools.
4. As a Windows player, I want the installer to remove the game cleanly, so that nothing stays behind.
5. As a player, I want the game in widescreen mode on my wide screen, so that the screen has no large black bars.
6. As a player who prefers the old layout, I want to switch to classic mode with one line in a settings file.
7. As a player, I want my saved games to survive an update to a new release.
8. As a player who sees a Gatekeeper or SmartScreen warning, I want the site to tell me the steps to open the game.
9. As a visitor, I want to see at once that the game runs on Mac and Windows.
10. As Max, I want one version number to show in the app, the installer, the file names and the window title.
11. As Max, I want each release tagged, listed in `CHANGELOG.md` and tracked by issues in a milestone.
12. As Max, I want the build to use only the clean files, so that no fan content ships.

## Decisions
Settled in the grilling interview of 2026-10-14 (Q1 to Q45).

**Product**
- Name: "CM 01/02: Re-Signed Edition". The description says "an unofficial port of Championship Manager 01/02". The full name of the original appears only in descriptive text.
- Logo: the classic CM 01/02 logo with the Apple and Windows marks from Simple Icons underneath. The logo files stay out of git. They come from the clean files at build time, or sit in the release assets.
- Repo: `maxevans60/cm0102-re-signed-edition` (renamed from `cm0102-mac`). Site: `maxevans60.github.io/cm0102-re-signed-edition`.

**Clean files**
- Free release: `CM0102.iso`, 303,628,288 bytes, SHA-256 `853134bbfc9e774424e62f9694ad7fe9dd7361c8bab93033f82fc906b31a5c53`. It holds version 3.9.60. The data sits in InstallShield cabinets.
- Official patch: `cm0102_3968.exe` from `www3.sigames.com/download/official/CM 01-02/`, Wayback capture 20030430004639, 23,458,816 bytes, SHA-256 `b3f2b7d2ace6e86d0974e5ad68ae7a87a1e8fc3494c90d00cc3758392dcf24f1`. It is a WinZip self-extractor. Its `setup.exe` applies `update.dat` (307 MB, format unknown at the time of writing).
- Max's copies: `/Users/max/Downloads/Archive/CM0102.iso` and `/Users/max/Downloads/Archive/cm0102_3968/cm0102_3968.exe`.
- The clean files are the free release with the official patch applied, and nothing else. The fan pack in `/Users/max/Downloads/Game` is never a build input.
- A manifest of the clean files (path, size, SHA-256) lives in the repo. It holds no content of the files.
- The recompiler runs on the clean 3.9.68 exe. If it differs from the fan-pack exe, the function list is checked again on it.

**Display**
- Widescreen mode: 1280x800, the default. On a 16:9 screen the window shows thin bars at the sides.
- Classic mode: 800x600. The switch is one line in a settings file in the save folder.
- The wide background is made at run time from the official 800x600 background. No fan-made background files.
- The window can be resized. It scales the picture to fit and keeps the aspect ratio, with bars at the sides or top. The screens do not lay out again for a new window shape (Max, 2026-10-14).

**Downloads**
- Mac: one `.dmg` with a universal app (arm64 and x86_64). SDL2 inside the bundle. The clean files inside the bundle. Ad hoc signature. Saves in `~/Library/Application Support/cm0102`.
- Windows: one 64-bit installer made with NSIS on the Mac. It installs the exe and the clean files, adds Start menu and desktop shortcuts and an uninstaller. SDL2 is built into the exe (mingw-w64 cross-compile). Saves in `%APPDATA%\cm0102`. No 32-bit build.
- Both unsigned. The site gives the steps for "Open Anyway" and "More info, Run anyway".

**Versions and tracking**
- Semantic versioning. The first release is 0.1.0, a GitHub pre-release. Max decides when 1.0.0 comes.
- Patch for fixes, minor for features, major for changes that break saves or settings.
- One version number in the CMake project feeds `Info.plist`, the exe properties, the installer, the window title and the file names.
- Each release: tag `vX.Y.Z`, a GitHub Release with both downloads, an entry in `CHANGELOG.md`.
- GitHub milestone 0.1.0 with one issue per goal and one for widescreen.
- Builds and uploads run on Max's Mac by one script. No GitHub Actions build, because it would need the clean files.

**Site**
- Content and order follow the OpenSGolf page (`crspytopgn.github.io/opensgolf`). The look follows the `design-taste-frontend` skill.
- Top picture: a widescreen screenshot of the game. Two downloads: Mac and Windows. A "How to install" section with the warning steps. No donation link. No visit counter.
- Footer: "An unofficial fan project. Not affiliated with or endorsed by Sports Interactive, SEGA, Eidos or Square Enix. The download includes the game files from the 2008 free release."
- The site lives in `site/` and publishes with a GitHub Pages workflow. Downloads live in GitHub Releases only.

**Git**
- Git never holds the original's code or data, the logo or the clean files.

## Testing
Two seams.
- **Smoke run.** The shipped program runs a fixed `RECOMP_SCRIPT` without a person: the start screen, a new game and five days, in widescreen mode and classic mode. The test compares the screen dumps with reference pictures kept outside git. The same script runs the Windows exe under CrossOver.
- **Package check.** A script checks each download: architectures, linked libraries (no Homebrew paths), the version number, and the clean files against the manifest.
- Existing tests: `tests/recomp/test_load.py` (database load against Unicorn) and `tests/diff/run.py` (hand port). Both must still pass.
- Manual checks by Max: a double-click from Finder on the Mac. Max has no Windows PC, so the Windows check is the smoke run under CrossOver.

## Out of scope
- Sound (DirectSound stays a stub), network and printing.
- Signing with a Developer ID or a Windows certificate.
- Linux and 32-bit Windows builds.
- Sizes other than 1280x800 and 800x600.
- A button for classic mode inside the game's screens.
- Changes to the hand port.

## Open decisions
None.

## Slices
Slices 1, 2, 3, 5 and 6 run in parallel. Review once for the whole spec, with two Opus review agents.

### 1. GitHub set-up
Blocked by: none
Delivers: the commits are on GitHub, the repo has its new name, milestone 0.1.0 and its issues exist.
Merge danger: one-way door. The push and the rename are public. Blast radius: the original's content in public git history.
- [ ] No file in git history holds the original's code or data
- [ ] Repo renamed; the old URL redirects
- [ ] Milestone 0.1.0 with issues for Mac app, Windows installer, widescreen, site, release

### 2. Clean files
Blocked by: none
Delivers: a tool that makes the clean files from the free release and the official patch on the Mac, the manifest, and a recompiled game built from the clean exe that passes the smoke run.
Merge danger: two-way door. Blast radius: the build input of every release.
- [ ] The tool checks both input hashes and stops on a mismatch
- [ ] The patched exe reports 3.9.68
- [ ] `test_load.py` passes on the clean files
- [ ] Smoke run in classic mode passes

### 3. Widescreen research
Blocked by: none
Delivers: an inventory in `docs/research/` of how the original sizes its screens and areas, and what 1280x800 needs.
Merge danger: two-way door. Documents only.
- [ ] Each fact has its address

### 4. Widescreen mode
Blocked by: 3
Delivers: the game draws at 1280x800 by default, with the background made from the official one; the settings file switches to classic mode.
Merge danger: two-way door. Blast radius: every screen of the game.
- [ ] Smoke run in both modes passes
- [ ] Max plays a new game in widescreen mode

### 5. Mac app
Blocked by: none (it takes the clean files from slice 2 when they exist)
Delivers: a universal `.dmg` that starts from Finder on a Mac without Homebrew; the one version number.
Merge danger: two-way door.
- [ ] Package check passes
- [ ] Max starts the app from Finder

### 6. Windows installer
Blocked by: none (it takes the clean files from slice 2 when they exist)
Delivers: the runtime builds for 64-bit Windows; an NSIS installer that installs, starts and uninstalls.
Merge danger: two-way door. Blast radius: the Mac build, which the smoke run guards.
- [ ] The Mac build and its smoke run are unchanged
- [ ] The installer installs and the game starts under CrossOver
- [ ] Saves go to `%APPDATA%\cm0102`

### 7. Release 0.1.0
Blocked by: 2, 4, 5, 6
Delivers: one script that builds both downloads, runs the package check, writes `CHANGELOG.md`, tags and uploads a GitHub pre-release.
Merge danger: one-way door. It publishes the original's data. Review rule applies, and Max approves the upload.
- [ ] Package check passes on both downloads
- [ ] Pre-release 0.1.0 on GitHub with both files

### 8. Site
Blocked by: 4 for the screenshot; links to 7
Delivers: the site on GitHub Pages with download links to the latest release.
Merge danger: one-way door. The site is public.
- [ ] The taste skill's inspector shows no FAIL
- [ ] Download links point at the latest release
