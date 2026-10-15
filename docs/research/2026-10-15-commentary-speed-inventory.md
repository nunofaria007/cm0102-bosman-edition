# Commentary speed inventory (#31)

Research for feature #31: one commentary speed step twice as fast as "Very Fast", with no change to a match result. Source: `clean-3968/cm0102.exe` (objdump disassembly and REA). Every fact has the address that shows it. Inferences have the mark **[inference]**.

## 1. Where the speed is stored

### 1a. The human manager record (the saved value)

- The speed is a 16-bit word at offset **+0x9ad2** of the human manager record. The records are an array at `[[0xb63c98]]`, 42,735 (0xa6ef) bytes apart. The index is `*human - [0xae2364] + 0x10`.
- Getter **0x5fcff0**: thiscall on `[0xb63c98]`, one argument (human pointer, 0 = the human of the current player slot `[0xb74c26 + slot*0x300]`), `ret 4`, returns the word in ax (0x5fd044..0x5fd07f).
- Setter **0x5fd100**: thiscall on `[0xb63c98]`, arguments (s16 speed, human pointer), `ret 8`, writes the word at 0x5fd18a.
- Default: 0x7d (Slow) at 0x5fbbac, in the record init 0x5fb810.
- Saved game: the save writer 0x5ff9e0 writes 2 bytes from +0x9ad2 at 0x5ffc74 (through 0x932e10). The loader 0x5fee50 reads 2 bytes into +0x9ad2 at 0x5ff2c2 (through the stream reader 0x932c30). So the speed is part of the saved game.

### 1b. The match object (the value the commentary uses)

- A byte at **match + 0x2db0**. The match object is `[ctx + 0x8f7a]`, where ctx is `[game + 0x57]` (0x6b55f5, 0x6b5669). The match object constructor 0x6d5bf0 writes 0x7d there (0x6d5c21).
- The match screen build 0x718e90 copies the human value into the byte at each build: 0x719c38..0x719c4e (`call 0x5fcff0(0)`, `mov [esi+0x2db0], al`).

### 1c. Values per item

The popup builder pushes the value as the item data (argument 16 of the widget create 0x55f990) and compares the getter result with it to mark the current item (style 0x2002 for the current item, 0x2 for the others).

| Item | Text string | Item create call | Value | Current-item compare |
|---|---|---|---|---|
| Very Fast | 0xa16408 | 0x719861 | 0x14 (20) | 0x7197d3 |
| Fast | 0xa16400 | 0x71991c | 0x37 (55) | 0x71988e |
| Medium | 0xa163cc | 0x7199dc | 0x5a (90) | 0x71994e |
| Slow | 0xa163c4 | 0x719a95 | 0x7d (125) | 0x719a07 |
| Very Slow | 0xa163b8 | 0x719b51 | 0x96 (150) | 0x719ac3 |

All five items use action id **0xc** (the push before each text push, for example 0x7197d1). Their parent is the popup area that 0x55fba0 creates at 0x7197a8 (kept in esi). The area hangs from the "Commentary\nSpeed" button that 0x55f990 creates at 0x719775. The value is a percentage of the line delay (section 2).

### 1d. The Space key cycle

After the items, 0x719b60..0x719c31 looks up the current value in the byte table 0x71a190 (index value - 0x14, 131 entries) and the jump table 0x71a178. It writes a widget index to Display +0x12f4ff and 0x20 (Space) to Display +0x12f4fe. The input poll 0x55f620 returns +0x12f4ff when the player presses that key (docs/research/2026-10-13-input-inventory.md line 93). The mapping:

| Current value | Widget for Space |
|---|---|
| 150 | Slow (edi) |
| 125 | Medium (ebx) |
| 90 | Fast |
| 55 | Very Fast |
| 20 | Very Slow (esi) |
| any other value | Very Slow (esi, the default 0x719c17) |

**[inference]** Space steps the speed one item faster and wraps from Very Fast to Very Slow.

### 1e. The left-bar label

0x760cb1 in 0x75db00 only makes a text label "Commentary\nSpeed" with the label create 0x804100. It does not read the speed.

## 2. The readers of the speed, and the wait

### Readers of the human record word (+0x9ad2)

| Address | Function | Use |
|---|---|---|
| 0x7197c9 .. 0x719b60 | 0x718e90 (match screen build) | Marks the current item and sets the Space key target. |
| 0x719c49 | 0x718e90 | Copies the word into match + 0x2db0. |
| 0x71a3f3, 0x71a41d | 0x71a240 (match screen input) | Compares and sets (section 2a). |
| 0x6b459f .. 0x6b46b1, 0x6b476d | 0x6b4360 (plays the matches) | Takes the largest value of the humans at one match and writes it to all of them. **[inference]** This keeps one speed for a hot-seat or network match. |
| 0x5ff2c2, 0x5ffc74 | load 0x5fee50, save 0x5ff9e0 | Saved game. |

### Readers of the match byte (+0x2db0)

| Address | Function | Use |
|---|---|---|
| 0x6d8bc7 | 0x6d7b00 (show a commentary line) | **The line delay.** `delay_ms = line_delay * speed / 100` (imul at 0x6d8bd5, the /100 at 0x6d8bd8..0x6d8be7). It goes to match + 0x2dac (0x6d8bec). The start time goes to match + 0x2d6c + 4*slot (0x6d8b9c..0x6d8ba4). `line_delay` is the s16 at +2 of the event entry (0x6d8bd0). **[inference]** That is the "Delay in milliseconds" of `Data/events_<lang>.cfg`. |
| 0x6d8ace | 0x6d7b00 | Plays the line's sound only when speed >= 0x65 (101), that is Slow and Very Slow. |
| 0x6b5686 | 0x6b55c0 (commentary step) | When speed == 0x14 (Very Fast), skips the extra pause `[state+0x109]`. For any other value it waits that pause after the line start `[state+0x10d]`. The pause is set at 0x6b57e9..0x6b58c2 (for example `0x226 - 50 * x` at 0x6b58a8..0x6b58c2) after special events. |
| 0x71a3d5, 0x71a3ed, 0x71a40e | 0x71a240 | Input (section 2a). |

No other instruction in the program uses the offset +0x2db0 (search of the whole disassembly).

### The clock and the wait

- The clock is **0x90c510** → 0x947299: GetSystemTimeAsFileTime divided by 10,000, so wall-clock milliseconds (the import at 0x9472a3, the divide 0x94dac0 by 0x2710). No GetTickCount or timeGetTime.
- **0x6d8c70** (thiscall on the match, one argument: the "seen" flags of the 16 slots) returns 1 when every slot passed `start + delay` (0x6d8c91..0x6d8ca8) and no wait sound plays (0x8029f0 at 0x6d8cc0). It is the only caller-side hold of a line. Called once, from 0x6b5673.
- **0x6b55c0** (the commentary step, thiscall, one argument: the human game record; callers 0x71a4fd in the match input 0x71a240 and 0x5d61bc in 0x5d5f20): when the line is not done, it calls 0x6b5ce0(0) and returns 0 (0x6b5982). When the line and the extra pause are done, it takes the next line (0x6d78b0, 0x6b575f). When no line is left, it runs one step of the match engine 0x6b98c0 (0x6b5736).

### Delay per speed (for the most common line delay, 900 ms, 2,118 lines of events_eng.cfg)

| Item | Factor | 900 ms line | Extra pause |
|---|---|---|---|
| Very Fast | 20 % | 180 ms | skipped |
| Fast | 55 % | 495 ms | yes |
| Medium | 90 % | 810 ms | yes |
| Slow | 125 % | 1125 ms, plus sound | yes |
| Very Slow | 150 % | 1350 ms, plus sound | yes |
| new value 10 | 10 % | 90 ms | yes, unless the runtime skips it (section 4) |

### 2a. The input

The match screen input 0x71a240 handles every item with id 0xc in one place: 0x71a394 (`cmp ax, 0xc`) reads the item data (widget + 0x40, 0x71a3c0) and writes it to match + 0x2db0 (0x71a3d5). When the byte differs from the human word, it calls the setter 0x5fd100 (0x71a41d) and returns -11 (0x71a425), so the screen builds again. Any data value passes through. A new item with id 0xc and data 10 needs no new input code.

## 3. Does the speed reach the match result?

What the code shows:

- The speed is not an argument of the engine step. 0x6b55c0 calls 0x6b98c0 with the constant 1 (0x6b572c) and only when no commentary line is left. The number of engine steps per line does not depend on the speed.
- The direct call tree of 0x6b55c0 (1,045 functions) does not reach 0x6d7b00 or 0x5fcff0. Indirect calls are not in that search.
- The speed goes into the saved game (section 1a). It is a display option, not match data.

What the code also shows, and what it means:

- While a line waits, 0x6b55c0 calls **0x6b5ce0(0)** (0x6b5982). That function runs one engine step 0x6b98c0 of the current background match `[mgr+0x2f]` (0x6b5dcc..0x6b5dce). When that match ends, 0x6b5a70 starts the next one that has no human club (0x6b5b62..0x6b5b92, 0x600a50 checks). Two option flags at `[0xae3138]+4` (0x40000, 0x80000) make it skip two calls in three or five in six (0x6b5d30..0x6b5d67).
- The engine step 0x6b98c0 calls the game random function **0x90cfc0** directly. That function uses one global table offset (0xde1f60, docs/research/2026-10-14-start-screen-helpers.md).
- **[inference]** So the number of background steps between two steps of the human match is the number of polls while a line waits. That depends on the wall-clock delay, so on the speed and on the frame rate. The human match then takes other values from the shared random table. In the original, a match result already depends on the commentary speed and on the machine speed. The same speed on two runs can also give two results if the frame times differ.
- Consequence for #31: the port cannot prove "no change to the result" by a comparison with Very Fast. A same-save check over a played match is only stable if the number of polls per line is the same on both runs. Whether the test harness gives that is unknown.

Unknown:

- Whether background matches finish before the first line of the human match in a normal day. If they do, the interleave is short, but it still exists at the start.
- The meaning of the option flags 0x40000 and 0x80000.
- Indirect calls from the engine that read the speed. The search found no other use of +0x2db0, so this is unlikely.

## 4. Design for the runtime

### 4a. The sixth item "  Fastest", value 10

The factor formula at 0x6d8bc7 is a percentage, so the value 10 gives half the Very Fast delay with no change to the delay code. The input 0x71a240 stores any value of an id 0xc item, so no input wrap is needed.

1. Popup item. Add a call site `speed_very_fast: (0x719861, 0x55F990)` in `tools/recomp/callsites.py`. In the existing wrap `RT_WRAP(0055f990, widget_create)` in `src/recomp_rt/win/display.c`, when the return address is `rt_site_speed_very_fast`, first create one more item with the same 18 arguments. Change only argument 13 (text, `rt_alloc_str("  Fastest")`), argument 16 (data, 10) and argument 0 (0x2002 when the human word is 10, else 2). Then call `o_0055f990()` for Very Fast as usual. The new item comes first, so it shows above Very Fast. Copy the item pattern from `src/recomp_rt/win/assistant.c` (the Filter-style items with `call_this(WIDGET_CREATE, display, 18, args)`).
   This is the same as the Setup Game menu wrap in `src/recomp_rt/win/database.c` (call site `setup_menu`, 0x81bcc5): test the return address of one call, change the arguments, then call the original.
   **[inference]** The popup area 0x55fba0 is made with an empty rectangle (0x719797..0x7197a6), so it takes its size from its items. Check this on screen.
2. Space key (optional). The value 10 falls to the default case, so Space goes from Fastest to Very Slow without a change. For Very Fast the original goes to Very Slow. To put Fastest in the cycle, add a call site `speed_copy: (0x719C49, 0x5FCFF0)`. That getter call comes right after the Space key write (0x719c31). In a wrap of 0x5fcff0, at that site, when the value is 20, write the Fastest widget index to Display +0x12f4ff.
3. The extra pause. 0x6b5686 skips it only for 20. Wrap **0x6b55c0** (thiscall, one argument, `ret 4`). When the argument is not 0 and `[[arg+0x57]+0x8f7a] + 0x2db0` is 10, write 20 there, call `o_006b55c0()`, then write 10 back. Nothing in the direct call tree of 0x6b55c0 reads the byte except 0x6b5686 (section 3), so only the pause changes.
   Alternative: a translate-time patch of the compare at 0x6b5686, as `tools/recomp/widescreen.py` does for constants. That patch format only swaps an immediate value, so it cannot express "20 or 10" cleanly.

### 4b. What does not need a change

- The delay function 0x6d7b00 and the wait 0x6d8c70.
- The input 0x71a240.
- The setter, the getter, the save and the load. The word 10 saves and loads as any other value. **[inference]** The original exe can load such a save: it shows no ticked item and runs at 10 %.
- The sound gate at 0x6d8ace (10 < 101, no sound, as for Very Fast).

### 4c. The decision for Max

The rule "the match result must not change" does not hold in the original between speeds (section 3, inference). Choices:

1. Accept the original behaviour. Fastest changes the result only in the way any other speed change does. The same-save check over a played match then needs the same speed and the same polls per line on both runs.
2. Make the background steps per line fixed in the runtime, for every speed. That makes results the same at every speed, but it changes the original behaviour and needs its own research.
