# Input inventory: mouse and keyboard handling of the original

Date: 2026-10-13. Source: `/Users/max/Downloads/Game/cm0102.exe`, read with REA (Ghidra 12.1.4). Assembly was read for `FUN_0055f620`, `FUN_0068d520` (with the window procedure), `FUN_005eec20`, `FUN_005eecb0`, `FUN_005e8170`, `FUN_00402d10` and `FUN_00402d80`. The other functions were read as pseudocode only. Where the pseudocode hides a thiscall ECX, the text says "inference".

## 0. Conventions

- `D` is the screen manager (Display) base. `mx` is `DAT_00b67a20` and `my` is `DAT_00b67a14`.
- Widget `W(i)` sits at `D + 0xba95e + i*0x18c`. Field offsets below are relative to the widget start. Evidence: `FUN_005eec20` writes `+0x188`, and the pseudocode of `FUN_00560490` reads flags at `D+0xba96a`, which is `+0x0c`.
- Area `A(i)` fields are written as `D + i*0xbf1 + X`. The brief and the pseudocode use this form, so this file calls the field "area +X". The area object's own methods (`FUN_00402d10`, `FUN_00402d80`, `FUN_00403ee0`) use X minus 4. Example: open flag is area +0xbed here and `+0xbe9` in `FUN_00402d80`.
- Widget +0x00 is a pointer back to the Display (asm `0x5eeca3`: `MOV EAX,[ESI]` then `[EDX+EAX+0x210]`).
- The widget init `FUN_005ee150` shows the widget layout. It is the main evidence for the field meanings in section 4.

## 1. Input globals

| Global | Meaning | Written by | Cleared by | Read by |
|---|---|---|---|---|
| `b67a04` | Last virtual-key code (WM_KEYDOWN wParam), as dword | `0x68d672` in the window procedure | Only `FUN_005617c0` (sets 0 when it consumes a key). The pump never clears it. | `FUN_005617c0` (`0x56197f`..`0x561ee2`), `FUN_005e8xxx` (`0x5e879b`, `0x5e87cf`, not read) |
| `b67a08` | Tick in ms, taken once per pump | `0x68d5aa`: result of `FUN_00947299` (a clock thunk; inference: `timeGetTime` or `GetTickCount`) | Never | `FUN_00560c30` (300 ms test), `FUN_005ee150` (widget +0x50 init), `FUN_005eec20`, `FUN_005eecb0` |
| `b67a0c` | One-shot "left button went down" (dword) | `0x68d9c1` on WM_LBUTTONDOWN, only if `b67a3c==0` | `FUN_0055f620` (`0x55f68b`, `0x55f6a9`) | `FUN_0055f620`, `FUN_005cde40` (`0x5cde5b`, "any click" test), inside `FUN_00564080` (`0x5677ba`, network) |
| `b67a14` | Mouse y (client pixels, unsigned 16-bit) | `0x68d928` on WM_MOUSEMOVE (HIWORD of lParam), only if `b67a3c==0`; also `FUN_005621f0` | Never | all eight input functions |
| `b67a1c` | One-shot "right button went down" | `0x68d9e6` on WM_RBUTTONDOWN, only if `b67a3c==0` | `FUN_0055f620` (`0x55f6c9`, `0x55f6e7`) | `FUN_0055f620`, `FUN_00564080` (`0x5677d1`) |
| `b67a20` | Mouse x (LOWORD of lParam) | `0x68d922`; also `FUN_005621f0` | Never | same as `b67a14` |
| `b67a24` | Last WM_CHAR character (byte) | `0x68d737` | `FUN_0055f620` (`0x55f65a`), `FUN_005617c0` | `FUN_0055f620` (hotkey), `FUN_005617c0`, also `0x590240..0x591eaa` (other code, not read) |
| `b67a34` | Left button held (1 while down) | 1: WM_LBUTTONDOWN (`0x68d9bc`, only if `b67a3c==0`) and `FUN_005621f0`. 0: WM_LBUTTONUP (`0x68da03`, only if `b67a3c==0`), the pump when `b67a3c` counts down to 0 (`0x68d537`), WM_ACTIVATE and WM_PAINT paths (`0x68d7dd`, `0x68d891`), after any error box, and by the input code (see below) | Same list | all eight input functions |
| `b67a38` | Right button held | 1: `0x68d9e1`. 0: WM_RBUTTONUP (`0x68da21`), `0x68d7e3`, `0x68d897`. Also written at `0x7fc97f` inside `FUN_007fc7e0` (not read). | Same | `FUN_00560490`, `FUN_00561210` (only for widgets with flag 0x100) |
| `b67a3c` | Count of frames of a synthetic click. While non-zero, real mouse events are ignored. | `FUN_005621f0` (`DAT_00b67a3c = param_2`) | The pump decrements it each call (`0x68d530`) and clears `b67a34` when it reaches 0 | Window procedure (guards at `0x68d907`, `0x68d955`, `0x68d9ae`, `0x68d9d3`, `0x68d9f8`, `0x68da16`), `FUN_005621f0` (`0x56220d`, `0x562284`) |
| `b67a2c` | Window handle | not part of this task | | used for paint (`0x68d790`) |
| `9c36e8` | Cursor kind chosen in this frame (0 default) | Reset to 0 at `0x55f630`. Set to 2 by `FUN_00560490`, `FUN_00560c30`, `FUN_005617c0` (xrefs `0x560578`, `0x560788`, `0x560eb9`, `0x5618cc`) | `FUN_0055f620` start | `FUN_0055f620` passes it to `FUN_005e8170` (`0x55f726`) |
| `9cd804` | Cursor kind now shown (cache) | `FUN_005e8170` | | `FUN_005e8170`, `FUN_005e81e0` |
| `ad9c58` | Pointer to the area object that the keyboard and wheel scroll | `0x4031dd`, `0x4031ed`, `0x403752` (area-object methods; not read) | unknown | `FUN_005621f0` only |
| `ae2a28` / `ae2a2c` | Last fire time of the 1000 ms and 100 ms text timers | `FUN_00569610` | | `FUN_00569610` |

Window messages (window procedure at `0x68d630`; Ghidra merged it into `FUN_0068d520`. Evidence that it is a window procedure: four stack arguments, `RET 0x10`, default tail calls import `0x9671f4` with the four arguments, inference: `DefWindowProc`).

Dispatch is a byte table at `0x68da78` indexed by `msg-6`, with the dword table at `0x68da60`. The jump tables at `0x68db78` and `0x68db98` were read as bytes.

| Message | Effect |
|---|---|
| WM_KEYDOWN (0x100) | `b67a04 = wParam`. If wParam is 0x21..0x28, call `FUN_00803390(mode, 1)` and return 0. Table `0x68db78`: PgUp(0x21) to mode 3. PgDn(0x22) to mode 2. End(0x23) to mode 4. Home(0x24) to mode 5. Left(0x25) nothing. Up(0x26) to mode 1. Right(0x27) nothing. Down(0x28) to mode 0. `FUN_00803390` only calls `FUN_005621f0(mode, 1)`. |
| WM_CHAR (0x102) | `b67a24 = (byte)wParam`. Return 0. |
| WM_MOUSEMOVE (0x200) | If `b67a3c==0`: `mx = LOWORD(lParam)`, `my = HIWORD(lParam)`, then `FUN_005e81e0` (re-apply the cursor shape). |
| WM_LBUTTONDOWN (0x201) | If `b67a3c==0`: `b67a34 = 1`, `b67a0c = 1`. |
| WM_LBUTTONUP (0x202) | If `b67a3c==0`: `b67a34 = 0`. |
| WM_LBUTTONDBLCLK (0x203), 0x206..0x209 | Default window procedure. Nothing is queued. |
| WM_RBUTTONDOWN (0x204) | If `b67a3c==0`: `b67a38 = 1`, `b67a1c = 1`. |
| WM_RBUTTONUP (0x205) | If `b67a3c==0`: `b67a38 = 0`. |
| WM_MOUSEWHEEL (0x20A) | If `b67a3c==0`: `n = (int16)(wParam>>16) / 120` (signed, truncating; code uses the 0x88888889 multiply). Call `FUN_00803390(n>0 ? 1 : 0, |n|)`. Return 1 (also when `b67a3c!=0`). |
| WM_PAINT (0x0F) | BeginPaint, set the current display index `b77c7a` to 0, redraw all with `FUN_0055f300(-1,-1,-1,-1)`, restore `b77c7a`, `b67a34 = b67a38 = 0`, EndPaint. |
| WM_ACTIVATE (6), and one more activation message (inference: WM_ACTIVATEAPP 0x1C; the handler tests `msg==6` and else uses wParam) | On deactivate: minimise path (`FUN_005e2ea0`, `FUN_005a8f60`). On activate: if `b67a30` set, `FUN_005e2e50`, full redraw as WM_PAINT, `b67a34 = b67a38 = 0`. Details not needed. |
| WM_CLOSE (0x10) | Error box, return 0. |
| other | Default window procedure. |

Pump `FUN_0068d520` (assembly read):
```
if (b67a3c) { b67a3c--; if (b67a3c == 0) b67a34 = 0; }
while (PeekMessage(&msg, 0,0,0, 0)) {           // imports 0x9671f0 etc: names are inference from the call pattern
    if (!GetMessage(&msg,0,0,0)) { error box; b67a34 = 0; exit; }   // WM_QUIT path, 0x68d5b7
    TranslateMessage(&msg); DispatchMessage(&msg);
}
b67a08 = clock_ms();
```
The tick `b67a08` is therefore fixed for the whole frame. All 300 ms and 100 ms tests use it, except `FUN_00569610`, which reads the clock itself.

Key behaviours the port needs:
1. A real click gives two signals: the one-shot `b67a0c` / `b67a1c` and the level `b67a34` / `b67a38`.
2. Synthetic click (`FUN_005621f0`): set `mx,my`, set `b67a34 = 1`, set `b67a3c = n`. The pump releases the button after `n` pumps.
3. The keyboard and the wheel never scroll directly. They warp the cursor into a scroll zone of area `ad9c58` and hold the left button for `n` frames. The scrollbar code (`FUN_00561f10`) then scrolls by one step per frame.

`FUN_005621f0(mode, n)` (pseudocode):
```
A = ad9c58 as int[];            // area object, area-relative offsets
if (A == 0) return;
b67a34 = 1; b67a3c = n;
switch (mode) {
 case 0: mx = A[0]+1; my = A[5]+2;  break;   // Down key, wheel down
 case 1: mx = A[0]+1; my = A[1]+1;  break;   // Up key, wheel up
 case 2: mx = A[0]+1; my = A[10]+2; break;   // PgDn
 case 3: mx = A[0]+1; my = A[4]+1;  break;   // PgUp
 default: b67a34 = 0; b67a3c = 0;            // End, Home: nothing
}
```
A[0] and A[1] are the area x and y (area-relative +0x00, +0x04: `FUN_00402d10` flips `[+0],[+4],[+8],[+0xc]`). The meaning of A[4], A[5], A[10] (area +0x10, +0x14, +0x28) is unknown. They are not the scrollbar struct (that sits at area-relative +0xb7a). See doubts.

## 2. The eight functions

All are thiscall on D. Mode is the dword `D+0x0`.

### 2.1 `FUN_0055f620` (poll)

Returns s16 in AX. -1 means nothing. Assembly read.
```
cursorKind = 0                                   // 9c36e8
if (D.hotkeyResult(+0x12f4ff) != -1 && D.hotkeyChar(+0x12f4fe) != 0 && b67a24 == D.hotkeyChar) {
    cursor(0); b67a24 = 0; return D.hotkeyResult;
}
if (b67a0c) {
    if (D.leftClickResult(+0x12f501) != -1) {
        cursor(0); b67a0c = 0; r = D.+0x12f501; D.+0x12f501 = -1; return r;
    }
    b67a0c = 0;                                  // click discarded
}
if (b67a1c) { same with D.+0x12f503, b67a1c }
FUN_00569610();                                  // text timers, section 2.8
switch (D.mode) {
  0: r = FUN_00560490(); 1: r = FUN_00560c30(); 2: r = FUN_00561050(); 3: r = FUN_00561210();
  default: error box "display.cpp" line 0x201; b67a34 = 0; return 0;
}
cursor(cursorKind);                              // 9c36e8
return (s16) r;
```
The hotkey and the click results are one-shot. They skip the hit test for that frame.

### 2.2 `FUN_00560490` (mode 0, hit test)

Returns s16 widget index, or -1. Pseudocode read, not assembly. The ECX of the highlight calls is an inference (the pseudocode guards them with the hover flag of the current widget).

Inputs: widget order list `D.+0x12eb96[]` with count `D.+0x12f4f8`. Area order list `D.+0x12e9a2[]` with count `D.+0x12f4f6`.
```
hit = -1
for pass in 1..2:
  // A: free widgets, in draw order
  for k in 0 .. D.+0x12f4f8-1:
     w = widgetOrder[k]; f = W(w).flags(+0x0c)
     if (f & 2) && pass == 1:
        held = (f & 0x100) ? b67a38 : b67a34
        inside = W.x1 <= mx <= W.x2 && W.y1 <= my <= W.y2          // +0x10,+0x18,+0x14,+0x1c, inclusive
        if (!inside)            { if (W.hover) unhighlight(W); }
        else if (!held)         { if (W.hover) unhighlight(W); cursorKind=2 if (W.+0x08 != 0 || W.+0x7c != -1); }
        else                    { if (!W.hover) highlight(W); hit = w; cursorKind=2 if (W.+0x08 != 0 || W.+0x7c != -1); }
     if (W(w).flags & 8) {                       // text entry, both passes
        if (textEntry(w)) {                      // FUN_005617c0
            ret = widgetOrder[k];
            for k2 in k .. end: if (flags&8) textEntry(widgetOrder[k2]);   // let the others react
            if (pass == 1) for all widgets in order: if (flags&8) textEntry(w);  // second sweep
            return ret;
        }
     }
  // B: rows of open top-level areas
  for k in 0 .. D.+0x12f4f6-1:
     a = areaOrder[k]
     if (A(a).+0xbed != 0 /*open*/ && A(a).+0x212 == -1 /*top level*/) {
        A.cur(+0xb7c) = A.first(+0xb7a)
        while (A.cur < A.count(+0xb78)) {
           w = A.list(+0x216)[A.cur]; A.cur++
           if (w == -1) break
           if (W(w).row(+0x24) - W(A.list[A.first]).row > A.pageRows(+0xbb0) - 1) break    // beyond the visible rows
           same highlight / hit / cursor test as in A  (flag 2, pass 1; hit = w)
           if (flags & 8 && textEntry(w)) → same early return as above (then rerun text sweeps over the remaining areas)
        }
     }
     if (A(a).scrollbarPresent(+0xbf1) != 0 && pass == 1) FUN_00561f10(a);    // section 2.5
  pass++
// after pass 2
if (hit == -1) return -1
if (W(hit).popup(+0x7c) != -1) {
    open(A(W.popup));                            // FUN_00402d10, ECX inference: area W.+0x7c
    D.mode = 1; D.+0x12f52d = W.popup; b67a34 = 0;
    if (W.+0x08 == 0) return -1;                 // pure popup opener
}
if (W.flags & 0x40) {                            // drag start
    D.mode = 3; D.+0x12f509 = mx; D.+0x12f50d = my; D.+0x12f531 = hit;
    D.+0x12f511.. +0x12f51d = W.x1, y1, x2, y2; D.+0x12f521 = 0; return -1;
}
return hit
```
Notes:
- `hit` is the last widget under the pointer with the button held. Later widgets in the order win (the topmost). Rows of areas override free widgets.
- A click gives a result in every frame in which the button stays down. Auto-repeat is real for the scrollbar (section 2.5). Whether a normal button fires once depends on the screen loop clearing `b67a34`. Not determined, see doubts.
- Highlight (hover flag +0x188) therefore means "pressed and inside" in mode 0, not "mouse over". Mouse-over highlight exists only in popup areas (mode 1).
- The scrollbar test `FUN_00561f10` runs for every area with the flag at area +0xbf1, whether or not the area is open (as the pseudocode shows; inference: closed areas have the flag clear).

### 2.3 `FUN_00560c30` (mode 1, popup)

Returns widget index (low 16 bits), or -1. Pseudocode read.
```
held = b67a34; best = -1
for pass in 1..2:
  for each area a in areaOrder:
     if (A(a).open(+0xbed) && A(a).+0x212 != -1) {         // popup areas only
        walk visible rows as in 2.2 B; for each widget w with flag 2:
          inside = point in W rect
          if (!inside) {
              if (W.hover) { if (W.popup(+0x7c) == -1) unhighlight(W); }     // keep it lit if it owns a sub-popup
              if (!W.hover && now - W.+0x50 > 299 && W.popup != -1 && A(W.popup).open && pass == 1)
                    close(A(W.popup));                                        // sub-popup closes 300 ms after the highlight left
          } else {
              if (!W.hover) {
                  x = A(a).+0x214;  if (x != -1 && W(x).hover) unhighlight(W(x));
                  highlight(W);
                  v = A(a).+0x212;                               // owner widget of this popup
                  if (!W(v).hover) {
                      y = A(W(v).ownerArea(+0x7a)).+0x214; if (y != -1 && W(y).hover) unhighlight(W(y));
                      highlight(W(v));
                  }
              }
              if (held) best = w;
              if (W.+0x08 != 0 || W.+0x7c != -1) cursorKind = 2;
          }
          if (W.hover && now - W.+0x50 > 299 && W.popup != -1 && !A(W.popup).open && pass == 2)
                    open(A(W.popup));                                         // sub-popup opens after 300 ms
     }
// end
if ((best != -1 && W(best).popup == -1) || held) { close(popup); D.mode = 0; b67a34 = 0; }   // ECX of close: inference, the active popup area
return best          // -1 if the release or click hit nothing
```
`now` is `b67a08`. Writer of area +0x214 is not in these functions. See doubts.

### 2.4 `FUN_00561050` (mode 2, scrollbar thumb drag)

Returns s16 -1 always (mode change shows in D.mode). The `puVar1==0` assert (line 0x4e1) cannot fire: the pointer is base plus an offset.
```
dy = my - D.+0x12f50d
if (b67a34 == 0) { D.mode = 0; return -1; }
if (dy != 0) {
   sb = &A(D.+0x12f52f).+0xb7e                  // scrollbar struct, ints: [0..3] rect x1,y1,x2,y2; [4] trackTop; [5] trackBottom;
                                                // [6] maxPos; [7] pos; [8] thumbHeight; [9] thumbTop; [10] thumbBottom; [11] saved image
   if (sb[9]  + dy < sb[4])  dy = sb[4]  - sb[9];
   if (sb[5]  < sb[10] + dy) dy = sb[5]  - sb[10];
   if (dy != 0) {
      sb[9] += dy; sb[10] += dy;
      if (sb[11]) { restore(sb[0], sb[1], sb[11]); free(sb[11]); sb[11] = 0; }
      FUN_00403c80();                           // area redraw (ECX = area; unread)
      flip(sb[0], sb[1], sb[2], sb[3]);
      old = sb[7];
      sb[7] = (sb[9] - sb[4]) * sb[6] / ((sb[5] - sb[8] - sb[4]) + 1);
      if (sb[7] != old) scroll(area, sb[7] - old);  // FUN_00403ee0, section 5
   }
   D.+0x12f50d = my;
}
return -1
```
Only the y delta is used. The thumb does not jump to the pointer. Dead zone: the last-y is updated even when the clamp gives dy = 0.

### 2.5 `FUN_00561f10` (area scrollbar click; arg s16 area index)

Called from `FUN_00560490` pass 1 for each area. Returns 1 if the position changed, else 0. The `piVar1==0` assert (line 0x71f) cannot fire.
```
sb = &A(a).+0xb7e;  old = copy of sb;  (oldPos = sb[7]; oldState = byte A.+0xbae)
A.+0xbae = 0
if (b67a34) {
   if (inside [sb.x1,sb.x2] x [sb.y1, trackTop-1])            { pos = max(pos-1, 0); A.+0xbae = 1; }        // up arrow
   else if (inside [x1,x2] x [trackBottom+1, sb.y2])           { pos = min(pos+1, maxPos); A.+0xbae = 2; }   // down arrow
   else if (inside [x1,x2] x [trackTop, thumbTop-1])           { pos = max(pos - A.pageRows(+0xbb0), 0); }   // track above thumb
   else if (inside [x1,x2] x [thumbBottom+1, trackBottom])     { pos = min(pos + A.pageRows, maxPos); }      // track below thumb
   else if (inside [x1,x2] x [thumbTop, thumbBottom])          { D.mode = 2; D.+0x12f52f = a; D.+0x12f50d = my; }
}
if (pos != oldPos) {
   thumbTop    = ((trackBottom - thumbH - trackTop) + 1) * pos / maxPos + trackTop;   // sb[9]
   thumbBottom = thumbTop - 1 + thumbH;                                               // sb[10]
   if (sb[11]) { restore; free; sb[11] = 0 }
   FUN_00403c80(); flip(sb rect); scroll(area, pos - oldPos);
   return 1;
}
if (A.+0xbae != oldState) { free saved image; FUN_00403c80(); flip(sb rect); }   // arrow pressed look
return 0
```
Evidence: struct offsets `0xb7e..0xbb0` in the pseudocode. Arrow clicks step one row per call, and a call happens each frame while the button is held. A wheel notch is one such frame.

### 2.6 `FUN_00561210` (mode 3, widget drag)

Returns s16: -1 while dragging; the dragged widget index on release.
```
held = (W(D.+0x12f531).flags & 0x100) ? b67a38 : b67a34
dx = mx - D.+0x12f509; dy = my - D.+0x12f50d
if (held) {
   if (dx != 0 || dy != 0 || D.+0x12f521 == 0) {
      clamp dx so that rect stays inside x 0..799 (x1+dx >= 0, x2+dx <= 799); same for dy, 0..599
      if (dx != 0 || dy != 0 || D.+0x12f521 == 0) {
         if (D.+0x12f521) { restore(rect.x1, rect.y1, img); flip(rect); free(img); D.+0x12f521 = 0; }
         rect += (dx,dy)                               // D.+0x12f511..+0x12f51d
         D.+0x12f521 = save(rect, 0)                   // FUN_005e3dd0
         if (!(W.style(+0x38) & 0x10000)) outline(rect, 1, colour ae31a8)   // FUN_005e3b50
         else                              outline2(rect, 1, colour ae31a8) // FUN_005e7840
         flip(rect)
      }
      D.+0x12f509 = mx; D.+0x12f50d = my
   }
   return -1
}
// release
if (D.+0x12f521) { restore(rect); flip(rect); free(img); D.+0x12f521 = 0 }
d = D.+0x12f531
W(d).+0x7e = -1
for k in widgetOrder: w = order[k]
     if (flags(w) & 2 && !(flags(w) & 0x200) && W(w).x1 <= mx <= W(w).x2 && W(w).y1 <= my <= W(w).y2 && W(d).+0x7e == -1) {
         W(d).+0x7e = w;                                // drop target
         W(d).+0x60..+0x6c = D.+0x12f511..+0x12f51d     // final dragged rect
     }
for each open top-level area row: same test and same writes
D.mode = 0
return D.+0x12f531
```
The drop target is the first widget in order that contains the pointer (the rects are the stored, unmoved ones). The dragged widget may be its own target. Widgets with flag 0x200 are never targets. The widget itself does not move. The screen loop reads `+0x7e` and `+0x60..+0x6c`.

### 2.7 `FUN_005617c0` (text entry; arg s16 widget index)

Returns bool "text finished and changed" (`W.+0x180 != 0` after Tab, Enter, or a click outside). Called from `FUN_00560490` twice per frame per text widget (once per pass).
```
assert (W.flags & 8) else error box line 0x5ee
buf = copy of W.text (+0x80, 256 bytes)
inside = point in W rect; if (inside) cursorKind = 2
cur = W.+0x44 (-1 = not editing); maxlen = W.+0x40
if (b67a34 != 0) {
   if (!inside) {
       if (cur >= 0) { W.+0x44 = -1; restoreBg; setText(buf,1); b67a04 = b67a24 = 0; return W.+0x180 != 0; }
       // else fall to keys
   } else if (cur == -1) {                       // click inside: start editing
       W.+0x44 = strlen(buf); W.+0x180 = 0; restoreBg; setText(buf,1); b67a24 = b67a04 = 0; return false;
   }
}
// keys
if ((b67a04 == 9 /*Tab*/ || b67a04 == 13 /*Enter*/) && W.+0x44 >= 0) {
    W.+0x44 = -1; restoreBg; draw(W); flip(rect);
    if (!(W.flags & 0x20)) D.+0x12f533 = w;      // pass focus on
    b67a04 = b67a24 = 0; setText(buf,1); return W.+0x180 != 0;
}
if (D.+0x12f533 != -1 && w != D.+0x12f533 && W.+0x44 == -1) {   // someone passed the focus: this widget starts editing
    W.+0x44 = strlen(buf); W.+0x180 = 0; restoreBg; setText(buf,1); D.+0x12f533 = -1; b67a04 = b67a24 = 0; return false;
}
if (W.+0x44 >= 0 && b67a04 != 0 && W.+0x40 < 0) {   // negative maxlen: first key clears the text
    setText("",0); W.+0x44 = 0; W.+0x180 = 1; W.+0x40 = abs(W.+0x40); restoreBg; draw; flip; return false;   // key stays queued
}
if (cur > 0 && key == 0x25 /*Left*/)                { cur--; restoreBg; }
else if (cur >= 0 && cur < len && key == 0x27 /*Right*/) { cur++; restoreBg; }
else if (cur > 0 && key == 8 /*Backspace*/)          { remove buf[cur-1]; cur--; W.+0x180 = 1; restoreBg; }
else if (cur >= 0 && cur < len && key == 0x2e /*Delete*/) { remove buf[cur]; W.+0x180 = 1; restoreBg; }
else {
   if (cur < 0) return false;
   if (len >= W.+0x40) return false;
   c = b67a24;
   if (c < 0x20) return false;
   reject c in { '"', '*', '/', '<', '>', '|', '{', '}', '[', ']', '~' }
   ':' and '\\' accepted only if W.flags & 0x8000;  '?' accepted only if W.flags & 0x10000
   tmp = buf + c; if (textWidth(font W.+0x76, tmp) > (W.x2 - W.x1) - 10) { b67a04 = b67a24 = 0; return false; }
   insert c at cur; cur++; W.+0x180 = 1; restoreBg;
}
setText(buf, 1)  // FUN_005eed50
b67a24 = b67a04 = 0; return false
```
`restoreBg` means: if `W.+0x4c` (saved image) is non-zero, `FUN_005e3fd0(W.x1, W.y1, W.+0x4c)`. `setText` is `FUN_005eed50(buf, redraw)` with ECX = the widget (inference); it was not read. The rejected-character list is the filter for file names, so this widget serves save-game and file name entry as well as other text.

### 2.8 `FUN_00569610` (text timers)

Void. Runs every poll, before the mode dispatch.
```
now = clock_ms()                                  // FUN_00947299, not b67a08
fire1000 = (now < ae2a28 || ae2a28 + 1000 <= now); if (fire1000) ae2a28 = now
fire100  = (now < ae2a2c || ae2a2c + 100  <= now); if (fire100)  ae2a2c = now
if (fire1000 || fire100)
  for i in 0 .. D.+0x12e9a0-1:                    // all widgets
     W = widget i; f = W.flags
     if ((f & 0x800) && fire1000) {                // rotating text
        s = W.text; find first byte 0x01 or 0 → idx; if s[idx]==1 { s[idx]=0; idx++ }
        rest = s + idx
        tmp = sprintf("%s%c%s", rest, 1, s)        // rest + \x01 + first part
        if (strcmp(s, tmp) != 0) { setText(tmp, 0); draw rect via FUN_0055f300(x1,y1,x2,y2) }
     }
     if ((f & 0x1000) && fire100) {                // blink
        swap(W.+0x72 (s16), W.+0x78 (s16))
        if (screenDisplay(b74c4c[b77c7a*0xc0]).+0x12f545 != 0) FUN_0055f300(rect)
     }
```
`D.+0x12f545` is a non-zero flag of the Display shown now (inference: "drawing allowed"). The rotating text shows long strings that scroll by swapping halves around a 0x01 marker. In a text without a marker, the string alternates between `text` and `\x01text` (inference from the code; check with a test).

## 3. Screen manager fields

| Field | Meaning | Evidence |
|---|---|---|
| +0x12f4fe (byte) | Hotkey character; compared with `b67a24` | `0x55f63f`..`0x55f649` |
| +0x12f4ff (s16) | Value returned when the hotkey is pressed; -1 = no hotkey | `0x55f636`, `0x55f660` |
| +0x12f501 (s16) | Value returned on the next left-click (one-shot); -1 = none | `0x55f679`..`0x55f698` |
| +0x12f503 (s16) | Same for the right click | `0x55f6b7`..`0x55f6d6` |
| +0x12f505 | Not used by the eight functions. Unknown. | |
| +0x12f509 (s32) | Last mouse x for the drag | `FUN_00561210` |
| +0x12f50d (s32) | Last mouse y for the widget drag and the scrollbar drag (shared) | `FUN_00561210`, `FUN_00561050`, `FUN_00561f10` |
| +0x12f511, +0x12f515, +0x12f519, +0x12f51d (s32 each) | Rect (x1, y1, x2, y2) of the dragged widget, moved each frame | `FUN_00560490` copies it from the widget at drag start; `FUN_00561210` moves it |
| +0x12f521 (image) | Handle of the screen background saved under the drag outline (0 = none). It is the saved image from `FUN_005e3dd0`; freed with `FUN_005e42b0`. | `FUN_00561210` |
| +0x12f52d (s16) | Index of the popup area opened by the click | `FUN_00560490` (written; not read by the eight) |
| +0x12f52f (s16) | Area whose scrollbar thumb is dragged | `FUN_00561f10`, `FUN_00561050` |
| +0x12f531 (s16) | Dragged widget index | `FUN_00560490`, `FUN_00561210` |
| +0x12f533 (s16) | Text entry that has just finished with Tab or Enter; the next text widget takes over editing. -1 = none. | `FUN_005617c0` |
| +0x12f539, +0x12f53b | Not used by the eight. Unknown. | |
| +0x12f545 | Non-zero when the Display shows (inference) | `FUN_00569610`; also written in `FUN_00564080` |
| +0x12f551, +0x12f555, +0x12f559 | Not used by the eight. +0x12f551 is read in the network loop `FUN_007fdc00`. +0x12f555 is written in `FUN_00564080`. Unknown. | prior inventory |
| +0x12f4f6 (s16) | Area count in draw order; list `+0x12e9a2[]` | `FUN_00560490` |
| +0x12f4f8 (s16) | Widget count in draw order; list `+0x12eb96[]` | |
| +0x12e9a0 (s16) | Total widget count | `FUN_00569610` |

## 4. Widget and area fields

Widget layout from `FUN_005ee150` (its parameter order). Offsets in bytes.

| Offset | Meaning | Evidence |
|---|---|---|
| +0x00 | Pointer to the Display | asm `0x5eeca3` |
| +0x08 | Action id (`param_19`). Non-zero: the widget gives a result on click and shows cursor kind 2. | `FUN_00560490`, `FUN_00560c30`; init |
| +0x0c | Flags (see below) | |
| +0x10, +0x14, +0x18, +0x1c | Rect x1, y1, x2, y2, inclusive | hit tests |
| +0x20 | Column in the area grid | `FUN_00403ee0` |
| +0x24 | Row in the area grid | `FUN_00403ee0`, `FUN_00560490` |
| +0x28..+0x34 | Fixed rect override (x1, x2, y1, y2); 0 = take from the area tables | `FUN_00567ac0`, `FUN_00403ee0` |
| +0x38 | Style flags. Bits 0x30: redraw on highlight. 0x10000: outline variant for the drag. 0x400 and 0x4000: coordinate swap rule at init. | `FUN_005eec20`, `FUN_00561210`, init |
| +0x40 | Text entry: maximum length. A negative value means "the first key clears the text"; the code stores the absolute value afterwards. | `FUN_005617c0` |
| +0x44 | Text entry: cursor position; -1 = not editing. Init: -1, or `strlen` if flags 8 and 0x10 are set. | init, `FUN_005617c0` |
| +0x48 | Init value `param_20`. Not read by the eight. Unknown. | |
| +0x4c | Saved background image handle (0 = none) | `FUN_005eec20`, `FUN_005617c0` |
| +0x50 | Tick of the last highlight change | `FUN_005eec20`, `FUN_005eecb0`, `FUN_00560c30` |
| +0x54 | Pointer to the caller's text buffer (only if flag 8). `FUN_00567b00` copies the edited text back into it. | `FUN_00567b00` |
| +0x60..+0x6c | Drop rect written by a drag release | `FUN_00561210` |
| +0x70 | Own index | `FUN_005eec20` |
| +0x72, +0x74 | Two image states. If flag 4 is set and they are equal, a highlight does not redraw. +0x72 is also swapped with +0x78 by the blink timer. | `FUN_005eec20`, `FUN_00569610` |
| +0x76 | Font id | `FUN_005617c0` |
| +0x7a | Owner area index (-1 = free widget) | `FUN_005eec20` |
| +0x7c | Popup area index opened by this widget (-1 = none) | `FUN_00560490`, `FUN_00402d80` |
| +0x7e | Drop target widget index after a drag (-1 = none). Init -1. | `FUN_00561210`, init |
| +0x80 | Text, 256 bytes | |
| +0x180 | Text changed flag | `FUN_005617c0` |
| +0x184 | Init: 1 if an odd row of a grid | init |
| +0x188 | Highlight flag ("hover"; in mode 0 it means pressed inside) | `FUN_005eec20` |

Flag bits of widget +0x0c that this code tests:

| Bit | Meaning |
|---|---|
| 0x02 | Takes part in the mouse tests |
| 0x04 | Has two image states (+0x72, +0x74) |
| 0x08 | Text entry (`FUN_005617c0`) |
| 0x10 | With 0x08: cursor starts at the end of the text (init) |
| 0x20 | Text entry: Tab or Enter does not pass the focus on |
| 0x40 | Draggable (mode 3) |
| 0x100 | Reacts to the right button (`b67a38`) in `FUN_00560490` and `FUN_00561210` |
| 0x200 | Not a drop target |
| 0x800 | Rotating text (timer, 1 s) |
| 0x1000 | Blink (timer, 100 ms) |
| 0x8000 | Text entry accepts `:` and `\` |
| 0x10000 | Text entry accepts `?` |

Area fields (form `D + i*0xbf1 + X`):

| Field | Meaning | Evidence |
|---|---|---|
| +0x20e | Not used by the eight. Tested against -1 in the widget init (grid mode off/on). | `FUN_005ee150` |
| +0x210 (s16) | Index of the highlighted widget of this area. Highlight sets it, unhighlight clears it only if it equals the widget. | `FUN_005eec20` (asm `0x5eeca5`), `FUN_005eecb0` (asm `0x5eed46`) |
| +0x212 (s16) | Owner widget of a popup area; -1 for a top-level area | `FUN_00560490`, `FUN_00560c30` |
| +0x214 (s16) | A widget index that mode 1 un-highlights before it highlights a new one. Writer unknown. Inference: previous highlighted row. | `FUN_00560c30` |
| +0x216 (s16[]) | List of widget indices in the area, row by row; -1 ends a row list | iteration loops |
| +0xb78 (s16) | Count of that list | |
| +0xb7a (s16) | First visible list entry | |
| +0xb7c (s16) | Scratch iterator, set to +0xb7a before every walk | |
| +0xb7e.. | Scrollbar struct, see 2.4 | |
| +0xbae (byte) | Arrow pressed state: 0 none, 1 up, 2 down | `FUN_00561f10` |
| +0xbb0 (byte) | Visible rows per page; 0 = no scrolling | `FUN_00403ee0` (byte at area-relative +0xbac) |
| +0xbed (s32) | Open flag | `FUN_00402d80` (area-relative `0xbe9`) |
| +0xbf1 (s32) | Scrollbar present flag | `FUN_00560490` |

## 5. Helpers the port lacks

All were read. `flip(x1,y1,x2,y2)` is `FUN_005e2eb0`, `restore(x,y,img)` is `FUN_005e3fd0`, `free(img)` is `FUN_005e42b0`, `save(x1,y1,x2,y2,0)` is `FUN_005e3dd0`. They are not in scope here.

`FUN_005eec20` highlight on (thiscall, ECX = widget; assembly):
```
W.hover(+0x188) = 1
W.tick(+0x50)   = b67a08
if ((W.style(+0x38) & 0x30) && !((W.flags & 4) && W.+0x72 == W.+0x74)) {
    if (W.+0x4c) restore(W.x1, W.y1, W.+0x4c);
    drawWidget(W, 0);                         // FUN_005ee590
    flip(W.x1, W.y1, W.x2, W.y2);
}
if (W.owner(+0x7a) != -1) A(W.owner).+0x210 = W.+0x70;
```
`FUN_005eecb0` highlight off: the same, with `hover = 0`, and the last line is `if (W.owner != -1 && A(owner).+0x210 == W.+0x70) A(owner).+0x210 = -1`.

`FUN_00402d10` popup open (ECX = area object P; assembly). Offsets are area-relative:
```
FUN_00403760(&x, &y);                         // compute the popup position (unread)
if (P.x != x || P.y != y) FUN_00403a70(x, y); // move the popup (unread)
P.open(+0xbe9) = 1
FUN_00402e70(0);                              // draw the popup and its widgets (unread)
flip(P.x, P.y, P.x2, P.y2);                   // [+0],[+4],[+8],[+0xc]
```
`FUN_00402d80` popup close (ECX = area object P; assembly):
```
if (!P.open) return;
for (i = P.first(+0xb76); i != -1 && i < P.count(+0xb74); i++) {
    w = P.list(+0x212)[i];
    P.widgets(+0x208)[w].hover = 0;
    if (P.widgets[w].+0x7c != -1) close(P.areas(+0x204)[P.widgets[w].+0x7c]);   // recursive
}
P.open = 0;
if (P.savedImage(+0x200)) { restore(P.x, P.y, img); free(img); P.savedImage = 0; flip(P.x, P.y, P.x2, P.y2); }
```
`FUN_00403ee0` scroll (ECX = area object, arg `delta` in rows; area-relative offsets; pseudocode only):
```
if (P.pageRows(+0xbac) == 0 || P.count(+0xb74) <= 0) return;
if (P.savedImage(+0x200)) restore(P.x, P.y, img);        // not freed here
first = P.first(+0xb76); row(i) = widget[list[i]].+0x24; cur = row(first)
if (delta < 1) {
    d = 0; while (delta < d && first-1 >= 0) { first--; d = row(first) - row(P.first); }    // old first as origin
    while (first != 0 && row(first-1) == row(first)) first--;                          // back to start of the row
} else {
    d = 0; while (d < delta && first+1 < count) { first++; d = row(first) - row(P.first); }
}
P.first = first
for (i = first; i != -1 && i < count; i++) {
    w = widget[list[i]]; r = w.row - widget[list[P.first]].row
    if (r > P.pageRows - 1) break
    if (w.+0x28 == 0) { x1 = P.colX[w.col] + P.x; x2 = P.colX2[w.col] + P.x } else { x1 = w.+0x28; x2 = w.+0x2c }
    if (w.+0x30 == 0) { y1 = P.rowY1[P.+0x10 + r] + P.y; y2 = P.rowY2[P.+0x10 + r] + P.y } else { y1 = w.+0x30; y2 = w.+0x34 }
    w.x1 = x1; w.x2 = x2; w.y1 = y1; w.y2 = y2
    drawWidget(w, 0)                                       // FUN_005ee590
}
flip(P.x, P.y, P.x2 - 0x16, P.y2)                          // 0x16 = scrollbar width, 22 px
FUN_00800330(P.id(+0xb72), widget[list[P.first]].row)      // notify the screen of the first visible row (unread)
```
The tables `colX` (area-relative `+0x1c..`), `colX2` (`+0x10c..`), `rowY1` (`+0x94..`), `rowY2` (`+0x184..`) are read as `in_ECX[col+7]`, `in_ECX[col+0x43]`, `in_ECX[idx+0x25]`, `in_ECX[idx+0x61]`. The meaning of `P.+0x10` as an index base is an inference.

`FUN_005e8170` cursor(kind) (assembly):
```
if (cursorShown(9cd804) == kind) return;
cursorShown = kind;
ShowCursor(0);                               // import 0x9671c0 (inference by pattern)
if (kind == 0) SetCursor(aebe24);            // import 0x9671bc
else if (kind == 1) SetCursor(aebe1c);
else if (kind == 2) SetCursor(aebe20);
ShowCursor(1);
```
For kind 3 or higher it only toggles ShowCursor. `FUN_005e81e0` (called on WM_MOUSEMOVE) re-applies the cached kind: if `GetCursor() != handle(kind)`, it does the same sequence. The image of each cursor is not determined (handles are loaded elsewhere). The eight functions only ever pick kind 0 (default, also the reset on hotkey and clicks) or kind 2 (over an interactive widget or a text widget). Kind 1 is never chosen here.

## 6. What the port can leave out, and what looks unused

For single player the port needs all of sections 2 and 5. Reasons: popup menus, scrollbars, drag, text entry and the wheel all appear in single-player screens. It can leave out:
- The network packet writers of these fields (`FUN_00564080`, `FUN_007fdc00`). The port builds screens directly.
- WM_CLOSE error box, the WM_QUIT error path in the pump, and the minimise logic of WM_ACTIVATE. On focus loss the port should clear `b67a34` and `b67a38`.
- The assert branches of `FUN_00561050` and `FUN_00561f10` (dead: pointer plus offset is never 0), and the mode > 3 branch of `FUN_0055f620`.

Looks unused or without effect:
- Keys End and Home (modes 4 and 5 clear the button at once) and VK Left and Right in the window procedure (no action).
- Cursor kind 1, and the `kind >= 3` branch of `FUN_005e8170`.
- Display fields +0x12f505, +0x12f539, +0x12f53b, +0x12f559, and widget +0x48. Not touched by any of the eight functions.
- Double clicks (not delivered as clicks).
- The right button is used only by widgets with flag 0x100. I did not check whether any screen creates such a widget (it needs a scan of the callers of `FUN_0055f990`). The port can keep the path, since it is small.

## 7. Open doubts

1. One click or many: mode 0 returns the widget in every frame while the button is held, because it tests `b67a34` (level). The scrollbar relies on this (one step per frame). For normal buttons the screen loop (`FUN_007fc7e0`, `FUN_007fdc00`) must clear `b67a34` after it dispatches the result, or the screens must ignore repeats. I did not read the loops. The port must decide by reading `FUN_007fc7e0` or by a differential test.
2. ECX of the highlight and popup calls in `FUN_00560490` and `FUN_00560c30` is inferred. The pseudocode drops it. The guards (`hover` of the widget just tested) and the area-object use in `FUN_00402d10` make the inference likely, but an assembly check of the calls is still open. Same for the ECX of the close call at the end of `FUN_00560c30` (assumed: the active popup area `D.+0x12f52d`).
3. Meaning of area +0x214 and of its writer. Meaning of area +0x20e.
4. `FUN_005621f0` uses `A[4]`, `A[5]`, `A[10]` (area +0x10, +0x14, +0x28) as y positions for the key presses. Their meaning is unknown, so the exact pixel the key presses click is unknown. `ad9c58` is set in `0x4031dd`, `0x4031ed`, `0x403752` (area methods, not read). The port needs to know which area is "current", and when.
5. A wheel delta with absolute value under 120 gives `n = 0`. Then `FUN_005621f0(0,0)` sets `b67a34 = 1` and `b67a3c = 0`. The pump only decrements a non-zero counter, so the button stays "held" until a real button-up. High-resolution wheels may hit this. Inference from the code; not tested.
6. Which writer sets `D.+0x12f4fe`, `+0x12f4ff`, `+0x12f501`, `+0x12f503`? Not among the eight functions. Probably screen build code or the setter functions near `0x567ac0`; I did not find it.
7. Tab focus order in `FUN_005617c0`: after a widget finishes, the first text widget in the order list with `cur == -1` and a different index takes over, not the next one by position. This is what the code says. The real behaviour may depend on how screens order their widgets.
8. `FUN_005eed50` (set text), `FUN_005ee590` (draw widget), `FUN_005e5ad0` (text width), `FUN_00403c80` (area redraw), `FUN_00403760`, `FUN_00403a70`, `FUN_00402e70`, `FUN_00800330`, `FUN_005e3b50`, `FUN_005e7840`: not read. The port may already have equivalents of some.
9. The second activation message number (probably 0x1C) is inferred from the dispatch byte table, which I could not count with certainty.
10. The import names behind `0x9671c0`, `0x9671bc`, `0x9671f0`, `0x9671e4`, `0x9671e0`, `0x9671dc`, `0x9671f4` are inferred from call patterns, not from the import table.
