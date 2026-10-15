# Handover, 2026-10-15

Read this first, then `GLOSSARY.md`. v0.3.0 is released (2026-10-15). The next work is 0.4.0.

The project lives in `/Users/max/Projects/cm0102` (moved from `~/Downloads/Game` on 2026-10-15): the repo `cm0102-mac/`, `clean-3968/`, `data-3960/`, `data-3965/`, `sdl2/`, `smoke-ref/`, `test-fixtures/`, `site-release-assets/`, `builds/` and `shots/`. Start sessions in `/Users/max/Projects/cm0102`. `~/Downloads/Game` now holds only the 2021 fan pack, which is never a build input. `build/` was configured fresh with `-DCM0102_EXE=/Users/max/Projects/cm0102/clean-3968/cm0102.exe -DRECOMP_PYTHON=/Users/max/.venvs/cm0102/bin/python`. `build-win/` is made again by the release script.

## Next session: 0.4.0

v0.3.0 is out as a pre-release with Mac, Windows and Linux downloads. The 0.3.0 issues and milestone are closed. Spec `docs/specs/2026-10-15-features-0.3.0-spec.md` (all slices built).

1. **Grilling, then one spec for 0.4.0.** Issues in the milestone: #22 player comparison with the polygon analyser, squad depth and best XI (Max's mockup in this session: a 4-4-2 on a pitch and a depth table), #32 HD layout at 1920x1080 (`display = hd`), #33 print to PDF, #43 attribute groups (physical, technical, mental).
2. **Manual checks Max has not done:** the Windows keys (Ctrl+[, F11) on a Windows PC, `remember_window` on two screens and in full screen, the AppImage on a Steam Deck. u/gundenaf says the Windows build runs on the Deck through Proton.
3. **Open spec checks:** #30 "no match in 60 days" message has no test (no such club in the test saves), and the same-save comparison against a manual Return from holiday is not done.
4. Player requests and credits: `docs/requests.md`. Credit each requester in the release post when their feature ships. Draft posts for 0.3.0: `/Users/max/Projects/cm0102/release-0.3.0-reddit-post.md`.

Facts from 0.3.0:
- Saved test games in `/Users/max/Projects/cm0102/test-fixtures`: `start.sav` (Arsenal 14.7.01, first friendly 18.7.01) and `prematch.sav` (16.7.01, Basel friendly 20.7.01; another game, other fixtures).
- Wait steps in scripts (`src/recomp_rt/win/waits.c`), `--match` and `--match --speed fastest` in `tests/smoke/smoke_run.py`.
- In the original the commentary speed already changes match results (background matches step while a line waits). So a speed is checked for repeatability, not against another speed (Q99).
- Game addresses that the runtime calls go through `tools/recomp/callbacks.py` (checked bytes) or `tools/recomp/callsites.py`.
- Docker for the Linux build: Colima with `--vm-type vz --vz-rosetta` (`colima start --arch aarch64 --cpu 6 --memory 8 --vm-type vz --vz-rosetta`).
- Contract clauses in memory and how to edit a save through the game: `docs/research/2026-10-15-contract-clauses.md`. Max's Yeovil save had Moukoko's release clause removed on 2026-10-15 (backup in `/Users/max/Projects/cm0102/save-backups`).
- The old preview server on port 8765 serves the old folder. Use the `site-8766` entry in `/Users/max/Projects/cm0102/.claude/launch.json`.

## The product

- **CM 01/02: Bosman Edition**, an unofficial port of Championship Manager 01/02 (3.9.68) to Mac and Windows. The whole original is recompiled from x86 to C (`tools/recomp/`, runtime `src/recomp_rt/`, target `cm0102_recomp`).
- GitHub `maxevans60/cm0102-bosman-edition`. Site https://maxevans60.github.io/cm0102-bosman-edition/ from `site/`.
- **v0.1.0** (2026-10-14) and **v0.2.0** (2026-10-15) are released as pre-releases: universal Mac `.dmg`, 64-bit Windows installer, widescreen 1280x800, clean files only.

## State of 0.2.0

**Released as v0.2.0 (2026-10-15):** every 0.2.0 feature, the review fixes, version 0.2.0 in `CMakeLists.txt` and the `## 0.2.0` changelog. The option `assistant_selection` is gone. The site lists the 0.2.0 options in its FAQ.

- Features: settings file, keys and window (#13 to #16), save folder (#17), clickable names (#11, #25), widescreen sweep, faster holiday days (#20), sound (#21), attribute colours (#19), match columns (#23, widescreen only).
- Review: two Opus reviews (`docs/research/review-brief-0.2.0.md`), one Sonnet fixer (`docs/research/fix-brief-0.2.0.md`). The one must-fix (a stack pop count in `dsound.c`) is fixed.
- Tests on the merged code: both smoke modes, `--same-save` with five runs (one with every option off), `test_load.py` (needs `/Users/max/.venvs/cm0102/bin/python`), `ctest -R options`. All pass.
- **Not checked (Max chose to skip):** a same-save check over a played match with the colours and columns on and off. Scripted runs from a pre-match save stop at the squad screen before the next friendly, so the save date was never reached. Evidence instead: both reviews found no write path, the columns code only reads, and the columns show in a live match.
- **Left out of 0.2.0 (Max):** the test speed-ups. Branch `worktree-agent-a74c7c18d9652aee1`, commit 7d14701 says what is left (match screen address in `waits.c`, test runs of the wait steps and of `tests/fixtures/fixtures.py`).
- **Assistant A key:** branch `worktree-agent-a8d8677cceced7e8f`. Do not merge. Kept for its match-setup code (0.3.0, #12).

**Done:** Max checked the test build and approved the colour bands and widescreen-only columns. Published as v0.2.0 on 2026-10-15. The Windows keys question is now #26.

## Plans after 0.2.0

**v0.2.0 is released (2026-10-15)**, with the site, the forum post and a Reddit post. Max approved these for 0.3.0 (issues in the milestone):
- #12 Assistant drop-down on the squad screen (Q64 to Q67), #22 player comparison, squad depth, best XI.
- #26 Windows shortcut keys (bug: Ctrl+[ and F11), #27 save backups, #28 remember the window, #29 colours for condition and morale, #30 holiday until the next match, #31 faster match commentary, #32 larger text, #33 print to PDF, #34 save names with club and date, #35 Linux and Steam Deck.
- #30 and #31 need a same-save check over a played match. Finish the test speed-ups first (branch `worktree-agent-a74c7c18d9652aee1`): the saved test games make that check possible.
- Grilling, then the spec, before the build.

**Maybe** (label `maybe`, no milestone): #24 team talks. Max likes the idea of notifications for something other than holiday results (not yet decided what).
Decided against: a "more frequent" violent conduct option (Q61), a screenshot key, a warning before quit (the original has one), highlights of young players (needs hidden potential values).

## How to make a release

1. Bump `project(... VERSION x.y.z)` and add the `CHANGELOG.md` section. Commit.
2. Dry run: `tools/release/make_release.sh --game /Users/max/Projects/cm0102/clean-3968 --sdl2-mac /Users/max/Projects/cm0102/sdl2 --sdl2-mingw /Users/max/Projects/cm0102/sdl2/mingw-2.32.10 --site-assets /Users/max/Projects/cm0102/site-release-assets`. Then the same with `--publish` after Max says yes.
3. The site workflow runs on the release (the Pages environment allows `main` and tags `v*`). If not: `gh workflow run pages.yml --ref main`.

## Tests

- `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968 /Users/max/Projects/cm0102/smoke-ref --mode classic`, `--mode widescreen` and `--same-save`. Run test games in parallel (Max, 2026-10-15). If a parallel run fails on timing, rerun it alone first.
- `tests/recomp/test_load.py` (see its docstring), `ctest -R options` in `build/`, `tests/smoke/save_folder_run.py`, `tests/smoke/save_backups_run.py`.
- While building, run only the touched test; the full set once per slice.

## Rules

- Never commit the original's code or data, saves, screenshots or the logo. Downloads use the clean files only (`/Users/max/Projects/cm0102/clean-3968`, made by `tools/clean_files/`); the fan pack in `/Users/max/Downloads/Game` is never a build input.
- No feature may change match results or the game's decisions, except picking a team (Max: selection is selection).
- Every option is a line in `settings.txt`. Feature issues labelled `enhancement` show on the site's "Coming next" list live, so their titles are written for players. Bugs use `bug` and never show.
- Grilling before a new feature, then the spec. One shared brief for parallel agents; Opus for research, review and anything a player sees, Sonnet for routine builds.
- Agents must not run `rm` with a wildcard or a variable path.
- Max approves every publish. He has approved pushes for this work.
