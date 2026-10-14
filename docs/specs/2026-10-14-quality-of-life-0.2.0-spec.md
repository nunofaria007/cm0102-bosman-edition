# Quality of life, release 0.2.0
Status: Ready

## Problem
Release 0.1.0 plays the 2002 game, but without sound and without the comforts players know from modern Football Manager: names you can click, colours on attributes, an assistant who picks the team, keys that save clicks, and days that pass quickly.

## Solution
Release 0.2.0 adds those comforts and sound, without changing a single match result or game decision. Every new option is a line in the settings file, explained on the site.

## User stories
1. As a player, I want to press Space to continue the game, so that I do not move the mouse to Continue Game each day.
2. As a player, I want Cmd+[ or Alt+Left or the mouse back button to go Back, and Escape to close a popup.
3. As a player, I want Cmd+F (Mac) or Alt+Enter (Windows) to switch to full screen and back.
4. As a player on a Retina or 4K screen, I want crisp text, not a blurred picture.
5. As a player with two computers, I want my save folder in iCloud Drive or Dropbox, so that one career plays on both.
6. As a player, I want the game's sounds, following the game's own Sound On/Off setting.
7. As a player who does not enjoy team selection, I want my assistant to pick the team before each match, keeping my tactic.
8. As a player reading news, I want to click a player's or club's name to open the profile, and Back to return to the news.
9. As a player on any screen, I want the same clickable names on squads, transfers and match reports.
10. As a player scouting, I want attribute numbers coloured by value, so that I see strengths at once.
11. As a player on holiday or at the end of a season, I want days to pass faster.
12. As a player, I want the same results with or without these features, so that the game stays the 2002 game.
13. As a player who prefers the original, I want to switch each comfort off in the settings file.

## Decisions
Settled in the interview of 2026-10-14 (Q50 to Q58).

- **Scope:** 0.2.0 holds issues #11 to #21 and #23 (position, condition and match rating together on the tactics screen, added by Max after the interview). The new screens (#22: player comparison, squad depth, best XI) are release 0.3.0.
- **Options:** every option is a line in the settings file. No new in-game options screen in 0.2.0. The site lists each option with its default.
- **Defaults:** on: Space to continue, shortcuts, sharp picture, sound, clickable names, attribute colours, faster days. Assistant selection is available by default and used per match on request. The save folder defaults to the current folder.
- **Faithful logic:** no feature may change match results or the game's decisions. Faster days must leave a save that is byte for byte the same as normal days, given the same start and the same input.
- **Assistant selection:** always available, used on request (Max, Q62): the A key on the tactics screen before a match asks the assistant to pick the eleven and subs, keeping the tactic. It uses the original's own holiday code for computer-picked teams. The option `assistant_selection = on` (default) enables the key; there is no automatic mode in 0.2.0. Picking the team changes later results, as any selection does; Max does not count that as an exception to the faithful-logic rule.
- **Attribute colours:** modern FM colours on the 1 to 20 scale: low red, mid amber, high green, top band bright green. The exact band edges are set in the build slice and shown to Max on a screenshot before release.
- **Sound:** the original's own sound files from the clean files, played through SDL audio. It follows the game's Sound On/Off setting. No new volume control.
- **Clickable names:** look the same as now, with an underline only while the mouse is over the name. A click calls the game's own function that opens the profile, so Back works as normal.
- **Sharp picture:** whole-pixel scaling when the window allows it, otherwise the nearest sharp scale. The picture keeps its shape, as in 0.1.0.
- **Save folder option:** one line names the folder. The game creates it if it does not exist, and never moves existing saves on its own.
- **Releases:** semantic versioning as before. 0.2.0 is a pre-release.

## Testing
- **Smoke run** in both modes (`tests/smoke/smoke_run.py`) must pass after every slice.
- **Same-save check (new):** the smoke run saves the game after a fixed number of days, once with every option off and once with every option on (assistant selection off, since it changes the team by design). The two saves must be identical. Faster days gets its own pair of runs.
- **Screen checks:** for drawing features (names, colours, sharp picture), a scripted run dumps the screen and the slice shows before and after screenshots of about 800 px.
- **Manual checks by Max:** sound, full screen and the feel of the keys, in the real Mac app.
- Existing tests: `tests/recomp/test_load.py` and the package checks.

## Out of scope
- New screens (#22), in 0.3.0.
- A volume slider or music.
- Network play and printing.
- Any change to the match engine, AI decisions or the database.
- A "more frequent" violent conduct option. The events are already in the game, always on and rare; Max chose to leave them as they are (Q61, 2026-10-14). See `docs/research/2026-10-14-violent-conduct-inventory.md`.

## Open decisions
None.

## Slices
Review once for the whole spec with two Opus review agents. Slices 2, 3, 4 and 5 can run in parallel after slice 1.

### 1. Settings file options
Blocked by: none
Delivers: one reader for every option in the settings file, with defaults, and the file written with comments for every option on first start. Existing `display` keeps working.
Merge danger: two-way door. Blast radius: every option.
- [ ] Unknown lines are kept and ignored
- [ ] A missing file is written with every option at its default

### 2. Keys and window (#13 to #16)
Blocked by: 1
Delivers: Space to continue, Back shortcuts and the mouse back button, Escape for popups, full screen toggle, sharp picture.
Merge danger: two-way door. Blast radius: input and the window on every screen.
- [ ] Space continues only where Continue Game is on screen, and never while text is typed
- [ ] Smoke runs pass
- [ ] Max checks the keys and full screen in the app

### 3. Save folder option (#17)
Blocked by: 1
Delivers: the save folder option on Mac and Windows.
Merge danger: two-way door. Blast radius: where saves go. It never moves or deletes saves.
- [ ] Saves and loads work in a folder with spaces and non-ASCII letters

### 4. Sound (#21)
Blocked by: 1
Delivers: the game's sounds through SDL audio, following the game's Sound setting.
Merge danger: two-way door. Blast radius: sound only, and the DirectSound stub.
- [ ] Smoke runs pass with the dummy audio driver
- [ ] Max hears the sounds in a match

### 5. Research (#11, #12, #18, #19, #20)
Blocked by: none
Delivers: one inventory in `docs/research/` answering: what the tactics screen's View drop-down offers and where it draws the column beside each name, and where the match screen's Player Ratings view draws its ratings; does a news item keep the IDs of the people and clubs it names; which function opens a profile; how the game picks teams for computer-run clubs and whether it can run for the player's club with the tactic kept; where attributes are drawn; what the day loop draws and which drawing can be skipped safely.
Merge danger: two-way door. Documents only.
- [ ] Each fact has its address

### 5b. Repeatable test runs
Blocked by: 5
Delivers: two unattended runs with the same `-seed`, the same script and the same start give byte-identical saves. Research B found that runs drift today (first suspect `srand(time())` at 0x696d51). A test-only fixed clock is allowed; play is unchanged.
Merge danger: two-way door. Test runs only.
- [ ] Two normal runs to a fixed date give identical saves

### 6. Assistant selection (#12)
Blocked by: 1, 5, 5b
Delivers: the option, off by default.
Merge danger: two-way door. Blast radius: the player's team before matches.
- [ ] With the option off, the same-save check passes
- [ ] With it on, the team is picked before a match and the tactic is kept

### 7. Clickable names in news (#11)
Blocked by: 1, 5
Delivers: clickable names in news items.
Merge danger: two-way door. Blast radius: text drawing on the news screen.
- [ ] A click opens the right profile, and Back returns to the news

### 8. Clickable names everywhere (#18)
Blocked by: 7
Delivers: clickable names on squads, transfers and match reports.
Merge danger: two-way door. Blast radius: text drawing on those screens.
- [ ] Screenshots of each screen with a hovered name

### 9. Attribute colours (#19)
Blocked by: 1, 5
Delivers: coloured attribute numbers.
Merge danger: two-way door. Blast radius: attribute screens.
- [ ] Max approves the colours on a screenshot

### 10. Faster days (#20)
Blocked by: 1, 5, 5b
Delivers: days that pass without redrawing every screen.
Merge danger: two-way door, but it touches the day loop. Blast radius: every saved game. The same-save check is the guard.
- [ ] The same-save check passes with faster days on and off

### 11. Position, condition and match rating together during matches (#23)
Blocked by: 1, 5
Delivers: an option, on by default, that shows the View drop-down's Position(s), Condition and Match Rating columns side by side beside each name on the tactics screen. The drop-down already offers each one alone (Max checked). The match screen's Player Ratings view also shows each player's condition beside the rating, for both teams. Widescreen has room; in classic mode, if the columns do not fit, the option applies in widescreen only.
Merge danger: two-way door. Blast radius: the tactics screen.
- [ ] Screenshots of the tactics screen (three columns) and the Player Ratings view (rating and condition) in a match, in both modes

### 12. Release 0.2.0
Blocked by: 2 to 11
Delivers: the options on the site, the changelog, and the pre-release.
Merge danger: one-way door. It publishes. Max approves the upload.
- [ ] Package checks pass
- [ ] Site lists every option with its default
