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
The single object that owns every area and widget on the current screen and draws them in order.
_Avoid_: scrman, UI manager

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

### Verification

**Differential test**:
A test that runs one function of the original in an emulator, runs the same function of the port, and compares the results.
_Avoid_: emulation test, golden test
