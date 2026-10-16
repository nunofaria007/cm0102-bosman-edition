# Contributing

Problem reports and feature ideas are welcome. Open an [issue](https://github.com/maxevans60/cm0102-bosman-edition/issues/new/choose) with the form for each.

For code:

- Never commit any part of the original game: its code, data, pictures, logo or saved games. The build makes the game from your own copy of the free release (see the README).
- No change may alter match results or the game's decisions, except the team that a player or the assistant picks.
- Each new feature is a line in `settings.txt`, with a default and a test in `tests/options`.
- Hook the game only through the checked mechanisms in `tools/recomp` (`callbacks.py`, `callsites.py`), so that the build stops if an address no longer matches.
- Run the smoke tests in `tests/smoke` in classic and widescreen mode before you open a pull request.
- `GLOSSARY.md` names each part of the game and the project. Use its words in code comments and docs.
