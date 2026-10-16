# Research: our options in the game (0.5.0)

Source: `clean-3968/cm0102.exe`, disassembled with capstone (helper `scratchpad/opt/dis.py`), and the runtime in `src/recomp_rt/win/`. Inferences have the mark **[inference]**.

Screenshots (800 px wide, from start.sav, widescreen):
- `scratchpad/options-game-options-popup-widescreen.png`: the side bar "Game Options" drop-down.
- `scratchpad/options-game-settings-widescreen.png`: the original's "Game Settings" box.

## 1. What the original does today

### 1a. One options screen: "Game Settings"

The original has one options screen, the "Game Settings" box. It opens from three places, all through `FUN_00823890(arg)`:

| From | How | Address |
|---|---|---|
| Start screen, menu item 5 "Game Settings" (id 6) | input 0x81bd70 calls 0x823890(1) | 0x81bd70, inventory 6c |
| First start (no game.cfg yet) | slot 0 = 1 when `[0xae3138]+0x213` == 0. The start input calls 0x823890(0) | 0x81ba23, 0x81bd82 |
| In a game: side bar "Game Options" drop-down, item "Game Settings" (id 0x423) | the side bar sets the hook `S+0x37ee` = 0x823930 for id 0x423 (0x75db45). 0x823930 calls 0x823890(2) | item at 0x7605a3..0x760602 |

The "Game Options" drop-down (0x760254) holds Save Game, Add Manager, Manager Status, Game Settings, Selected Leagues, Hall of Fame, Game Credits, Web Sites, Restart Game, Exit Game (screenshot).

`FUN_00823890` (0x823890..0x82392b):
- pushes a screen with `0x7fe600(build 0x823950, input 0x8259a0, flag 1, 0, 0)` (0x8238bb). **[inference]** flag 1 draws it as a box over the screen below.
- allocates a copy of the settings object, 0x31b bytes (`new` 0x944cf1, copy 0x5d7ad0 of `[0xae3138]`), into node slot 0. Slot 1 = the caller's argument (0x823905, 0x823918).

Build `0x823950`:
- Box area (145, y)-(744, y2), 500 or 475 high, centred (0x823a97..0x823af4). Heading "Game Settings" (0x803fd0 at 0x823b41), version line, "Reset" button id 0x27 (0x804030 at 0x823bff), "Cancel" id 0x28 and "Ok" id 0x29 (0x8040a0 at 0x823c78, 0x823cb8).
- The rows: one area with 5 columns and 15 rows (14 when only one language) (0x823d44, row count at `[esp+0x30]` = 0xf, 0xe at 0x823d06). All 15 rows are in use. There is no free row.
- Rows: Language, Currency, Fonts, Wages, Screen Mode, Background Changes, Sound, Database, Matches Stored, Auto Save, Save Compressed, Save Safely, Flashing Text, Background Matches, Print To (our "PDF").

Widget types (only two kinds):
- **Choice row** (Wages to Print To): a label in column 0, then one cell per value. The current value is a yellow label with a frame (`0x804100`, style 0x221). Each other value is a button (`0x8040a0`) with id 0x2c and data = the value's bit, for example Sound On = 0x80, Off = 0x100 (0x8247a9..0x8248bc). A value that cannot change now is grey (Database, Matches Stored in a game).
- **Drop-down row** (Language, Currency, Fonts, Auto Save): a yellow label with the current value and a small arrow button. The list is an area with flag 0x100030 and one item per value (0x55fba0, 0x55f990 at 0x823dcd..0x823f33).
- No check boxes. No text entry.

Input `0x8259a0`:
- 0x27 Reset: 0x5d7bc0 on the copy (defaults), return -11 (build again).
- 0x28 Cancel: 0x5d8480 on the live object `[0xae3138]`, free the copy, return -9 (close).
- 0x29 Ok: 0x5d8480 on the copy (applies it and writes `CM3_PREFS\game.cfg`, 0x5d87ea), free the copy, return -9.
- 0x2c: 0x5d7c60(bit) sets one bit group. 0x2a language (0x5d8020, and 0x827d50 in a game). 0x2b, 0x3d, 0x2d: the other drop-downs (0x5d8270, 0x5d8b50, 0x5d83e0).
- So every change waits for Ok. Cancel keeps the old values.

Commentary speed is not on this box. It is a drop-down on the match screen (0x718e90), stored in the manager record +0x9ad2 and in the saved game (`2026-10-15-commentary-speed-inventory.md`).

### 1b. Rows that overlap our options

- Sound On/Off (bits 0x80/0x100) and our `sound`. Our `sound = off` mutes everything. With `sound = on` the game's row still applies (options.c comment).
- Screen Mode Full/Windowed (bits 0x8/0x10). **[inference]** It does nothing in the port: `SetCooperativeLevel` returns 0 (ddraw.c:187). Full screen is our F11/Cmd+F key and `remember_window`.
- Print To: already relabelled by us (print_pdf.c). This is the one place where we already changed this box.

## 2. What Nick's patch does

Nothing here. No patch in `Patcher.cs` touches 0x823890..0x825cbf (file offsets 0x423890..0x425cbf), and no file names game.cfg.

## 3. What our recompile already has

### 3a. The store and its write path

`src/recomp_rt/win/options.c`:
- `options_load(path)` reads `settings.txt` once at start (display.c:33). It appends a line with its comment for each missing option.
- `options_set_enum(id, value)` already writes one option back to the file. It rewrites the file line by line through `settings.txt.tmp` and a rename, keeps the player's comments, other lines and CRLF, and appends the line when it is missing. Only KIND_ENUM options are accepted today. Only `database` uses it (database.c:100).
- To write switches and `save_folder`, `options_set_enum` needs a general form, for example `options_set(id, value)` with the switch and text checks. About 15 lines.

### 3b. The 16 options: when each one is read

| Option | Read | Live change? |
|---|---|---|
| display | once at start: screen size (display.c:34), window and texture (user32.c win_video_init). Every area is scaled when it is made | **Restart.** Also a new memory layout (Q152) |
| database | at the next Start New Game (files.c:369) | Next new game. Already has a button on Setup Game |
| save_folder | first save or load, then cached (files.c:75, `static done`) | **Restart** (or reset the cache). Saves of the old folder do not show in the new one |
| sound | once, when the device opens (dsound.c:116, `device_tried`) | Off: small change (use the `muted` flag, dsound.c:64). On after a start with off: open the device then. Else restart |
| sharp_picture | texture scale mode at window create (user32.c:217), integer scale on resize (user32.c:90) | Needs a small re-apply function. Else next start |
| remember_window | at window open (user32.c:190) and at quit (user32.c:135) | Live for the save at quit |
| space_to_continue, shortcuts | each key (user32.c:333, 335) | Live |
| clickable_names | each draw and click (names.c:129, 320) | Live |
| attribute_colours, condition_colours | each label (columns.c:330, 434, 439) | Live, at the next draw |
| match_columns | each area create (columns.c:135) | Next build of a match screen |
| attribute_groups | each profile build (groups.c:133) | Next build of a profile |
| squad_views | each Assistant drop-down (assistant.c:110, 133), comparison (compare_screen.c:89) | Next drop-down |
| faster_days | each holiday step (holiday.c:62) | Live |
| save_backups | each save (files.c:216) | Live |

So 13 of 16 can change at once or at the next screen. `display` and `save_folder` need a restart. `sound` needs a restart unless we add a small change.

### 3c. How Team views was added (views.c, views.h)

- A new screen of the runtime on the original's screen stack. `views_open` pushes it with the original push 0x7fe600, flag 0 (as the club screen does at 0x453e2c).
- Its build and input functions are host functions at fake addresses (`rt_host_fn`). The main loop 0x7fc7e0 calls them as it calls the original's. Back, the side bar and the history work as on any screen.
- The frame calls the original's helpers through `rt_callback`: background 0x41c250, heading 0x803f30, side bar 0x75db00, tab bar 0x5ed780, Back/Next bar 0x5edcc0, area 0x55fba0, widget 0x55f990, label 0x804100.
- The entry point is a new item in an original drop-down (the Assistant drop-down, assistant.c:133).
- Node slots hold only numbers, so no leave function is needed.

An options screen can follow the same path. It would use the original's choice-row style (yellow framed label for the current value, buttons for the others), so it looks like Game Settings.

## 4. Ways to do it

### (a) New rows on the original's Game Settings box

- The box has 15 of 15 rows in use (0x823d44). 16 more rows do not fit. The box is 500 of 600 rows high.
- To fit, we would need a second page of the box (a "More" button next to Reset) or to grow the area and rebuild row layout in a wrap of 0x823950. The build is one 0x2300-byte function with fixed row numbers (`inc ebp` per row). Changing it means a wrap that rewrites argument values at ~45 call sites or a full C rewrite of the build.
- Changes would need to follow Ok/Cancel: the box applies only on Ok (0x8259a0). Our options would need the same copy-and-apply logic.
- Cost: high. Risk: high (wrap of a large original function, row and column numbers in three modes). Looks: native in all modes.

### (b) A new "Bosman Edition options" screen (or box)

- A runtime screen like Team views, pushed with 0x7fe600. Opened from:
  - the start screen: an eleventh menu item next to "Database" (database.c already adds item 9 to the menu builder 0x5ed300; the grid is 2 x 6, 10 cells are in use, so two are free. The 0.6.0 "Choose Season" button (Q145) may take one of them);
  - in a game: a new item "Bosman Edition Options" in the side bar "Game Options" drop-down (0x760254..0x760700), after "Game Settings".
- Content: one choice row per option, in the original's style (On / Off buttons, the current value as a yellow framed label). `display` as Classic / Widescreen / HD. `database` as on the start screen. `save_folder` shown as text, not editable (no text entry widget is in use on this box; the original has text entry, flags 0x08, inventory section 7, but a path is long).
- A change writes `settings.txt` at once through a general `options_set`. No Ok/Cancel needed, or Ok/Cancel as on Game Settings.
- Two forms: a full screen with side bar and Back/Next (as Team views; 16 rows fit in 13-row grids only with two columns or two tabs), or a box like Game Settings (flag 1 push, 500 high, room for 15 rows plus Ok).
- Cost: medium. About 300 to 400 lines of C (views.c is 311 lines for its frame). Risk: low to medium; the frame code of views.c is proven in classic, widescreen and HD (widescreen inventory, HD sweep).
- Looks: the area create wrap scales every area (display.c), so classic, widescreen and HD come out the same as Game Settings. A 16-row list at 800x600 needs rows of about 26 px in a 500 px box, the same as Game Settings today.

### (c) A tab on a Team views style page

- A full screen with the tab bar 0x5ed780: tabs "Display and window", "Screens", "Game", for example. Opened from the same two places as (b).
- Cost: medium, a little more than (b). Risk: low. Looks: like Team views, a full screen with the side bar. Better room for a line of help text under each option.

**Recommendation: (b) as a box**, because it looks like the Game Settings box that players know, it fits 16 rows, and it opens from both places with the code paths we already use (database.c, assistant.c, views.c).

## 5. Can it change game decisions?

- Opening any screen or box allocates memory (the Game Settings box allocates a 0x31b-byte copy and widgets). A new box changes later days in the same way, as Q152 says for any screen. It is the player's own click, so it is allowed. The same-save check must compare "open the box and close it" against the same steps in another run that also opens it, never against a run that does not.
- The options themselves: the same-save run `off` already shows that no switch option changes a decision. `display` changes the memory layout, as it always did. `database` changes only the next new game.
- A change that applies live (for example `faster_days` while on holiday) is a player action. `faster_days` is already proven to keep the save the same (`faster` run).
- Test plan:
  1. Unit: `ctest -R options` with new cases for `options_set` on switches, text, CRLF and a missing line.
  2. fixtures play start.sav, widescreen, classic and HD: open the box from the side bar, click each row, dump the box (pictures for review), close it. Check that settings.txt holds the new values and nothing else changed.
  3. Same-save: two runs with the same steps (open box, switch `faster_days` off and on, close, holiday 14 days). The saves must be byte for byte the same.
  4. Start screen: open the box, set `display = classic`, quit, start again: classic.

## 6. Open decisions for Max

1. **Where does the screen open from?** Recommended: both the start screen (a menu item "Bosman Edition Options" in a free cell of the Setup Game grid, leaving the other for "Choose Season") and the side bar "Game Options" drop-down, after "Game Settings". Reason: players look for options in those two places, and both entry paths exist in our code.
2. **Box or full screen?** Recommended: a box like "Game Settings". Reason: same look as the original's options, and 16 rows fit.
3. **Which options does it show?** Recommended: all 16. `save_folder` is shown read-only with a note "change it in settings.txt". `database` stays also on Setup Game. Reason: one place for every option. A long path needs a text entry, which the original's box has no example of.
4. **Options that need a restart (`display`, `save_folder`, `sound` unless changed):** Recommended: write the file at once and show "Applies at the next start" next to the row. Do not restart the game for the player. Reason: a live display change would rebuild every area and change the memory layout mid-game.
5. **Apply at once or on Ok?** Recommended: at once, with a Back or Close button. Reason: simpler, and the file is always current. Option: Ok/Cancel as on Game Settings, if Max wants the same feel.
6. **Does settings.txt stay documented?** Recommended: yes. Keep the comment lines in the file and the site line, and change the site text (site/index.html:299) to "Each option can be switched on and off in the game or in settings.txt." Reason: the file stays the store and power users edit it.
7. **The overlap with the original's Sound row:** Recommended: keep both, and name ours "All sound" with a help line. Reason: removing the original's row means a wrap of its build.
