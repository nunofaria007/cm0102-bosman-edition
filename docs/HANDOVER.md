# Handover, 2026-10-15 (about 2 am)

Read this first. Then read `GLOSSARY.md`, `docs/specs/2026-10-14-quality-of-life-0.2.0-spec.md` and `docs/research/build-brief-0.2.0.md`.

## The product

- **CM 01/02: Bosman Edition**, an unofficial port of Championship Manager 01/02 (3.9.68) to Mac and Windows. The whole original is recompiled from x86 to C (`tools/recomp/`, runtime `src/recomp_rt/`, target `cm0102_recomp`).
- GitHub `maxevans60/cm0102-bosman-edition`. Site https://maxevans60.github.io/cm0102-bosman-edition/ from `site/`.
- **v0.1.0 is released** (pre-release, 2026-10-14): universal Mac `.dmg` with a styled window, 64-bit Windows installer, widescreen 1280x800, clean files only.

## State of 0.2.0

**Merged on `main` and pushed** (issues closed):
- Settings file reader with every option (`src/recomp_rt/win/options.c`).
- Keys and window (#13 to #16): Space to continue, Cmd+[ and mouse back button, Escape for popups, Cmd+F full screen, sharp picture.
- Save folder option (#17).
- Clickable names (#11): hand cursor and underline on hover; the name buttons in news now land on the names in widescreen (#25).
- Widescreen sweep: text widths scaled twice, tactics drag limit.
- Repeatable test runs (`RECOMP_FIXED_CLOCK`, `RECOMP_SAVE_AFTER_DAYS`, `smoke_run.py --same-save`) and faster days on holiday (#20).
- Sound through SDL audio, muted when the window has no focus (#21).
- Save tools `tools/save/` (Max's "Yeovil 1m.sav" was made with them).

**Not merged, work on disk** (the agents stopped at the account's session limit, which resets at 2:20 am):
1. **Attribute colours (#19) and match columns (#23)**: worktree `.claude/worktrees/agent-a069b80021b6499a7`, uncommitted (display.c, options.c, columns.c, tools/recomp/callsites.py, recomp.py, translate.py, the research note). The agent reported both work in widescreen and both smoke modes pass. Left: a same-save check with the options on and off over a played match, the full test set once, commit. Colour bands proposed: 1-5 red, 6-10 amber, 11-15 light green, 16-20 bright green; Max approves on a screenshot. Classic-mode columns: show Max a screenshot, he decides.
2. **Test speed-ups**: worktree `.claude/worktrees/agent-a74c7c18d9652aee1`, uncommitted (waits.c, user32.c, win.h). Wait-for-screen script steps were started; saved test games (`tests/fixtures/`, saves outside git in `/Users/max/Downloads/Game/test-fixtures/`) not yet. Finish it or restart it from the brief in the old task (see the 0.2.0 build brief, "Test speed").
3. **Assistant A key**: branch `worktree-agent-a8d8677cceced7e8f` (commit 8d5345b). **Do not merge.** Max moved assistant selection to 0.3.0 as a squad screen drop-down (#12). Keep the branch for its match-setup code.

**Before releasing 0.2.0:**
1. Finish and merge items 1 and 2 above.
2. Remove the option `assistant_selection` from `options.c` and its test (it moves to 0.3.0).
3. Review the whole 0.2.0 diff with two fresh Opus agents (one against the spec, one against standards). Fix findings with one Sonnet fixer and one brief.
4. Build test downloads for Max (`tools/mac/make_app.sh`, `cmake --build build-win --target windows_installer`) into `/Users/max/Downloads/Game/builds/`. Max checks by ear and eye: sound, keys, full screen, colours, columns.
5. Bump the version to 0.2.0 in `CMakeLists.txt`, add `## 0.2.0` to `CHANGELOG.md`, update the site's options list, then `tools/release/make_release.sh ... --publish` on Max's yes (see "How to make a release").

## Plans after 0.2.0

- **0.3.0** (milestone): #12 Assistant drop-down on the squad screen replacing Clear Squad (Pick best team, most match-fit, under-21, Clear squad; Q64 to Q67), #22 player comparison, squad depth, best XI.
- **Maybe** (label `maybe`, no milestone): #24 team talks.
- Decided against: a "more frequent" violent conduct option (Q61; research in `docs/research/2026-10-14-violent-conduct-inventory.md`).

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
