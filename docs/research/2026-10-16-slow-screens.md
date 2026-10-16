# Research #49: faster transfer status and discipline screens

Scripts, dumps and logs: `scratchpad/r49/` (`x86d.py` disassembles the original, `applyp.py` applies Nick's patches to a copy, `run5/` holds the timing dumps).

Address rule, checked against the PE header: `.text` raw offset 0x1000 = RVA 0x1000, image base 0x400000, so memory = file offset + 0x400000. Nick's new section: file 0x6DC000 = memory 0xDE7000.

## 1. What the original does today

Each screen has an input function. The screen loop 0x7fc7e0 calls it once per pass with the widget index from the input poll 0x55f620 (-1 for nothing). The functions all start the same way: if the widget is not -1, read the widget's event code from the area table (0xb74c4c, +0xba966), else take the global code at 0xdd6bde, then switch on the code.

- Set Staff Transfer Status (build 0x8f6a10, input 0x8f7640, pointer stored at 0x8f6778). Case 2 is the Asking Price "-" (step down 0x43f580, at 0x8f7aae). Case 3 is "+" (step up 0x43f4e0, at 0x8f7c5c). After each step the code compares the price with 30,000,000 (0x01C9C380) at 0x8f7b92 and 0x8f7d40, and the screen build compares it at 0x8f6f66. Inference: this sets whether "+" can be pressed, so 30M is the top asking price that "+" reaches.
- Discipline Player (build 0x5a83e0, input 0x5a8990, pointer stored at 0x5a838a). Case 2 is "-" (0x5a8a3d: `dec edi` while above 1). The values run Official Warning, 1 Week Fine, 2 Week Fine.
- The original has no Sleep, GetTickCount or timeGetTime import (`docs/research/2026-10-14-assistant-and-days-inventory.md`). Its screen loop is a busy loop. While the left button is held, the hit test gives the same widget on every pass, so a held "+" steps once per pass (`src/recomp_rt/win/user32.c` comment above REPEAT_DELAY). On a 2001 PC a pass took about two screen draws. On a modern PC one click gives many steps.

So the problem in the original on a fast PC is that these screens step too fast, and the CPU runs at 100%. I found no loop or timer that makes them slow. I don't know where the word "faster" in the issue comes from. Inference: from the patch name.

## 2. What Nick's patches do

PatcherForm.cs lines 655 to 669: one check box, "Enable idle sensitivity" (tooltip: stops the CPU from using 100% when idle), applies all four patches together and removes all four together. It is disabled for Tapani's exe, which "implements it in a different way".

`idlesensitivity` (general):
- 0x966120 (file 0x566120, empty space at the end of `.text`): `Sleep(n)` through `GetModuleHandleA("KERNEL32")` (IAT 0x967138) and `GetProcAddress(.., "Sleep")` (IAT 0x9670b8, string at 0x96614c). 0x966180 is the same with the pointer cached at 0x9f189c.
- 0x966170: `push 0x60; call 0x966120; xor ebx, ebx; ret`. This is Sleep(96 ms) plus the `xor ebx, ebx` that the hook overwrote.
- 0x9472d4 (file 0x5472d4): the `call 0x94dac0` (64-bit divide by 10,000) at the end of `clock()` 0x947299 now calls 0x966152. That code counts down the byte 0x9870e6. Every 33rd `clock()` call it sleeps 20 ms (`push 0x14; call 0x966180`), then it jumps to the divide.
- 0x4e243a (file 0xe243a): the Offer Contract input 0x4e2280. The `movsx edx, dx` is dropped, and the `xor ebx, ebx` before the jump table at 0x4e2455 becomes `call 0x966170`. So every event with code 4 to 0x30 sleeps 96 ms.
- 0x9661b8 to 0x9662b2: code that adjusts player attribute bytes with the game random function 0x90cfc0. No byte of the four patches jumps to it. Inference: bytes of another patch that sit in the same list.

`idlesensitivitytransferscreen`: 0x8ec743 (file 0x4ec743, input function 0x8ec420) jumps to 0x96647b: `call 0x966170` (Sleep 96), then the two overwritten `movsx`, then back to 0x8ec749.

`idlesensitivitytransferstatusscreen`:
- 0x8f76fe (file 0x4f76fe): `mov eax, ecx; shl eax, 5` becomes `jmp 0xde8170`. At 0xDE8170 (file 0x6dd170): `call 0x966170` (Sleep 96), `mov eax, ecx; shl eax, 5`, `jmp 0x8f7703`. This runs on every pass in which a widget is hit (si != -1), so on each click and on each pass of a held button.
- 0x8f7b94 and 0x8f7d42 (operands of the two compares above): 30,000,000 becomes 60,000,000 (0x03938700). The build check at 0x8f6f66 keeps 30,000,000. Inference: this raises the top asking price of "+" to £60M. It is a game change, not a speed change.

`idlesensitivitydisciplinescreen`: 0x5a8a10 (file 0x1a8a10): `mov ecx, edx; shl ecx, 5` becomes `jmp 0xde8180`. At 0xDE8180 (file 0x6dd180): Sleep 96, the two moved instructions, `jmp 0x5a8a15`. Same place in the code as the transfer status hook.

In words: "idle sensitivity" adds sleeps to a game that never sleeps. The general part cuts CPU use (20 ms every 33 clock reads). The screen parts put a 96 ms sleep on each pass with a hit widget, so a held "+" or "-" steps about ten times a second. Each click on these screens then takes 96 ms longer. The patch makes the screens slower to respond, and usable on a fast PC.

## 3. What our recompile already has

- `src/recomp_rt/win/user32.c`, `RT_WRAP(0055f620, input_poll)`: a held button gives the same widget again only 400 ms after the press (REPEAT_DELAY), then every 100 ms (REPEAT_INTERVAL). The first result of a press goes through at once. The same pacing for scrollbars (`RT_WRAP(00561f10, scrollbar_test)`). This covers every screen, the two screens of #49 among them. It is the same effect as Nick's three screen patches, without the 96 ms on a single click.
- No option for idle sensitivity exists. No `Sleep` import is replaced, because the original has none.
- Idle: `PeekMessageA` calls `win_poll_events`, which returns at once when the last poll is less than 2 ms old. It does not sleep. Only `GetMessageA` and the two repeat waits call `SDL_Delay(1)`.

## 4. Measurements (build/cm0102_recomp of 2026-10-16, classic, start.sav, Seaman)

Steps in `r49/steps5.txt`, run through `tests/fixtures/fixtures.py play` with `RECOMP_SCREEN_LOG=1`. Load average was 5.7 (other agents). The numbers were even, so I did not rerun alone.

| Action | Time to result |
|---|---|
| Click squad name to profile screen | 34 ms |
| Click Action menu to popup | 31 ms |
| Click Set Transfer Status to screen | 34 ms |
| Click Discipline Player to screen | 32 ms |
| Click "+" Asking Price to new value drawn | under 10 ms (first dump) |
| Click "+" Disciplinary action to new value drawn | under 10 ms |

- One click gives one step on both screens (£1.8M to £1.9M, Official Warning to 1 Week Fine). "-" goes back one step.
- "+" held for 2 s: £1.9M to £3.6M, 17 steps (one at the press, then about 16 at 100 ms). Discipline held: stops at 2 Week Fine, "+" greys.
- Idle on the transfer status screen for 25 s: the game process uses 98 to 100% of one core (`ps`, SDL dummy video driver).

So the two screens are not slow in our build. They open as fast as other screens and redraw within 10 ms of a click. The only open cost is CPU use when idle, on every screen.

## 5. Hook points and cost

- #49 as written: nothing to build. Our repeat pacing already does what Nick's screen patches do, and a single click is faster than with his patch.
- Do not copy the 30M to 60M change. It is a game change (see 6).
- If CPU use when idle is wanted (new issue, not #49): in `win_poll_events` or `PeekMessageA` (user32.c), when the queue is empty and no input came for some ms, call `SDL_Delay(1)`. About 10 lines. It must not run while a match, holiday or Continue Game waits on the clock (commentary 0x90c510, holiday hold 0x76224c). Inference: a 1 ms delay on an empty queue only stretches passes, which those waits already measure in real time.

## 6. Can it change game decisions?

- Repeat pacing (exists): no. It changes only how many passes see the held widget. The same clicks give the same steps.
- A sleep in the screen input path: inference, no. Sleep touches no game memory and no random number. It changes only the count of passes and the real time between them. On these two screens no game time passes, and the same-save runs (`smoke_run.py --same-save`) already match at different machine speeds.
- The 30M cap: yes. With the same clicks of "+" at £30M, the price goes higher, so AI bids differ.
- An idle delay in the pump: it changes pass counts during matches and holidays, where the original's results already depend on real time (Q99). Test it by repeatability, not against the old build.

Test plan if anything is built: `smoke_run.py --same-save` and `--match` with the option on and off; `r49/steps5.txt` (one click = one step, under 50 ms); a 25 s idle CPU sample on the transfer status screen.

## 7. Open decisions for Max

1. Close #49 as already done in 0.1.0 (held-button pacing), with these numbers? Recommended: yes. Reason: no slowness exists in our build, and Nick's patch makes a single click slower.
2. Raise the "+" cap of the asking price from £30M to £60M, as Nick's patch does? Recommended: no. Reason: it changes game decisions with the same clicks, and it is not what #49 asks.
3. Open a new issue for CPU use when idle (100% of one core today)? Recommended: yes, always on with no option, for a later release. Reason: laptop battery and fan noise. It needs the same-save and match checks.
