# Changelog

CM 01/02 Bosman Edition uses semantic versioning. A patch release (0.1.1) fixes bugs. A minor release (0.2.0) adds features. A major release (1.0.0 and later) can break saved games or settings. Releases below 1.0.0 are test releases.

## 0.2.0 (pre-release)

Comfort features and sound. Each feature has a line in `settings.txt` in the save folder, and `off` turns it off. No feature changes a match result or a decision of the game.

- Sound: the game's own sounds play. The game is silent while its window is not in front. `sound = off` mutes it.
- The space bar presses Continue Game.
- Back: Cmd+[ on a Mac, or the back button of a mouse. Escape closes a popup. Cmd+F switches full screen on and off.
- The picture scales in whole pixels, so text stays sharp.
- `save_folder` keeps saved games in another folder, for example in iCloud Drive.
- Player and club names show a hand and an underline under the mouse. A click opens the profile.
- Attribute numbers have colours by value: 1 to 5 red, 6 to 10 amber, 11 to 15 light green, 16 to 20 bright green.
- In a match, widescreen only: the tactics screen shows each player's positions and condition beside the match rating, and the Player Ratings view shows the condition of both teams.
- Days pass faster while you are on holiday.
- Widescreen fixes: longer texts fit, and the name buttons in the news land on the names.

## 0.1.0 (pre-release)

The first test release.

- Championship Manager 01/02 at version 3.9.68: the 2008 free release with the last official patch. No fan patches or fan databases.
- Mac: one app for Apple silicon and Intel, macOS 11 or later.
- Windows: an installer for 64-bit Windows 10 and 11, with Start menu and desktop shortcuts.
- Widescreen at 1280 × 800 by default. The window can be resized and keeps the shape of the picture.
- The old 800 × 600 layout: open `settings.txt` in the save folder and change the line to `display = classic`.
- Saves and settings live in `~/Library/Application Support/cm0102` on a Mac and in `%APPDATA%\cm0102` on Windows.

Not in this release: sound, network play and printing.
