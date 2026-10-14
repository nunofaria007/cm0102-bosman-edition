# Screen stack inventory (scrman.cpp of the original)

Method: objdump disassembly of the original (Intel syntax) and REA for function bounds. Every fact names the address that shows it. "Inference" marks a conclusion that the code does not show directly. All offsets are hex. `S` is the screen stack object at 0xb74450. `ch` is the current channel, the s16 at `S+0x382a` (global 0xb77c7a).

## Corrections to the earlier inventory

- `FUN_0090ba90` is a byte swap of two buffers, not a copy (0x90bb52..0x90bb5d). Every "copy record to slot 0" in the earlier inventory is a swap.
- Result -0xb does not end a screen. In the main loop it rebuilds the current screen (jump table 0x7fdbcc, case 0x7fd3a9).
- The 60 "text slots" hold any 32-bit value. Screens store integers in them as often as text. Example: the start screen reads slot 0 as a flag (0x81bd82) and clears it with `FUN_007ff270(0, 0, 0)` (0x81bd93).
- The functions `FUN_00567ac0` to `FUN_00568290` are mostly buffer readers. The widget fields of opcode 8 are written inline in `FUN_00564080` (0x5670ec to 0x567436). See item 4.
- The main loop is named `SCREEN_MANAGER::monitor()` by its own error text (string 0xa7dde4, used at 0x7fd566).

## 1. Layouts

### 1a. Screen stack object S (fields that one local player needs)

| Offset | Size | Meaning | Evidence |
|---|---|---|---|
| +0x7d4 | 16 x 16 x 0x30 | Records. Channel c, slot s is at `S + 0x7d4 + c*0x300 + s*0x30`. | 0x7ff9cc..0x7ff9dc, 0x7ffb04 |
| +0x37d4 | 4 | Default build function | 0x7feb28, read 0x7fe635 |
| +0x37d8 | 4 | Default input function | 0x7feb32 |
| +0x37dc | 4 | Default flag | 0x7feb3c |
| +0x37e0 | 4 | Default leave function | 0x7feb46 |
| +0x37e4 | 4 | Default exit function | 0x7feb50 |
| +0x37e8 | 4 | Pointer to the default slot values (array of u32) | 0x7feb5b, 0x7fea27 |
| +0x37ec | 2 | Number of default slot values (s16) | 0x7feb61, 0x7fea1e |
| +0x37ee | 4 | Hook function, called with 0 when the clicked widget id equals +0x37f2 | 0x7fcce4..0x7fcd2f |
| +0x37f2 | 4 | Widget id for the hook | 0x7fcd25 |
| +0x37f6 | 4 | Unknown. The side bar sets it to 0x424. | 0x75db4f |
| +0x37fa | 4 | Screen key to promote to slot 0 at the end of a loop pass | 0x7fd388..0x7fd39e |
| +0x37fe | 12 | Network write buffer {ptr, pos, capacity} | 0x7fc3a1, 0x7fd60b |
| +0x380a | 16 x 2 | s16 number of records of each channel | 0x7feb7a, 0x7fc484 |
| +0x382a | 2 | Current channel (s16) | everywhere |
| +0x382c | 2 | Forced next channel, -1 = none (network) | 0x7fc41f, 0x7fca7b |
| +0x3834 | 4 | 1 while the loop calls an input function with -1 after a pop | 0x7fcf78, 0x7fcf98 |
| +0x383c | 4 | 1 while `FUN_007fc7e0` runs | 0x7fc7fa, 0x7fdbb3 |
| +0x3840 | 4 | Network session object, 0 in a local game | 0x7fc901 |
| +0x3844 | Display | Screen manager 0 (the local one) | 0x7fc3b2, 0x7fc478 |
| +0x132ea5 | Display | Screen manager 1 (remote player) | 0x7fc3c4 |
| +0x262506 | 4 | Function called once per loop pass, 0 = none | 0x7fd557..0x7fd561 |
| +0x26278e | 2 | Injected action id. Input functions use it when no widget is clicked. Code sets it, calls an input step and clears it. | 0x5133c0/0x5133cb, 0x81c914 |
| +0x262798 | 4 | Time of the last click (C `time()`) | 0x7fcc5e..0x7fcc80 |
| +0x26279c | 2 | Idle timeout in seconds, 0 = off | 0x7fcc54, set at 0x5cdf14 |
| +0x26279e | 2 | Minimum seconds since the loop started before the timeout acts | 0x7fccb2 |

The constructor `FUN_007fc380` zeroes all of these, sets channel 0 to one record, sets record +0x16 to 1, record +0x22 to -1 and record +0x28 to screen manager 0 (0x7fc474..0x7fc4cd).

### 1b. Record of a channel (0x30 bytes)

Base for slot 0 of channel c is `S + 0x7d4 + c*0x300`. A channel holds several records only when several human managers share a channel. One local manager uses slot 0 only.

| Offset | Size | Meaning | Evidence |
|---|---|---|---|
| +0x00 | 2 | Player id (s16). 0 = local player. The loop draws locally only when it is 0. | 0x7fca2c..0x7fca37, 0x7ffb83 |
| +0x02 | 4 | Screen key. Pointer to the manager of this record (0 on the start screen). `FUN_007fe600` uses the default screen only when it is 0. | 0x7fe622, 0x75db1e, 0x75de0e |
| +0x06 | 4 | First node | 0x7fe62d, 0x7fef23 |
| +0x0a | 4 | Last node | 0x7fe6d6, 0x7fe940 |
| +0x0e | 4 | Current node | 0x7fe6a2, 0x7fee8d |
| +0x12 | 2 | Number of nodes with the flag set (s16) | 0x7fe8f9, 0x7fce9d |
| +0x14 | 2 | Number of nodes (s16), at most 100 | 0x7fe7be, 0x7fe996 |
| +0x16 | 4 | Rebuild flag. 1 = run the build function on the next loop pass. | 0x7fea0f, 0x7fc94d, 0x7fc994 |
| +0x1a | 4 | Away flag. 1 = this record takes no input in this loop run. | 0x7fc892, 0x7fcc04, 0x7fcce2 |
| +0x1e | 4 | Done flag. 1 = this manager pressed continue. | 0x7fc898, 0x7fd25a..0x7fd27b, 0x7fd543 |
| +0x22 | 2 | Set to -1 by push. Written by `FUN_005680b0`. Meaning unknown. | 0x7fe9c6, 0x5680c2 |
| +0x24 | 4 | Set to 0 by push. Written by `FUN_005680b0`. Meaning unknown. | 0x7fe9df, 0x5680db |
| +0x28 | 4 | Pointer to the screen manager of this record | 0x7fc47e, 0x7ffbbe |
| +0x2c | 4 | Time when the loop started for this record, -1 after reset | 0x7fc8b2, 0x7ffbda |

### 1c. Screen node (0x300 bytes, operator new at 0x7fe872, zeroed at 0x7fe88c)

| Offset | Size | Meaning | Evidence |
|---|---|---|---|
| +0x000 | 4 | Build function, `void build(void)` | 0x7fe89c, called 0x7fc9e6 |
| +0x004 | 4 | Leave function, `void leave(void)` | 0x7fe8a5, called 0x7fe86b, 0x7fce61 |
| +0x008 | 4 | Input function, `int input(int clicked)` | 0x7fe8b2, called 0x7fcd46 |
| +0x00c | 4 | Exit function, `void exit(void)`. The loop calls it for every node when it ends. It also marks a node that a continue skips. | 0x7fe8af, 0x7fd959..0x7fd960, 0x7fd1b8 |
| +0x010 | 4 | Flag. Marks a node of a chain (see item 3). | 0x7fe8bf, 0x7fce92 |
| +0x014 | 60 x 8 | Slots. Each is {u32 value, u32 owned}. Owned = 1 means the stack frees the value. | 0x7ff31b, 0x7ff444 |
| +0x1f4 | 4 | Previous node | 0x7fe93a |
| +0x1f8 | 4 | Next node | 0x7fe920 |
| +0x1fc | 260 | Title, a C string. Push sets it to "\" (string 0x9c3730). | 0x7fe8b5..0x7fe8e4 |

## 2. Pseudocode of the stack functions (one local player)

```c
/* FUN_007fe600, thiscall, ret 0x14 */
int push(S, build, input, flag, leave, exitfn) {
    R = &S->rec[ch][0];  used_default = 0;
    if (R->key == 0 && R->first == 0 && S->def_build != 0) {      /* 0x7fe62b */
        build = S->def_build; input = S->def_input; flag = S->def_flag;
        leave = S->def_leave; exitfn = S->def_exit; used_default = 1;
    }
    if (build == 0 || input == 0) { assert(0x4ed); return 0; }    /* 0x7feab0 */
    if (R->current && flag && R->first) {                         /* 0x7fe6a2 */
        for (n = R->first; n; n = n->next)
            if (n->flag && n->build == build) {                   /* 0x7fea59 */
                old = R->current;
                for (; n; n = n->next) if (n->flag) R->current = n;
                if (R->current->build != build) R->current = old;
                return 0;                                         /* no push */
            }
    }
    if (R->current != R->last) {                 /* drop forward history, 0x7fe6e2 */
        R->last = R->current; n = R->current->next; R->last->next = 0;
        while (n) { nx = n->next; free_slots(n); free(n); R->count--; n = nx; }
        R->current = R->last;                    /* flagged count is not lowered */
    }
    if (R->count > 100) {                        /* 0x7fe7be */
        n = R->first;
        if (n->flag) return 0;
        R->first = n->next; R->first->prev = 0;
        free_slots(n); free(n); R->count--;
    }
    if (R->current && R->current->leave) R->current->leave();     /* 0x7fe86b */
    n = calloc(0x300);
    n->build = build; n->leave = leave; n->input = input;
    n->exitfn = exitfn; n->flag = flag; strcpy(n->title, "\\");
    if (flag) R->flagged++;
    if (R->last) { R->last->next = n; n->prev = R->last; }
    R->last = n; if (!R->first) R->first = n;
    R->current = n; R->count++;
    R->away = 0; R->f22 = -1; R->f24 = 0;
    if (flag) R->done = 0;
    R->rebuild = 1;                                               /* 0x7fea0f */
    if (used_default)
        for (i = 0; i < S->def_count; i++) set_slot(i, S->def_values[i], 0);
    return 1;
}
/* free_slots(n): for i in 0..59: if (n->slot[i].value && n->slot[i].owned) free(value) */
```

`FUN_007fe4a0(key_filter, build, input, flag, leave, exitfn)` calls push for every record of every channel whose key equals the filter, or for all when the filter is 0. It swaps each later record into slot 0 for the call and back (0x7fe4ed..0x7fe5c1). It returns 1 (0x7fe5ed). For one player it is one push.

```c
/* FUN_007ff990, thiscall, ret 8. (key, channel) */
void remove(S, key, chan) {
    slot = 0;
    if (chan == -1) {                          /* search, 0x7ff9ad */
        find chan 0..15 and slot (from last to 0) with rec[chan][slot].key == key;
        if none: return;
    }
    if (ch == chan) ch = 0;
    saved = ch; ch = chan;
    if (slot == 0) { if (current && current->leave) current->leave(); }
    else swap(&rec[chan][slot], &rec[chan][0], 0x30);
    free_all(S);                               /* FUN_007fef10 on 0xb74450 */
    FUN_0055f290(chan);                        /* snapshot flag, network */
    if (DAT_00dd6bf4) DAT_00dd6bf4(rec[chan][0].player);
    ch = saved;
    shift rec[chan][1..n-1] down by one, zero the last, count[chan]--;
    if (count[chan] != 0) { rec[chan][0].rebuild = 1; return; }
    if (chan == 0) {                           /* 0x7ffb7e */
        rec[0][0] = {player 0, key 0, nodes 0, counts 0, rebuild 1,
                     away 0, f22 -1, f24 0, display = screen manager 0, f2c -1};
        count[0] = 1;
        push(S, def_build, def_input, def_flag, def_leave, def_exit);
    } else send packet 4;                      /* network */
}

/* FUN_007fef10, thiscall, no args: free every node of rec[ch][0] */
void free_all(S) {
    R = &S->rec[ch][0];
    for (n = R->first; n; n = R->current) {   /* current is used as the cursor */
        R->current = n->next; free_slots(n); free(n); R->count--;
    }
    R->first = R->last = R->current = 0; R->count = 0; R->flagged = 0;
}

/* FUN_007ff270, thiscall, ret 0xc. (slot, value, copy) */
void set_slot(S, i, value, copy) {
    n = current; if (!n) { assert(0x6da); exit; }
    if (i < 0 || i > 59) { assert(0x6e0); return; }
    if (n->slot[i].owned) { free(n->slot[i].value); n->slot[i].owned = 0; }
    if (value && copy) {
        p = malloc(strlen(value) + 1);         /* 0x7ff372 */
        if (!p) { assert(0x6f7); return; }
        strcpy(p, value); n->slot[i].value = p; n->slot[i].owned = 1;
    } else { n->slot[i].value = value; n->slot[i].owned = 0; }
}

/* FUN_007ff010, thiscall, ret 4 */
u32 get_slot(S, i) {
    n = current; if (!n) { assert(0x689); exit; }
    if (i < 0 || i > 59) { assert(0x68f); return 0; }
    return n->slot[i].value;
}

/* FUN_007feeb0, thiscall, ret 4: set title */
void set_title(S, s) { n = current; if (!n) return;
    if (s) strcpy(n->title, s); else n->title[0] = 0; }   /* no length check */

/* FUN_007fee80, thiscall: get title */
char *get_title(S) { n = current; return n ? n->title : ""; }  /* "" = 0xad9c64 */
```

Small queries, from the assembly:

- `FUN_007febb0`: returns 1 when the current node has no next node (0x7fec42). Exits the program when no current node.
- `FUN_007fec60(arg)`: returns 0 when the current node has a previous node and (arg != 0 or its flag is 0). Else 1 (0x7fecf2..0x7fed24). The start screen enables its Back button when it returns 0 (0x81bcd6..0x81bcdf).
- `FUN_007fed30`: returns the flag of the current node. `FUN_007fedd0`: returns 1 when the current node has an exit function.
- `FUN_007feb20`: stores the default screen, see 1a.
- `FUN_00803d50`: sums the flagged-node count (+0x12) of all 256 records (0x803d54..0x803d70).

Ghidra and the earlier inventory show "unaff_retaddr" for the argument of 0x7fec60, 0x7ff010 and 0x7ff270. The assembly reads `[esp+0x204]` after `sub esp,0x200`, so each takes its first stack argument normally.

## 3. Main loop `FUN_007fc7e0` (SCREEN_MANAGER::monitor), one local player

The loop runs until every channel is done or an input function returns -7. It returns 1 when the last result was -5 (0x7fdbb9). Callers: 0x5cd2a3, 0x5cde6e, 0x5ce051, 0x6b433e, 0x6b481a, 0x6b49c7, 0x81ba47.

Entry, once (0x7fc7e0..0x7fc92a):

1. `S+0x383c = 1`.
2. Screen manager +0x12f559 = (dword 5 of the 32-byte block at 0xae3158 == 0x7e0), for both screen managers. Inference: 0x7e0 is the green mask of RGB565, so this flag selects 565 or 555 colour. `FUN_005eef80` uses it to split colours (0x5eef8c).
3. Screen manager 0 +0x12f4fa = 0. `t = time()` (0x7fc84e).
4. For each record: if rebuild == 0 then away = 1, done = 1. Else away = 0, done = 0, +0x2c = t, and the record moves to slot 0 when slot 0 is away (0x7fc889..0x7fc8cb).

Each pass (0x7fc92c):

1. For each channel with rebuild != 0 and records (0x7fc93d..0x7fca70):
   - For channel 0, clear the mouse buttons `DAT_00b67a34` and `DAT_00b67a38`.
   - rebuild = 0.
   - `FUN_0055f7c0(1)` on the record's screen manager. This is the reset.
   - Screen manager +0x12f53d = 1 (no draw during build).
   - `current->build()` with no arguments. The return value is not read.
   - +0x12f53d = 0, +0x12f4fa = 0.
   - Player 0: `FUN_0055f300(-1,-1,-1,-1)`, the screen draw. Other players: `FUN_005622a0` (network send).
2. `FUN_0068d520()` (inference: Windows message pump).
3. No network: walk channels 1..15 for one with records and away == 0. None is found for one player, so ch = 0 (0x7fcb93..0x7fcbe3).
4. clicked = -1. If ch == 0 and record away == 0: clicked = `FUN_0055f620()` on screen manager 0 (0x7fcc0e).
5. If ch has no records or no current node: error box 0x1c7 ("invalid player") and go to step 10.
6. Idle timeout (0x7fcc54): if `S+0x26279c` != 0, a click resets the last-click time. When both time limits pass, r = -5 and the input function is not called.
7. If away != 0, r keeps the value from before (0 at the start of a pass) and no input function runs.
8. If the hook `S+0x37ee` is set: id = clicked != -1 ? (s16) widget[clicked]+0x08 : 0. If id == 0, id = `S+0x26278e`. If id == `S+0x37f2`: `hook(0)`, r = -4. Else r = `current->input(clicked)` (0x7fcd46). The input function runs on every pass, also with clicked = -1.
9. Act on r (jump table 0x7fdbcc, index r + 13):

| r | Case | Effect |
|---|---|---|
| -1 | 0x7fd3c6 | leave(). If a first node exists: current = first, rebuild = 1. Home. |
| -2 | 0x7fd423 | leave(). If current has a previous node and current flag == 0: current = previous, rebuild = 1. History back, nothing is freed. |
| -3 | 0x7fd474 | leave(). If current has a next node: current = next, rebuild = 1. History forward. |
| -4, -11 | 0x7fd3a9 | rebuild = 1. Rebuild the current screen. |
| -5, -6, -8, -13 | 0x7fd164 | Continue. See below. |
| -7, -12 | 0x7fd4fe | As the default. -12 also sends packet 4 first (0x7fcd79, network). -7 ends the loop. |
| -9 | 0x7fce46 | Pop. See below. |
| -10 | 0x7fcfc9 | Pop the chain. See below. |
| other (0 and up) | 0x7fd4fe | If clicked != -1 and rebuild == 0: `FUN_00802d00(0,0)`, which redraws the whole screen. |

   Pop (-9): leave(). From the last node backwards, free nodes until one with the flag set is freed. Lower the flagged count for it. Each freed node sets current = last = its previous node. If a node to free has no previous node: r = -5, flagged count = 0, and act on -5 (0x7fcfa9). After the pop: `S+0x3834 = 1`, r = `current->input(-1)`, `S+0x3834 = 0`. If r == 0: rebuild = 1. Else act on r again (0x7fcf73..0x7fcfa4).

   Pop the chain (-10): leave(). From the last node backwards, find a flagged node whose previous node is missing or has no flag. If it is the first node: r = -5, flagged count = 0. Else free it and all nodes after it, current = last = its previous node. Then call input(-1) as for -9 (0x7fcfe6..0x7fd15f).

   Continue (-5, -6, -8, -13), 0x7fd164: if the flagged count > 0, search from the last node back for a flagged node with no exit function (any flagged node for -13). If found: current = that node, rebuild = 1, r = 0, go to step 10. Else restore current. Then: -6 sets done = 1 and away = 1. Others flip done when away == 0. rebuild = 1. Then the record loop gives the next manager whose away and done are 0 the slot 0. With one manager, -5 sets done = 1, and the loop ends at step 10.

10. Done check (0x7fd525): done when every channel with records has done != 0 in slot 0. Call `S+0x262506` if set. If not done and r != -7: next pass.

Exit (0x7fd5eb):

1. Write opcode 7 with 0, 0 and "Processing, Please Wait ..." (0xa7dda0). Player 0 applies it on screen manager 0 (0x7fd862). This draws the message box `FUN_005e76f0`.
2. For each record: call exit() of every node from first to last. Call leave() of the current node. `FUN_0055f7c0(1)`. current = 0, rebuild = 0, `FUN_007fef10` (free all). The stack is empty after the loop.
3. ch = 0, screen manager 0 +0x12f4fa = 1, `S+0x383c = 0`.

Arguments: build, leave and exit take none and return nothing that is read. Input takes one stack argument, the s16 widget index from `FUN_0055f620` in a 32-bit slot, or -1. All are called without ECX setup, so they are plain cdecl functions (0x7fcd45..0x7fcd49).

The meaning of the node flag (inference from -9, -10 and continue): a flagged node is a step in a chain of screens that the game forces on the manager. Continue returns to a pending flagged screen before it ends the loop.

## 4. Widget setters (opcode 8)

Each setter writes opcode 8, the widget index and the sub-op, then the local side runs `FUN_00564080`. The opcode switch is at 0x56412a (table 0x567a1c). Opcode 8 is case 0x56708c. Before the sub-op switch it reads the sub-op with `FUN_00567be0`. If screen manager +0x12f53b != 0, it applies only when the area and widget slot counts equal +0x12f53b and +0x12f539 (0x5670a8..0x5670d1). Sub-op table at 0x567a88. Widget base = screen manager + 0xBA95E, stride 0x18c.

| Sub-op | Setter | Value | Widget field | Redraw | Name I would give |
|---|---|---|---|---|---|
| 0 | 0x800580 | u32 | +0x0c flags (0x567113) | No | widget_set_flags |
| 1 | 0x800890 | 4 x s16 | +0x10..+0x1c and +0x28..+0x34 through `FUN_00567ae0` and `FUN_00567ac0` (0x567189, 0x5671bc) | No | widget_set_rect |
| 2 | 0x800d30 | u32 | +0x38 style (0x5671f9) | No | widget_set_style |
| 3 | 0x801040 | u16 | +0x72 colour, then `FUN_005eef80(+0x12f559, 1)` (0x567233) | No. 5eef80 recomputes the colour parts. | widget_set_colour |
| 4 | none found | u16 | +0x74 highlight colour, then `FUN_005eef80(.., 2)` (0x567294) | No | widget_set_highlight |
| 5 | 0x801350 | u32 | +0x3c text flags (0x5672f6) | No | widget_set_text_flags |
| 6 | 0x801660 | u8 | +0x76 font, sign-extended to s16 (0x567333) | No | widget_set_font |
| 7 | 0x801960 | u16 | +0x78 text colour, then `FUN_005eef80(.., 4)` (0x56736e) | No | widget_set_text_colour |
| 8 | 0x801c70 | string | `FUN_005eed50(text, 0)`: set text, redraw argument 0 (0x5673ad) | No | widget_set_text |
| 9 | 0x801fc0 | u32 | +0x08 action id (0x5673ea) | No | widget_set_id |
| 10 | 0x8022d0 | u32 | +0x48 action data (0x567425) | No | widget_set_data |

`FUN_00567ac0` stores its four arguments to +0x28, +0x30, +0x2c, +0x34 in that order. `FUN_00567ae0` stores them to +0x10, +0x14, +0x18, +0x1c. The caller pushes different reads for the two calls. The exact pairing of x and y in the copy needs a check in a differential test.

None of the setters redraws. The redraw comes from the main loop: after a click that does not rebuild, it calls `FUN_00802d00(0,0)` (0x7fd520).

Other opcodes, local side:

- `FUN_00802d00`, opcode 9, case 0x567439: screen manager +0x12f551 = 1, +0x12f4fa = 0. If laid out (+0x12f545): `FUN_0055f300(-1,-1,-1,-1)`. A full redraw.
- `FUN_00800000`, opcode 0x11, case 0x5677b8: sets `DAT_00b67a0c` = 0. Inference: this clears a queued click.
- `FUN_00802f80(text, a, b, channel)`, opcode 7, case 0x567013: reads u32 a, u32 b and the text. Sets +0x12f551 = 0 and calls `FUN_005e76f0(text, a, b)`, the progress or message box. If `DAT_00b77c8c` != 0 it calls `FUN_005e8170(1)` (cursor).

## 5. `FUN_00804030` (12 arguments, ret 0x30)

It creates one free button on the current screen manager and returns the widget index (0x804030..0x804090). It does not create an area. Mapping to widget create `FUN_0055f990`:

`FUN_00804030(x1, y1, x2, y2, style, colour, text_flags, font, text_colour, text, id, data)` = `widget_create(flags 2, x1, y1, x2, y2, column 0, row 0, style, colour, colour, text_flags, font, text_colour, text, 0, id, data, area -1)`.

The highlight colour equals the colour. Its siblings, all thiscall on S:

- `FUN_008040a0(area, column, row, style, colour, text_flags, font, text_colour, text, id, data)`, ret 0x2c: button (flags 2) in an area cell, highlight = colour.
- `FUN_00804100(area, column, row, style, colour, text_flags, font, text_colour, text)`, ret 0x24: label (flags 1) in an area cell, highlight 0, id 0, data 0.
- `FUN_00804160`, ret 0x2c: label like 0x804100, reads its text through a pointer (0x80418f). Not used by the start screen.
- `FUN_00803fd0`, ret 0x2c: free label (flags 1). Not used by the start screen.
- `FUN_00803f30(colour, text_colour, text)`, ret 0xc: heading. `widget_create(1, 100, 10, 790, 70, 0, 0, 0x30, colour, 0, 0xc, 7, text_colour, text, 0, 0, 0, -1)`.
- `FUN_00803f80(text, unused)`, ret 8: sub-heading. `widget_create(1, 100, 80, 790, 125, 0, 0, 2, 0, 0, 0xc, 6, DAT_00aebdec, text, 0, 0, 0, -1)`.

## 6. First real screen: the start screen ("Setup Game")

Path: WinMain `FUN_0068d370` calls `FUN_005cc6b0` (0x68d4dd), which calls `FUN_005cccd0` (0x5cc6ba), which calls `FUN_0081b890` (0x5cd0bd). `FUN_0081b890` pushes the start screen with `FUN_007fe4a0(0, 0x81bab0, 0x81bd70, 0, 0, 0)` (0x81ba1a) and runs the loop (0x81ba47). Then it sets the default screen to build 0x81ff10, input 0x820950, five slot values at 0xdd7148 (0x81ba97).

- Build: `FUN_0081bab0`. Input: `FUN_0081bd70`. Leave: none. Exit: none. Flag: 0.
- Before the loop, slot 0 = 1 when `[DAT_00ae3138]+0x213` == 0, else 0 (0x81ba23..0x81ba3d). Inference: a first-run flag that opens the settings screen.

### 6a. Calls of the build function, in order

1. Construct 25 menu items of 0xd4 bytes on the stack (`FUN_009463af`, item constructor `FUN_004568f0`: font = -1, colour = 0). `DAT_00dd7128 = 0`.
2. `FUN_0041bfb0()`: background. If the title is "\" or empty: when `DAT_00989858` == 0, title = ":". Else pick a random picture with `FUN_0090cfc0` from a table at 0x989864 that the program fills at run time. Title = `FUN_0090bde0("CM3_PICTURES", name)` through `FUN_007feeb0` (0x41c0b9). The screen draw then loads that file.
3. `FUN_0066f4e0(0xdd70a8, "Championship Manager 2001/02")`.
4. `FUN_00803f30(DAT_00ae31a8, DAT_00ae3184, 0xdd70a8)`: heading, font 7.
5. `FUN_0075db00(4, 0)`: side bar. On the start screen `DAT_00dd7118` is 0, so its mode becomes 1 (0x75dbf1). It makes, in order:
   - Area (0,0)-(89,599), 1 column, 1 row, flags 1, colour 0, owner -1.
   - Picture widget: flags 0x400, style 1, font 1, text_flags 0xc, text = `FUN_0090bde0("CM3_DATA", "game.mbr")`, in that area (0x75dc67).
   - Area (5,10)-(85,590), 1 column, 13 rows, flags 1 (0x75dc95).
   - Row 0: label "Version\n<%s - version>", style 0x1061, font 1, text colour `DAT_00aebdec`. The version argument comes from `FUN_0090d130` with arguments that I could not trace (unknown).
   - Area (5,10)-(85,590), 2 columns, 13 rows, flags 1 (0x75dd25).
   - Row 1: buttons "<<<" id -2 (column 0) and ">>>" id -3 (column 1), style 0x1021, font 1, text colour `DAT_00ae3184`. When `DAT_009cc2c4` != -1 the text is the character 0x28 or 0x29 instead. `DAT_009cc2c4` is -2 in the file.
   - Row 2: "Add\nManager". Button id 0x3fb when `DAT_00dd711c` and `FUN_00838c60()` and not `FUN_00838de0()` and `[DAT_00b63c98]+8 == 0`. Else a label with text_flags 0x2c (inference: greyed).
   - "Chat", id 0x421: only when `DAT_00b77c90` != 0 and the record key is set. Not on the start screen.
   - Row 3: "Restart\nGame". Button id 0x42f when the player id is 0 and `FUN_00803d50()` == 0, else label 0x2c.
   - Row 4: "Exit\nGame". Button id 0x402 when `FUN_00803d50()` == 0, else label 0x2c.
   - It also sets `S+0x37ee` = 0x823930, `S+0x37f2` = 0x423, `S+0x37f6` = 0x424 (0x75db3b..0x75db4f), and calls `time()` into 0xb74310.
   - The rest of 0x75db00 (game modes, news, chat, waiting players, 0x75e04b to 0x7611ff) needs a manager record. The start screen does not reach it.
6. `FUN_0066f4e0(0xde1f64, "Setup Game")`, then `FUN_00803f80(0xde1f64, 0)`: sub-heading, font 6.
7. Fill 9 items with `FUN_0066f4e0`. Item layout: +0x00 s16 font (-1 = default), +0x02 s16 text colour (0 = default), +0x04 text, +0xcc id (widget +0x08), +0xd0 data (widget +0x48).

| Item | Text | Id |
|---|---|---|
| 0 | Start New Game | 1 |
| 1 | Quick Start Game | 2 |
| 2 | Restore Saved Game | 3 |
| 3 | Delete Saved Game | 4 |
| 4 | Network Play | 5. When `DAT_00a801c4` == 0 (command line `-nonetwork`), id 0 and text colour `DAT_00ae317e`. |
| 5 | Game Settings | 6 |
| 6 | Hall Of Fame | 7 |
| 7 | Game Credits | 8 |
| 8 | Web Sites | 9 |

8. `FUN_005ed300(110, 145, 780, 535, 2, 6, 3, 9, items, 0, 0)` (cdecl, 11 arguments): menu builder. It copies the items, sorts them by text when argument 11 != 0 (0x5ed3c0), creates area (110,145)-(780,535) with 2 columns, 6 rows, flags 1, colour 0, owner -1 (0x5ed524). Then for each item, a button: `widget_create(2, x1, 0, x2, 0, column, row, style, DAT_00aebe04, DAT_00aebe04, 0xc, font, text_colour, item+4, 0, item+0xcc, item+0xd0, area)`. Style is 0x822 when the item's bit in argument 10 is set, else 0x22. The default text colour is `DAT_00aebdec` for a set bit, else `DAT_00ae3184`. Font -1 becomes argument 7 (3). Column and row fill row by row. x1 and x2 are 0 unless the item count is less than the column count (centred single row, 0x5ed56d..0x5ed5b2).
9. `FUN_005edcc0(back, 0)` with back = (`FUN_007fec60(0)` == 0). Area (100,555)-(790,590), 2 columns with weights {3,1}, 1 row, flags 1 (0x5edd10). Column 0: "Back", button id -2 through `FUN_008040a0` (style 0x30, colour `DAT_00ae317e`, text_flags 0xc, font 3, text colour `DAT_00ae3184`) when back != 0, else label text_flags 0x2c. Column 1: "Next", id -3, same rule with argument 2 (0 here, so a label).
10. If slot 0 != 0: area (0,0)-(799,599), 1 column, 1 row, flags 2, colour 0, owner -1 (0x81bd28). Purpose unknown.
11. Destroy the items (`FUN_009462bb`, destructor `FUN_005a53c0`).

### 6b. Where the texts come from

All texts are English literals in the original. The build copies them through `FUN_0066f4e0(dst, format, ...)`, which has 9,191 call sites. It is a printf-like function that also handles `{...}` and `[...]` tags (0x66f7fe..0x66f832). It does not look up a language table in the part I read. A global at 0x9eb2dc (-1 in the file) selects one of its paths. Whether the path for other values changes the text is unknown.

Colour globals, with the values that `include/gui_utils.h` of the port already gives: `DAT_00aebe04` navy, `DAT_00aebdec` yellow, `DAT_00aebdea` black, `DAT_00ae3184` light grey, `DAT_00ae31a8` red, `DAT_00ae317e` grey.

### 6c. The input function `FUN_0081bd70`

1. If slot 0 != 0: set slot 0 to 0, call `FUN_00823890(0)`, return -4 (0x81bd82..0x81bdb0).
2. Command line `-load` with a ".sav" or ".qst" file, and the files "quick_start.cfg" and "auto_setup.cfg", start a game at once (0x81bdb1..0x81c4bb). A port can leave this out at first.
3. `DAT_00b77c84` and `DAT_00dd6be0` select a pending load or quick start (0x81c4c0..0x81c7a0). Not reached on a plain start.
4. id = clicked != -1 ? (s16) widget[clicked]+0x08 : 0. If id == 0, id = `S+0x26278e` (0x81c8d7..0x81c920). Then:
   - 1: `FUN_0081ce00` (start new game, pushes 0x826d20/0x826e60 or loads the database), return -4.
   - 2: quick start file dialog `FUN_0058f500`, return -4.
   - 3: restore dialog `FUN_0058f500` with ".sav", return -4.
   - 4: delete dialog, return -4.
   - 5: network, `FUN_007ff500` and push 0x81edd0/0x81ef70, return -4.
   - 6: `FUN_00823890(1)` (settings), return -4.
   - 7: push 0x825d10/0x826690 (hall of fame), slots 0..2 = 0, return -4.
   - 8: `FUN_004e5920` (credits), return -4.
   - 9: `FUN_005134c0` (web sites), return -4.
   - -3: return 0.
   - Else: r = `FUN_007615d0(clicked)`. If r == 0, r = `FUN_007643d0(clicked)`. Return r. Inference: these two handle the side bar and the Back button.

### 6d. Functions the start screen needs that the port does not have

The port has `FUN_0090bde0`, area create, widget create, the reset and the screen draw. It lacks:

- Screen stack: `FUN_007fe4a0`, `FUN_007fe600`, `FUN_007feb20`, `FUN_007fec60`, `FUN_007fee80`, `FUN_007feeb0`, `FUN_007fef10`, `FUN_007ff010`, `FUN_007ff270`, `FUN_00803d50`, `FUN_007fc7e0`.
- Widget helpers: `FUN_00803f30`, `FUN_00803f80`, `FUN_008040a0`, `FUN_00804100`.
- Screen parts: `FUN_0041bfb0`, `FUN_0075db00` (mode 1 part only), `FUN_005ed300`, `FUN_005edcc0`, `FUN_004568f0`, `FUN_005a53c0`.
- Text: `FUN_0066f4e0`.
- Input: `FUN_0055f620` and its mode handlers, `FUN_0081bd70`, `FUN_007615d0`, `FUN_007643d0`.
- Run-time state: `FUN_0090cfc0` (random), the picture table at 0x989864, `FUN_00838c60`, `FUN_00838de0`, `DAT_00dd711c`, `DAT_00b63c98`, `DAT_00a801c4`, `DAT_00989858`.
- Targets of the menu items: `FUN_0081ce00`, `FUN_0058f500`, `FUN_00823890`, `FUN_004e5920`, `FUN_005134c0`, and the pushed screens.
- `FUN_005e76f0` (message box) for the loop exit.

The Pictures folder holds 61 RGN files, and Data holds `game.mbr` and `default_pic.rgn`.

## 7. Widget fields +0x08, +0x40, +0x48

- +0x08 (argument 16): action id. The start screen passes 1..9, -2 (Back, "<<<"), -3 (Next, ">>>") and ids such as 0x3fb, 0x402, 0x42f. Input functions read it with the pattern at 0x81c8ed..0x81c920 (742 reads of offset 0xba966 in the binary). The main loop compares it with `S+0x37f2` (0x7fcd0b). Sub-op 9 sets it.
- +0x48 (argument 17): action data. Menu items pass item+0xd0. `FUN_0049d930` passes a pointer. 286 reads of offset 0xba9a6, at the same places as the id reads (example 0x4169a5 and 0x4169eb). Sub-op 10 sets it. Inference: a pointer or number that the input function uses with the id, such as the player or club of a row.
- +0x40 (argument 15): maximum text length of a text entry widget (flags 0x08). `FUN_005617c0` adds a character only while strlen(text) < +0x40 (0x561d53..0x561d5e). A negative value means "clear the text on first edit": it sets the text to "", +0x44 = 0, +0x180 = 1, and +0x40 = abs(+0x40) (0x561b07..0x561b41). Only 4 reads exist. All callers seen pass 0.

## 8. Open doubts

- The 4 values of sub-op 1 and the order in which they reach +0x28..+0x34: see item 4.
- The meaning of record +0x22 and +0x24, and of `S+0x37f6`.
- The meaning of the area with flags 2 that the start screen adds when slot 0 is set (6a step 10).
- Whether `FUN_0066f4e0` translates text when 0x9eb2dc != -1.
- The version text of the side bar: the arguments of `FUN_0090d130` at 0x75dcb3.
- The picture table at 0x989864 is empty in the file. The code that fills it was not traced.
- What `FUN_007615d0` and `FUN_007643d0` return for Back and the side bar buttons. They decide whether Back gives -2 or -9.
- The default screen 0x81ff10/0x820950 and its 5 slot values at 0xdd7148 were not read.
- `FUN_0068d520` is taken to be the message pump. Not read.

Unused or network only, which a single-player port can leave out: channels 1..15, records in slots 1..15 (hot-seat managers), `S+0x382c`, `S+0x3840`, packet 4 for -12, `FUN_005622a0`, `FUN_0055f290`, the voice object `DAT_00dd7eec`, and the network writes of every setter. A local port can apply each setter's field write directly.
