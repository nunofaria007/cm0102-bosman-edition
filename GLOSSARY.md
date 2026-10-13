# cm0102-mac

A native macOS port of Championship Manager 01/02, rebuilt from the original Windows program.

## Language

### Programs

**Original**:
The shipped Windows program, cm0102.exe. It is the reference for every behaviour.
_Avoid_: binary, the exe, the game

**Port**:
The C and SDL2 program in this repo that reproduces the original.
_Avoid_: remake, clone, wrapper

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

### Verification

**Differential test**:
A test that runs one function of the original in an emulator, runs the same function of the port, and compares the results.
_Avoid_: emulation test, golden test
