# Handover, 2026-10-15

Read this first, then `GLOSSARY.md`. v0.3.0 is released (2026-10-15). The next work is 0.4.0.

The project lives in `/Users/max/Projects/cm0102` (moved from `~/Downloads/Game` on 2026-10-15): the repo `cm0102-mac/`, `clean-3968/`, `data-3960/`, `data-3965/`, `sdl2/`, `smoke-ref/`, `test-fixtures/`, `site-release-assets/`, `builds/` and `shots/`. Start sessions in `/Users/max/Projects/cm0102`. `~/Downloads/Game` now holds only the 2021 fan pack, which is never a build input. `build/` was configured fresh with `-DCM0102_EXE=/Users/max/Projects/cm0102/clean-3968/cm0102.exe -DRECOMP_PYTHON=/Users/max/.venvs/cm0102/bin/python`. `build-win/` is made again by the release script.

## Next session: 0.4.0

v0.3.0 is out as a pre-release with Mac, Windows and Linux downloads. The 0.3.0 issues and milestone are closed. Spec `docs/specs/2026-10-15-features-0.3.0-spec.md` (all slices built).

1. **Build the 0.4.0 spec** `docs/specs/2026-10-15-features-0.4.0-spec.md` (grilling Q100 to Q137 done 2026-10-15). Slice 1 (picker fix) ships first as 0.3.1. Slices 1, 3, 7, 8 and 9 can start in parallel. The picker check needs a copy of Max's Yeovil save in `/Users/max/Projects/cm0102/test-fixtures` (source: the save in the `_outside` folder of `~/Library/Application Support/cm0102`; copy it, never edit it). The investigation scripts of 2026-10-15 were in the session scratchpad and are gone; the spec holds the findings.
2. **Manual checks Max has not done:** the Windows keys (Ctrl+[, F11) on a Windows PC, `remember_window` on two screens and in full screen, the AppImage on a Steam Deck. u/gundenaf says the Windows build runs on the Deck through Proton.
3. **Open spec checks:** #30 "no match in 60 days" message has no test (no such club in the test saves), and the same-save comparison against a manual Return from holiday is not done.
4. Player requests and credits: `docs/requests.md`. Credit each requester by name, feature by feature, when their feature ships. Releases are announced as an update at the end of the original Reddit post (not a new post) and as a forum post. The 0.3.0 texts are in `/Users/max/Projects/cm0102/release-0.3.0-reddit-post.md`. Whether Max has posted them: unknown. The screenshots of the Reddit thread showed 16 of 17 comments; only u/gundenaf made suggestions in them.
5. The worktrees of the 0.3.0 build were removed on 2026-10-15 (Max's permission). Their branches stay in git.

Facts from 0.3.0:
- Saved test games in `/Users/max/Projects/cm0102/test-fixtures`: `start.sav` (Arsenal 14.7.01, first friendly 18.7.01) and `prematch.sav` (16.7.01, Basel friendly 20.7.01; another game, other fixtures).
- Wait steps in scripts (`src/recomp_rt/win/waits.c`), `--match` and `--match --speed fastest` in `tests/smoke/smoke_run.py`.
- In the original the commentary speed already changes match results (background matches step while a line waits). So a speed is checked for repeatability, not against another speed (Q99).
- Game addresses that the runtime calls go through `tools/recomp/callbacks.py` (checked bytes) or `tools/recomp/callsites.py`.
- Docker for the Linux build: Colima with `--vm-type vz --vz-rosetta` (`colima start --arch aarch64 --cpu 6 --memory 8 --vm-type vz --vz-rosetta`).
- Contract clauses in memory and how to edit a save through the game: `docs/research/2026-10-15-contract-clauses.md`. Max's Yeovil save had Moukoko's release clause removed on 2026-10-15 (backup in `/Users/max/Projects/cm0102/save-backups`).
- The old preview server on port 8765 serves the old folder. Use the `site-8766` entry in `/Users/max/Projects/cm0102/.claude/launch.json`.

## The product

- **CM 01/02: Bosman Edition**, an unofficial port of Championship Manager 01/02 (3.9.68) to Mac, Windows and Linux. The whole original is recompiled from x86 to C (`tools/recomp/`, runtime `src/recomp_rt/`, target `cm0102_recomp`).
- GitHub `maxevans60/cm0102-bosman-edition`. Site https://maxevans60.github.io/cm0102-bosman-edition/ from `site/` (pictures come from `site-assets.zip` on the release).
- Pre-releases: v0.1.0 and v0.2.0 (Mac, Windows), v0.3.0 (Mac, Windows, Linux AppImage; three data sets: 3.9.60, 3.9.65, 3.9.68). All 2026-10-14/15.
- **Maybe** (label `maybe`, no milestone): #24 team talks, #36 2D match view. Later, no milestone: #37 Android, #38 iOS.
- Decided against: a "more frequent" violent conduct option (Q61), a screenshot key, a warning before quit (the original has one), highlights of young players (needs hidden potential values), a newer match engine (it would change results).

## How to make a release

1. Bump `project(... VERSION x.y.z)` and add the `CHANGELOG.md` section. Commit.
2. Dry run: `tools/release/make_release.sh --game /Users/max/Projects/cm0102/clean-3968 --data-3960 /Users/max/Projects/cm0102/data-3960 --data-3965 /Users/max/Projects/cm0102/data-3965 --sdl2-mac /Users/max/Projects/cm0102/sdl2 --sdl2-mingw /Users/max/Projects/cm0102/sdl2/mingw-2.32.10 --site-assets /Users/max/Projects/cm0102/site-release-assets`. Then the same with `--publish` after Max says yes. Colima must run for the Linux step. The checkout must be clean.
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
