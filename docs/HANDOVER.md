# Handover, 2026-10-15

Read this first, then `GLOSSARY.md`. v0.2.0 is released. The next work is 0.3.0.

## Next session: 0.3.0 features

1. **Test speed-ups first.** Branch `worktree-agent-a74c7c18d9652aee1` (commit 7d14701 lists what is left). Find the match screen build address for `MATCH_BUILD` in `waits.c` with `RECOMP_SCREEN_LOG=1`, run both smoke modes and `--same-save` with the wait steps, then check the load steps of `tests/fixtures/fixtures.py` and make `start.sav`. A pre-match save (Arsenal, the Basel friendly on 20.7.01 is next) is in `/Users/max/Downloads/Game/test-fixtures/prematch.sav`.
2. **Make the match same-save check work.** #30 and #31 need it. What failed on 2026-10-15:
   - The left-bar button at (72, 161) in widescreen is Pause or Resume during a match and Continue Game outside it, so a run of Continue clicks pauses the match.
   - The tactics screen pauses the match, and a click on Ok while paused does nothing.
   - After the match, Continue stops at the squad screen before the next friendly (Baden), so `RECOMP_SAVE_AFTER_DAYS` 7 or 8 never fired. The game date at that point is unknown. A wait step (`wait day:...`, `wait screen:...`) fixes this kind of guess.
3. **Grilling, then one spec for the 0.3.0 features**, then the build in slices with parallel agents and one review per spec (`feedback-cm0102-speed` in memory). Issues: #12, #22, #26 to #35 in the 0.3.0 milestone. #26 is a small bug fix and can go first.
4. Site pictures: add new ones to `/Users/max/Downloads/Game/site-release-assets/` (now with `s2@2x.png`, the coloured profile). The release script zips that folder.

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
2. Dry run: `tools/release/make_release.sh --game /Users/max/Downloads/Game/clean-3968 --sdl2-mac /Users/max/Downloads/Game/sdl2 --sdl2-mingw /Users/max/Downloads/Game/sdl2/mingw-2.32.10 --site-assets /Users/max/Downloads/Game/site-release-assets`. Then the same with `--publish` after Max says yes.
3. The site workflow runs on the release (the Pages environment allows `main` and tags `v*`). If not: `gh workflow run pages.yml --ref main`.

## Tests

- `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Downloads/Game/clean-3968 /Users/max/Downloads/Game/smoke-ref --mode classic`, then `--mode widescreen`, then `--same-save` alone. Never run two test games in parallel.
- `tests/recomp/test_load.py` (see its docstring), `ctest -R options` in `build/`, `tests/smoke/save_folder_run.py`.
- While building, run only the touched test; the full set once per slice.

## Rules

- Never commit the original's code or data, saves, screenshots or the logo. Downloads use the clean files only (`/Users/max/Downloads/Game/clean-3968`, made by `tools/clean_files/`); the fan pack in `/Users/max/Downloads/Game` is never a build input.
- No feature may change match results or the game's decisions, except picking a team (Max: selection is selection).
- Every option is a line in `settings.txt`. Feature issues labelled `enhancement` show on the site's "Coming next" list live, so their titles are written for players. Bugs use `bug` and never show.
- Grilling before a new feature, then the spec. One shared brief for parallel agents; Opus for research, review and anything a player sees, Sonnet for routine builds.
- Agents must not run `rm` with a wildcard or a variable path.
- Max approves every publish. He has approved pushes for this work.
