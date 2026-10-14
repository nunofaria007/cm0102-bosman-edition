# Handover, 2026-10-14 (evening)

Read this first in the next session, then `GLOSSARY.md`.

## Where things stand

- **Released:** CM 01/02: Bosman Edition v0.1.0, a GitHub pre-release with a universal Mac `.dmg` and a 64-bit Windows installer. Spec `docs/specs/2026-10-14-re-signed-edition-release-spec.md` (shipped; the file keeps its old name).
- **Site:** https://maxevans60.github.io/cm0102-bosman-edition/ from `site/`, published by `.github/workflows/pages.yml`. Its pictures come from `site-assets.zip` on the newest release. The Pages environment allows `main` and tags `v*`.
- **Repo:** GitHub `maxevans60/cm0102-bosman-edition` (old names redirect). Milestone 0.1.0 is closed. Milestone 0.2.0 holds issue #11: click a name in news to open the profile.

## How to make the next release

1. Bump `project(... VERSION x.y.z)` in `CMakeLists.txt` and add a `## x.y.z` section to `CHANGELOG.md`. Commit.
2. Run `tools/release/make_release.sh --game /Users/max/Downloads/Game/clean-3968 --sdl2-mac /Users/max/Downloads/Game/sdl2 --sdl2-mingw /Users/max/Downloads/Game/sdl2/mingw-2.32.10 --site-assets /Users/max/Downloads/Game/site-release-assets` (dry run), then again with `--publish` after Max says yes.
3. The site workflow runs on the release. If it does not, run `gh workflow run pages.yml --ref main`.

## Tests

- `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Downloads/Game/clean-3968 /Users/max/Downloads/Game/smoke-ref --mode classic` and `--mode widescreen`. References live outside git.
- `tests/recomp/test_load.py` (see its docstring) and the hand port's `tests/diff/run.py`.

## Known limits of 0.1.0

- No sound (DirectSound is a stub), no network play, no printing.
- In widescreen, popup menus and message boxes keep their 800x600 size and place. They look fine.
- Unsigned: Gatekeeper and SmartScreen warn once. The site explains the clicks.
- The Windows build is tested only under CrossOver. Max has no Windows PC.

## Rules that still apply

- Never commit the original's code or data, the logo or screenshots. Clean files only in downloads; the fan pack in `/Users/max/Downloads/Game` is never a build input.
- Grilling before a new feature, then a spec. Track bugs and features as GitHub issues in a milestone.
- Agents must not run `rm` with a wildcard or a variable path.
