# Handover, 2026-10-16

Read this first, then `GLOSSARY.md`. v0.4.0 was published on 2026-10-16 with Mac, Windows and Linux downloads, and the site shows it. The next work is 0.5.0.

The project lives in `/Users/max/Projects/cm0102`: the repo `cm0102-mac/`, `clean-3968/`, `data-3960/`, `data-3965/`, `sdl2/`, `smoke-ref/` (now with `hd/`), `test-fixtures/` (with `yeovil.sav` and `databases/`: all 49 Update Team databases unpacked, zips in `databases/zips`), `site-release-assets/`, `save-backups/`, `builds/` and `shots/`. Start sessions in `/Users/max/Projects/cm0102`. `build/` is configured with `-DCM0102_EXE=/Users/max/Projects/cm0102/clean-3968/cm0102.exe -DRECOMP_PYTHON=/Users/max/.venvs/cm0102/bin/python`.

## Next session: 0.5.0

1. **0.4.0 is closed:** issues #22, #32, #33, #43 and #54 and the milestone are closed, and the spec is marked shipped. Posts: `/Users/max/Projects/cm0102/release-0.4.0-posts.md` (Reddit update and forum post; whether Max posted them: unknown).
2. **0.5.0: spec ready, build next.** `docs/specs/2026-10-16-features-0.5.0-spec.md` (Q173 to Q202 settled). Issues #46, #47, #48, #56 (options screen in the game, Max's request), #57 (full player search option), #58 (idle CPU, bug). #49 closed as done (Q174). Options screen: two tabs, Screen and Game (Q203; mockup `docs/research/mockups/2026-10-16-options-screen.html`). Build slices 1 to 5 and 7 in parallel, then 6, then 8. Research: `docs/research/2026-10-16-crash-fixes.md`, `-foreign-and-tactics-positions.md`, `-slow-screens.md`, `-options-screen.md`. Nick's CM0102Patcher (github.com/nckstwrt/CM0102Patcher) may be used (Q168); no public page names Nick or Dav.
3. **0.6.0 seasons release** (#50 to #53): decisions Q143 to Q172 in `docs/research/2026-10-15-season-screen-decisions.md`, the 87 change points and five eras in `docs/research/2026-10-16-season-change-points.md`, the chosen layout in `docs/research/mockups/2026-10-15-season-picker.html` (third screen, "A3 + both"). Grill the rest, then spec.
4. **0.7.0 easy online play (#45):** network games today need Tapani's patch and a VPN such as Tailscale; make it quick and easy. The original's network game uses WinSock TCP; `src/recomp_rt/win/net.c` answers every call with "no network". Grill before building.
5. **Later: stadium expansion (#55, Max's request).** Research in `docs/research/2026-10-16-stadium-research.md`: the original already has "Expand the Stadium" in Meet with Board and a yearly expansion routine (0x59cfa0), but 7,269 stadiums have expansion capacity 0 (Highbury among them). Seven decisions are open in that file.
6. **Manual checks Max has not done:** HD (`display = hd`) on a 1080p and a 4K screen; the covered-name underline on Match Overview during a goal; the Windows keys (Ctrl+[, F11) on a Windows PC; `remember_window` on two screens and in full screen; the AppImage on a Steam Deck.
7. **Open checks:** #30 "no match in 60 days" has no test; the same-save comparison against a manual Return from holiday is not done.
8. Player requests and credits: `docs/requests.md`. Releases are announced as an update at the end of the original Reddit post and as a forum post.
9. Worktrees of the 0.4.0 build (`/Users/max/Projects/cm0102/wt-*` and `cm0102-mac/.claude/worktrees/agent-*`) can be removed with Max's permission. Their branches are merged into main.

Facts from 0.4.0:
- **Same game rule (Q152):** a feature must not change the game's decisions while the player makes the same clicks. The original's results depend on where its memory blocks sit: opening any screen, a box or a different display mode changes later days. Same-save checks therefore compare the same steps with the option on and off.
- Max's Yeovil save had Skalidis's release clause removed on 2026-10-16 through the game's own Save Game (backup `save-backups/Yeovil-2026-10-16-before-skalidis-edit.sav`). A direct call of the save function 0x82e430 failed on that screen with "human_manager..cpp 2710". A Python edit of `contract.dat` is not possible yet: the member has no fixed record size.
- `tools/save/savfile.py` reads and writes the .sav container; `tools/save/edit_money.py` sets a club's bank balance.
- The picker scores by the average of the key shown attributes for the position times fit / 20 (Q138 to Q140), with an exact assignment (Q130). Best XI, squad depth and the comparison share `picker_shown_attribute` for other clubs.
- Update Team databases need their whole folder, euro.cfg included; a mismatched euro.cfg made the original loop forever until the skip guard at 0x905a4c.
- Questions for Max go in full in the last message of a turn, never only above the wrap-up box (Max, 2026-10-16).

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
- No feature may change the game's decisions while the player makes the same clicks, except the team that is picked. A change of memory layout counts like opening a screen: the original's results already depend on where its memory blocks sit (Q152). Classic, widescreen and HD saves differ for this reason, as they always have.
- Every option is a line in `settings.txt`. Feature issues labelled `enhancement` show on the site's "Coming next" list live, so their titles are written for players. Bugs use `bug` and never show.
- Grilling before a new feature, then the spec. One shared brief for parallel agents; Opus for research, review and anything a player sees, Sonnet for routine builds.
- Agents must not run `rm` with a wildcard or a variable path.
- Max approves every publish. He has approved pushes for this work.
