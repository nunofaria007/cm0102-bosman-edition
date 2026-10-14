# Columns, colours and sound inventory (0.2.0, slice 5 part C)

Research for issues #19 (attribute colours), #23 (position, condition and match rating together) and #21 (sound). Original: `clean-3968/cm0102.exe` 3.9.68. Every address is in the original. "Code" marks a fact that the code or a run shows. "Inference" marks a conclusion that I did not prove.

## How the facts were found

- Static: REA (Ghidra) on the original. Ghidra has no function at 0x718aa0, 0x71b4a0, 0x722ed0 and 0x723350, so for those I read the instruction comments in the recompiler output (`build/recomp/gen/fn_041.c`).
- Run: `build/cm0102_recomp` with `RECOMP_SCRIPT`, `RECOMP_USER_DIR` (a new temp folder), the dummy video and audio drivers, under lldb. A Python breakpoint on `f_0055f990` (widget create, the runtime wrap) logged each widget with its text, text colour, column, row and the return addresses on the guest stack. This gives the call site of every drawn value.
- Reaching a match: select the squad with Filter "Available Only" (an injured player blocks the match), then Continue Game in a loop. Bids for players stop the days. The loop answers them with Respond, Reject and Yes. The world is random, so the day of the first friendly changes from run to run.

## Shared facts

- `0x804100` is the label create, thiscall on the screen stack 0xb74450, `(area, column, row, style, colour, text_flags, font, text_colour, text)`, ret 0x24. It calls widget create `0x55f990` and returns at 0x804158. `0x8040a0` is the button version (two more arguments, id and data). Code: trace and the screen-stack inventory.
- `0x7ff010(n)` reads slot n (0..59) of the current screen node, `0x7ff270(n, value, 0)` writes it. Screens keep their state in these slots. Code: 0x7ff010 body (`node + 0x14 + n*8`).
- Colour globals (RGB565) seen in this work: `0xaebdec` yellow (0xffe0), `0xaebde8` light cyan (0x87ff), `0xaebdd4` orange (0xfc00), `0xae3184` light grey (0xe71c), `0xae317e` grey, `0xaebdea` black, `0xae31a8` red. Code: trace values.

## 1. Attribute numbers (#19)

### Player profile, Profile tab

- Builder: `0x87a510`. It loops over a table of attribute cells, three columns of names and values (area 28).
- The name label: call at 0x87aa09 (returns 0x87aa0e), text colour `[0xae3184]`.
- The value: one shared label call at **0x87ac15** (returns **0x87ac1a**) for every value cell. The branches before it choose text and colour by the value byte at `[esp+0x14]`. Code: 0x87aa12..0x87ac15.
  - value >= 0 (an attribute 1..20): `sprintf("%d")` (format 0x98806c) at 0x87abde, text colour `[0xaebdec]` yellow, loaded at **0x87abf1**.
  - 0xfc: condition, `0x62c590(player, 1)` printed as `"%d%%"` (0x98e8d4), or a fixed string (0xa86304) when `0x62c400(player) == 2`. Text colour `[0xaebdd4]` orange. The run shows "88%".
  - 0xfd: form (0x4cc2e0), 0xfe: preferred foot or a fixed text, 0xff: morale words (thresholds on player +0x30, +0x3b), 0xf9: "  -". Colours `[0xaebdd4]` or `[0xaebdec]`.
- Font 2, text flags 0xc, area 28. Code: trace line `col=1 row=0 ... tcol=ffe0 text='2' area=28 stk=87ac1a`.

### Squad screen lists (View: Attributes and others)

- Builder: `0x456900` (25 KB, one function for the whole squad list). Two halves side by side (columns 3 and 7 at 0x45c013 `add ebx, 4`).
- Attribute cells: label call at **0x45c7ca** (returns 0x45c7cf). Value from `0x5431e0(player, attribute index)` when `0x543280(...)` says it is known (attribute masking), else the text at 0x98ba0c. Text colour `[0xaebde8]` light cyan (0x45c781, 0x45c7a5). Code: trace `tcol=87ff text='18' stk=45c7cf`.
- Other single-value columns of the same list (Position(s), Condition, Contract and so on): one shared label call at 0x45c00e (returns 0x45c013), text colour `[0xaebdec]`.

### Other screens that read attributes

`0x5431e0` (attribute getter) has these callers: 0x456900 (squad list), 0x475520, 0x47c180, 0x4d1600, 0x6ddab0, 0x72c1f0, 0x807450, 0x808690, 0x8b37c0. 0x475520 and 0x47c180 also reference " Condition" (0x4763ea, 0x47d0de), so they are other squad-style lists (inference: other club, reserves or shortlist). Which of the rest draw and which only compute is unknown. The profile does not call 0x5431e0. Staff profiles and "Compare two chosen players" were not traced (unknown).

### One place to choose the colour

Yes, without a change to the game code: wrap the label create **0x804100** in the runtime (`RT_WRAP(00804100, label_create)`, the same way `display.c` wraps 0x55f990). In the wrap, read the return address at `[esp]`. When it is 0x87ac1a (profile) or 0x45c7cf (squad lists) and the text (argument 8) is a number 1..20, replace the text colour (argument 7) with the band colour, then call `o_00804100`. Nothing else changes. The text, the layout and the game state stay the same, so the same-save check cannot fail on this.

Because the profile shares one call for all value kinds, the number test is the guard: "88%", "Ok", "Right Only" and "  -" keep their colour.

## 2. Position, condition and match rating (#23)

### Tactics screen (outside and inside a match)

- Builder: **`0x899ea0`** (assert file `cm3\code\ta...`, string 0xa8b94c). It is the same screen outside a match (Squad > Tactics) and in a match (left bar "<club> Tactics"). Code: trace `ret=89a17b` for the "Arsenal Tactics" heading, screenshots.
- The chosen view is **node slot 10**: `0x7ff010(10)` at 0x89a04d into `local_929` (pseudocode line 375). The View menu items carry id 0xd and data = the view number (0x89add7 and the following `0x55f990` calls, argument +0x48). A tick (flags 0x2002) marks the item whose data equals the slot. Views: 1 Position(s), 2 Condition, 3 Goals, 4 Form, 5 Average Rating (3..5 only outside a match), 6 Match Rating (only in a match: the test `[edx+4] != 0` at 0x89ae94 skips 3..5), 7.. the instructions (Passing 0xa8bf4c, Tackling, Pressing, Pass To, Set Pieces (A), Set Pieces (D), Free Role, Forward Runs, Run With Ball, Hold Up Ball, Long Shots, Try Through Balls, Cross Ball, Marking, Marker). In the run the match opened with Match Rating ticked.
- The player list area: `0x55fba0(0x6e, 0x91, 0x1b8, 0x221, 9 columns, weights, 0x14 rows, ...)`, that is x 110..440, y 145..545 (pseudocode lines after `LAB_0089c819`). Column weights are bytes set at 0x899ff4..0x89a01c: 14, 1, 5, 1, 60, 15, 16, 7, 7 (sum 126). Before the area is made, the weights of unused columns are merged by view: views 1, 3, 4, 5 merge 7 and 8 into 6, view 2 merges 8 into 7, view 6 keeps all, the instruction views merge 6..8 into 5.
- Row loop: player `0x8ab480(row, 0)`. Cells by view (pseudocode, block "if (local_929 == '\x01')"):
  - view 1: column 6, text `0x53ee50(player+0x61)` (positions string), colour `[0xaebdec]` or grey when not on the pitch (`0x8aa9d0`).
  - view 2: column 6 status text `0x8aadb0` ("capt", "Inj"), column 7 condition `0x8aafd0(row, 0)` as `"%d%%"` (0x98e8d4), "Inj" (0x98e98c) when below 1.
  - view 6: column 6 status, column 7 match rating `0x8ab150(row, 0)` `"%d"` font 2 colour `[0xaebde8]` light cyan, column 8 goals `0x8ab260(row, 0)` `"%d"` yellow, only when above 0. The run shows the cyan "6" ratings.
  All these cells use label calls into one shared site (`LAB_0089e14d`) except the rating at column 7.
- Data during a match: `0x8aafd0`, `0x8ab150`, `0x8ab260` read a 0x1be-byte record per player at `match + 0x4796 + (row + team*20) * 0x1be` (match = tactics object +4, team = byte +0xdea), or a local copy at `this + 0x2735`. Condition = float +0x4d times 0.01 (double 0x967898), rating = byte +0x1b, goals = byte +0xc, 0xfb at +0x19 means no player. Outside a match condition comes from `0x62c590(player, 1)`. These getters only read (code: 0x8aafd0 assembly). They are safe for display.

### Three columns side by side

The data exist and the getters are safe, so this is a layout change only.

Recommended: in a match, with the option on and view 6 chosen, the runtime makes the area with more columns and adds two labels per row.

1. Area: the existing `area_create` wrap (0x55fba0) sees the return address of the list area calls in 0x899ea0 (the two `0x55fba0(... 9, &local_8f4 ...)` calls after `LAB_0089c819`). For those it passes 11 columns with new weights, for example 14, 1, 5, 1, 44, 30 (position), 14 (condition), 14 (status), 7 (rating), 7 (goals) plus the old column 5. The weights array must live in flat memory owned by the runtime.
2. Labels: a wrap of 0x804100 moves column numbers of labels made by 0x899ea0 (return address in 0x899ea0..0x89f6f0) to the new layout, and after each rating label adds the position label (`0x53ee50` text) and the condition label (`0x8aafd0` text) through `rt_callback(0x804100, ...)` with ECX = 0xb74450.
3. Inference: the tactics list is rebuilt when the screen redraws (the builder reads slot 10 each time). Whether a separate updater refreshes the cells by widget index, as the Player Ratings view does, is unknown. Check this first in the build slice: if an updater uses fixed widget strides, add the new labels after the loop instead of between cells.

Fit: in widescreen the runtime scales the area by 1.6 (528 px wide), so three extra columns fit with room. In classic the area is 330 px. Number 37 px, boxes about 18 px, then about 275 px for name, position ("D/DM RLC" is about 70 px in font 2), condition ("100%" about 32 px), status, rating and goals. It fits only if names lose about 40 px ("Kolo Abib Touré" is about 110 px). Inference from the screenshot widths. Recommendation: widescreen only first, as the spec allows, and decide classic on a screenshot.

### Match screen, Player Ratings view

- Match screen builder: 0x718aa0 (`match_screens.cpp`, 0xa161d4). The bottom and top tab rows: Match Overview, Match Stats, Action Zones, Match Report, "<team> Stats", Player Ratings (0x719d36), Latest Scores, the other team's Stats. Menu data: Player Ratings = 7 (0x719d4b `mov edi, 7`).
- Panel dispatch at 0x719f12: `switch (view - 1)` through the table at 0x71a214. View 7 calls **`0x722ed0`** (Player Ratings). Others: 1 0x71c270, 2 0x71e040, 3 0x71f500, 4 0x721000, 6 and 9 0x7217a0(team), 8 0x723d20, 10 0x7280e0. Inference: the view is node slot 1 (0x71a735 and 0x71a787 write slot 1 with 3 and 5).
- Layout, `0x722ed0`: one area x 110..780, 16 columns (8 per team), 20 rows, weights per team 14, 1, 5, 1, 70, 21, 7, 7 (0x722ed8..0x722f2b). Per row seven widgets: number (column 0), two boxes (column 2, cards), name (column 4, a button with id 0x7d7 or 0x7dc that opens the profile, 0x723266), event (column 5), rating (column 6, font 2, light cyan), goals (column 7, yellow). The texts start empty.
- Values: `0x723350` fills them by widget index. Base index = node slot 0xd, stride 7 widgets per row, 20 rows, 2 teams (0x723844, 0x723851). Record = `match_summary + 0x190 + team*0x625 + row*0x49` (match_summary = slot 0 object +0x5f, or slot 0x20). Rating byte +0x35 printed `"%d"` into widget +4 (0x723693), goals +0x21 into widget +5 only when above 0 (0x7236e0), events "inj/off/sub/on <minute>" (0xa16a68..0xa16a2c) into widget +3. Name colour by widget +2 (0x7237d7).
- Condition is not in this record. It is in the match engine records above (float +0x4d). Inference: the row order of the two record sets is the same (both are 20 per team in team-sheet order), but this is not proved. An alternative source is `0x62c590(player, 1)`, but during a match the tactics screen does not use it, so it may hold the value from before the match (inference).
- Recommended: wrap `0x722ed0`. After the original returns, create 40 condition labels (one per row and team) in the same area, in a new column made by the area wrap (the area call returns at 0x722fa8; give 18 columns, weights 14, 1, 5, 1, 58, 21, 12, 7, 7 per team). Keep them out of the stride-7 widget stream, because 0x723350 addresses widgets by index. Wrap `0x723350` too and refresh the condition texts with the set-text call 0x801c70 after it runs. Width: classic half is 335 px. Name 70 to 58 loses about 30 px ("Giovanni Van Bronckhorst" will be cut). Widescreen has room.

### How the game shows condition

A percentage everywhere it was seen: `"%d%%"` (0x98e8d4) on the profile (orange, "88%"), the squad list Condition view (0x45bbe5) and the tactics Condition view. The value is a short from `0x62c590(player, 1)` outside a match, and float +0x4d times 0.01 inside a match. "Inj" (0x98e98c) replaces it for an injured player.

## 3. Sound (#21)

### DirectSound calls

- `DirectSoundCreate` (dsound ordinal 1) from the sound object constructor 0x84ca80, then `SetCooperativeLevel(hwnd, 1)`.
- `CreateSoundBuffer` with flags 0xe2 (static, frequency, pan, volume) and the WAV format, one buffer per play (0x84cd10). `Lock(0, size)`, one `fread` into it, `Unlock`.
- `SetPan` (-5000, 5000 or 0 for pan argument 0, 1, 2), `SetVolume(10000)` then `SetVolume((percent-100)*100)` by 0x84ccc0, `Play(0, 0, loop)`, `GetStatus` (bit mask 5, 0x84cca8), `Stop`, `SetCurrentPosition(0)`, `Release`. Ten slots, the 11th play is dropped (0x84cbc0). Code: 0x84ca80..0x84cf40 and the import inventory 2.2. The stub in `src/recomp_rt/win/dsound.c` covers exactly these methods.

### When the game plays sounds

- The sound object exists only when the game's Sound option is on (preference flag 0x80, 0x5d7c60). Sound Off (flag 0x100) stops all and leaves `DAT_00dd7eec` null. So the game's own setting already gates every call (code: 0x8025e0 tests `DAT_00dd7eec`).
- Intro: `CM3_SOUNDS\intro1.wav` and `intro2.wav` from the main loop (0x5ccdf8, 0x5cce54).
- Match start: `fxloop2.wav` (crowd), centre, looped (0x6d5bf0).
- Match events: each commentary line of `Data/events_<lang>.cfg` has `> probability, delay in ms, sound file`. The commentary function 0x6d7b00 plays it through 0x8025e0 (scrman play, single player branch) with path from `0x90bde0("CM3_SOUNDS", name)`.
- Wait on sound: 0x6d8c70 holds the next commentary line while a "wait" sound plays (0x8029f0, GetStatus) or its delay runs. With real sound the commentary waits for the sound. Inference: this changes the pace on screen, not the match result. The same-save check must run with the dummy audio driver to prove it.
- 0x564080 also plays sounds, but only for network messages (network play, out of scope).

### Sound files

`clean-3968/Sounds`: 135 WAV files, all PCM 16-bit: 99 mono 22050 Hz, 18 mono 44100 Hz, 9 stereo 44100 Hz, 9 stereo 22050 Hz. `events_eng.cfg` names 131 files. Nine of them are missing in the clean files (cheating, closhot, cornurg, manu, oohahh, sexyfoot, smluuh, tinyurg, winkref): the loader then returns 0 and nothing plays (0x84cd10 checks the `fopen`). Thirteen files are not in the event file (intro1..3, fxloop2 and others). The loader reads a fixed 44-byte header, so files with extra chunks would read wrong data (inference: all 135 files have the plain header, the format scan found "fmt " at the same place). How `0x90bde0` maps "CM3_SOUNDS" to the `Sounds` folder is unknown. Check it with `RECOMP_TRACE` on the file opens.

### Recommendation

Keep the DirectSound shape in `dsound.c` and add a small mixer:

- One SDL audio device, 44100 Hz stereo S16, opened at `DirectSoundCreate`. If it fails, still return success and stay silent (a failure shows the original's error box).
- Each buffer keeps its WAVEFORMATEX (desc +0x10). At `Unlock`, convert the PCM once to the device format with `SDL_AudioCVT` or `SDL_AudioStream`.
- `Play` sets playing and loop, `Stop` clears, `SetCurrentPosition` sets the position, `SetVolume` maps hundredths of dB to a gain (`10^(v/2000)`), `SetPan` attenuates one side the same way. `GetStatus` returns 1 when playing and 5 when looping. The callback mixes up to ten voices, with `SDL_LockAudioDevice` around changes.
- A settings line for sound off can open no device. The game's own Sound On/Off works unchanged.
- Estimate: 1 session. Risk: the commentary pace changes because of the wait on sound (this is the original behaviour).

## 4. Recommended designs, risks and estimates

| Issue | Design | Addresses | Risks | Estimate |
|---|---|---|---|---|
| #19 attribute colours | `RT_WRAP(00804100)`: by return address 0x87ac1a and 0x45c7cf and a text of 1..20, set the text colour. Add the other list call sites after a trace of 0x475520 and 0x47c180. | 0x804100, 0x87ac15, 0x87abf1, 0x45c7ca | Return addresses must be in a checked list (like `widescreen.py`), so a changed build stops. Masked attributes stay as they are. | 1 session |
| #23 tactics, three columns | Area wrap by return address, an 0x804100 wrap that moves columns and adds position and condition labels, only in a match, view 6, widescreen. | 0x899ea0, slot 10, area after `LAB_0089c819`, weights 0x899ff4, getters 0x53ee50, 0x8aafd0, 0x8ab150 | The refresh path of the list is unknown. Clicks on rows use the area and must still hit. Classic fit is tight. | 1.5 sessions |
| #23 Player Ratings, condition | Wrap 0x722ed0 (extra column and 40 labels after the build) and 0x723350 (refresh). | 0x722ed0, 0x722fa3, 0x723350, slot 0xd, stride 7 | Row to engine record mapping is an inference. The stride-7 index must not move. | 1 session |
| #21 sound | SDL mixer behind the DirectSound stub. | 0x84ca80..0x84cf40, dsound.c | Pace of commentary, missing files, path of CM3_SOUNDS. | 1 session |

No design here writes game data, so none of them changes a match result or a decision. The same-save check guards this.

## Screenshots (before, 800x600, classic)

Outside the repo, in the session scratch folder `cols-shots/`: `before-profile.png`, `before-squad-attributes.png`, `before-tactics-view-menu-no-match.png`, `before-match-tactics.png`, `before-match-tactics-view-menu.png`, `before-player-ratings.png`. Widescreen screenshots were not taken.

## Unknown

- The refresh path of the tactics list cells during a match.
- The row mapping between the Player Ratings records (0x49 bytes) and the match engine records (0x1be bytes).
- How `0x90bde0` resolves "CM3_SOUNDS".
- Which of 0x4d1600, 0x6ddab0, 0x72c1f0, 0x807450, 0x808690, 0x8b37c0 draw attributes. Staff profile and the player comparison screen.
