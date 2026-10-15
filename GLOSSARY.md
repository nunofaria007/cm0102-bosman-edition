# cm0102-mac

A native macOS port of Championship Manager 01/02, rebuilt from the original Windows program.

## Language

### Programs

**Original**:
The shipped Windows program, cm0102.exe. It is the reference for every behaviour.
_Avoid_: binary, the exe, the game

**Recompiled game**:
The original's code translated instruction by instruction into C, with a runtime that replaces the Windows calls. It is the program that every release ships.
_Avoid_: recomp, emulator, the port

**Hand port**:
The clean C program in this repo that rewrites parts of the original by hand and is checked against it by the differential test.
_Avoid_: port (alone), remake, clone, wrapper

### Distribution

**Bosman Edition**:
The product name, written in full as "CM 01/02: Bosman Edition", after the free transfers that older CM players know. It is an unofficial port of Championship Manager 01/02.
_Avoid_: Championship Manager (as the product name), Re-Signed Edition (the old name), CM Modern Edition

**Free release**:
The CD image that Eidos made free to download in 2008. It holds version 3.9.60 of the original and its data.
_Avoid_: freeware, ISO, base game

**Official patch**:
The 3.9.68 update from Sports Interactive, the last official update. It changes the original and its data.
_Avoid_: latest patch, update

**Clean files**:
The free release with the official patch applied, and nothing else. They are the only input to a release. The 3.9.60 data set comes from the same free release.
_Avoid_: vanilla, original files

**Fan pack**:
A community collection that adds a loader, patches or updated data to the game. It is never an input to a release.
_Avoid_: mod pack, starter kit

**Release**:
One numbered version of the Bosman Edition, published with a tag and the downloads for Mac and Windows.
_Avoid_: build, drop

**Download**:
The file that a player gets from a release: the recompiled game with the clean files, ready to start.
_Avoid_: package, installer, bundle

### Display

**Widescreen mode**:
The game drawn at 1280x800. It is the default.
_Avoid_: wide mode

**HD mode**:
The game drawn at 1920x1080 for big screens, made the same way as widescreen mode. Planned for 0.4.0 (#32).
_Avoid_: larger text, 4K mode

**Classic mode**:
The game drawn at 800x600, as the original draws it.
_Avoid_: 4:3 mode, original mode

### Screen

**Screen manager**:
The single object that owns every area and widget on the current screen and draws them in order. The original keeps it in display.cpp.
_Avoid_: scrman, display, UI manager

**Screen stack**:
The list of screens that a player has open, with the newest on top. Each screen names the functions that build it, handle its input and close it. The original keeps it in scrman.cpp.
_Avoid_: scrman, screen history

**Area**:
A rectangular region of the screen, split into a grid of columns and rows. An area can scroll its rows.
_Avoid_: window, pane, region

**Widget**:
One control on the screen, such as a text cell, a button or a picture. A widget sits in a grid cell of an area or stands free.
_Avoid_: control, cell

**Popup area**:
An area that belongs to a widget and opens next to it.
_Avoid_: dropdown, menu

**Panel**:
The background and border of an area or widget.
_Avoid_: frame, box

**Glyph table**:
A small one-colour picture, such as a scroll arrow, stored as one byte per pixel.
_Avoid_: icon, sprite

### Text

**Bitmap font**:
One of the seven fonts that the original loads from its .fnt files and uses when its font setting is -1.
_Avoid_: raster font, FNT font

**Glyph**:
The picture of one character in a bitmap font.
_Avoid_: character image, letter

**Mark**:
A small picture that a widget draws beside its text: the tick or one of the three arrows.
_Avoid_: icon, symbol

**Text entry**:
A widget that takes typed text, such as the first name of the manager. A click, Tab or Enter starts or ends the editing.
_Avoid_: input field, edit box, text field

### Verification

**Differential test**:
A test that runs one function of the original in an emulator, runs the same function of the hand port, and compares the results.
_Avoid_: emulation test, golden test

### Data

**Nation table**:
The nations of the original's database, loaded from nation.dat, one record for each nation.
_Avoid_: country list

**League table**:
The 35 leagues that a new game can include, with the nation of each and whether it is selected.
_Avoid_: competition list, division table

**Key nation**:
A nation that the original finds by name after the load, such as England for the default league.
_Avoid_: main nation

**Data set**:
The season's data that a new game loads: the 2001/02 data of the free release (3.9.60), the 2001/02 December update of the official 3.9.65 patch (3.9.65) or the 2002/03 data of the official patch (3.9.68, the default). The option `database` names it. A download holds 3.9.60 and 3.9.65 in the folders Data-3.9.60 and Data-3.9.65 of the game folder.
_Avoid_: database version, data pack, DB

**Progress bar**:
The box that shows how far a long step has gone, such as "Loading database".
_Avoid_: loading screen

### Options and features

**Settings file**:
The file settings.txt in the save folder. Each line sets one option, such as `display = classic`. It is the only place where the player sets the options of the Bosman Edition.
_Avoid_: config, preferences file, ini

**Option**:
One line of the settings file that switches a feature on or off or sets its value.
_Avoid_: preference, flag, toggle

**Assistant selection**:
The assistant manager picks the player's team, from the Assistant drop-down on the squad screen (0.3.0, #12): best team, most match-fit team or under-21 team.
_Avoid_: auto pick, auto team, AI selection

**Save backup**:
An older copy of a save, kept in `backups/` in the save folder. The game keeps three for each save.
_Avoid_: autosave, snapshot

**Clickable name**:
A person's or club's name drawn on a screen that opens their profile when the player clicks it. It shows an underline only while the mouse is over it.
_Avoid_: link, hyperlink

**Attribute colour**:
The colour of an attribute number by its value on the 1 to 20 scale, as in modern Football Manager.
_Avoid_: attribute highlighting, heatmap
