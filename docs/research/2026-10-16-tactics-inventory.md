# Tactics inventory for the Best XI formation list

Date: 2026-10-16. Source: `clean-3968/cm0102.exe` (Ghidra via REA) and the 40 `.pct` files in `clean-3968/Data`.
Scope: what the game's own tactics list shows, where its tactic blocks live, the `.pct` format, and a safe read path for a view-only Best XI screen.

## Summary for the build

- The tactics screen has no separate "formation" drop-down. The list of formations is the "Set To Default" submenu of the "Tactics" menu button on the tactics screen. It lists the 17 preset tactics, in a fixed order, with no mark on the current one.
- The 17 preset blocks stay in memory for the whole session, in the tactics store at `[0xDE1D08]`. Each block is 0x18E3 bytes. Name at +0, 11 slot masks (u16) at +0x12D.
- Read them in place. Do not call `FUN_005fbd00` for an AI manager. Do not call the file loader `FUN_005b5d10`.
- The `.pct` masks start at file offset 0x102, not 0x103. The second mask set starts at 0x123. The name field in all 40 shipped files is empty.

## 1. The game's tactics list

### Where the list is built

`FUN_00899ea0` is the build function of the tactics screen (it uses the `tactics_screens.cpp` assert path 0xA8B94C). From 0x89A3A7 on, it builds the "Tactics" menu button (string 0x98E840) and its items, in this order:

1. "Team Instructions" (0xA8BFA4)
2. "Penalty Takers" (0xA8BF94)
3. "Set To Default" (0xA8BF84). It opens a submenu, made by `FUN_0055fba0(…, uVar9)`.
4. The submenu gets one item per preset: `for (i = 0; i < *(int*)([0xDE1D08] + 0x5FD); i++)`, action code 2, item parameter `i` (call `FUN_0055f990(…, 0, 2, i, submenu)`).
5. Separator, then "Load" (0xA7E7F4, action 3), "Load Preset" (0xA8BF78, action 4, only when byte `[0xB7434A]` is not 0), "Save" (0xA7E7DC, action 5), "Save As" (action 6), "Save For" submenu (other human managers, only when `FUN_00838d10(0) > 1`), "Delete" (action 9), "Unlock" or lock (actions 10/11).
6. After a separator: up to 4 recent tactic files, text "%d. %s" (0xA7E728), action 0xC. The names come from `FUN_005fc760(i, buf, 0x104, 1, 0, 1)` for i = 0..3 (see 1.4).

"Load" opens a file dialog on the tactics folder. It is not a list in the menu.

### Text of each preset item

For each index the code gets the block with `FUN_00896570(i, 1)` and uses the string at block +0. It compares the name (`_strcmpi`) with eight known names ("4-4-2 Defensive", "4-4-2 Attacking", "4-4-2 Diamond", "3-5-2 Defensive", "3-5-2 Attacking", "5-3-2 Defensive", "5-3-2 Attacking", "Sweeper"). On a match it puts the literal through `FUN_0066f4e0` (the text formatter). Otherwise it copies the block name as is. The shown text is therefore the block name at +0 (0x89A3A7 to 0x89A6F0).

### Order and names (the preset store)

`FUN_00893dc0` (thiscall, ECX = the 0x609-byte store that `FUN_0082a9c0` saves in `[0xDE1D08]`, at 0x82A9C0 call site line "DAT_00de1d08 = FUN_00893dc0()") loads the presets. It sets the count `+0x5FD = 0x11`, allocates 17 blocks of 0x18E3 at `+0x5F9`, and for each file calls `FUN_005b5d10(path, 0, 1)` with `FUN_0090bde0("CM3_TACTICS", file, 0, 1)` as the path. After a good load it overwrites the block name at +0 with a literal. The final order and names:

| Index | File | Name (block +0) | Name address |
|---|---|---|---|
| 0 | 442_default.pct | 4-4-2 | 0xA782B0 |
| 1 | 442_defensive_default.pct | 4-4-2 Defensive | 0xA8B758 |
| 2 | 442_push.pct | 4-4-2 Attacking | 0xA8B748 |
| 3 | 442_diamond_default.pct | 4-4-2 Diamond | 0xA8B720 |
| 4 | 343_default.pct | 3-4-3 | 0xA1594C |
| 5 | 352_default.pct | 3-5-2 | 0xA78298 |
| 6 | 352_defensive_default.pct | 3-5-2 Defensive | 0xA8B6E4 |
| 7 | 352_attacking_default.pct | 3-5-2 Attacking | 0xA1593C |
| 8 | 41212_default.pct | 4-1-2-1-2 | 0xA7828C |
| 9 | 424_default.pct | 4-2-4 | 0xA782A0 |
| 10 | 433_default.pct | 4-3-3 | 0xA782A8 |
| 11 | 451_default.pct | 4-5-1 | 0xA78284 |
| 12 | 532_default.pct | 5-3-2 | 0xA782B8 |
| 13 | 532_defensive_default.pct | 5-3-2 Defensive | 0xA8B658 |
| 14 | 532_attacking_default.pct | 5-3-2 Attacking | 0xA8B62C |
| 15 | sweeper_default.pct | Sweeper | 0x9B75DC |
| 16 | 4132.pct | 4-1-3-2 | 0xA8B624 |

The order is fixed in code. It is not the directory order and it is not sorted. The game reads no directory listing for this list.

Edge case seen in code: a file that fails to load does not advance the write index, and the code sets `[0xB67A34] = 0` (the error flag). The count at +0x5FD stays 0x11. The port can ignore this. All 17 files exist in `Data`.

### The second set in the same store (not listed)

The same store holds 28 blocks (`+0x5F5 = 0x1C`, array at `+0x5F1`) loaded from the "_vN" and other files (352_v1, 442_v1, 532_v1 … 4132, 343_default, 451_default, 442_wide, …). Their name at +0 is the file base name (see 2.3). `FUN_00896570(i, 1)` excludes them. `FUN_00896570(i, 0)` reaches them at index ≥ 17 (0x896570). Inference: AI teams use them (caller `FUN_006e0230`). The Best XI list must leave them out to match the game.

`FUN_00896570(short i, int presetOnly)` (thiscall, ECX = `[0xDE1D08]`) is a pure lookup. For `0 ≤ i < [+0x5FD]` it returns `[+0x5F9] + i*0x18E3`. Out of range it shows the assert box (tactics.cpp line 0x265) and returns 0.

### Human manager tactics (`[0xB63C98]`)

`[0xB63C98]` is a 0x10-byte object made by `FUN_005fb430` (source path 0x9CF11C, human manager code). Layout:

- +0: pointer to 16 records of 0xA6EF bytes. +4: count (16).
- Record index = `staff_id - [0xAE2364] + 0x10`. The 16 records belong to the last 16 staff ids (`FUN_005fb430` loop: `[0xAE23BC] + ([0xAE2364] - i - 1) * 0x6E`).
- Each record holds 5 slots of 0x1ECD bytes at +0. A slot is a 0x18E3 tactic block at +0, then 0x18F1: the tactics folder path (from `FUN_0090bea0("tactics")`), 0x19F5: 4 recent file paths of 0x104 bytes (0x5FB810).
- Record +0x9A69: byte, the active slot index. `FUN_005fb810` sets it to 0 and fills all 5 slots with a copy of preset 0 ("4-4-2").

What the 5 slots stand for is unknown. Inference: one per team the manager picks for (first team, reserves, and so on), from the strings "No tactics are available for the last reserve match" and "'B' international".

`FUN_005fbd00(manager)` (0x5FBD00, ret 4) returns `rec + rec[0x9A69] * 0x1ECD`. For an id outside the 16 human records it shows the assert box (line 0x139), sets `[0xB67A34] = 0`, and returns 0. An AI club's manager therefore gives 0 and a visible error. Do not call it for AI managers. With a null argument it uses the current network player `[0xB74C26 + [0xB77C7A]*0x300]`.

The list is the same for every human manager. Only the recent-files part (4 paths) is per manager slot.

### Recent files (part of the same menu)

`FUN_005fc760(i, buf, 0x104, 1, 0, 1)` copies path i from the active slot (+0x19F5 + i*0x104). With flag 1 it removes the folder when the folder is the slot's tactics folder. The menu shows "1. name" and so on. These are user `.pct` files. A Best XI list that wants to match the menu fully would need these, through a file parse. Recommendation: leave them out of the first build. The 17 presets are the formations.

## 2. The .pct format

### Loader

`FUN_005b5d10(const char *path, int silent, int obfuscated)`, thiscall, ECX = destination block, ret 0xC (0x5B5D10). The file opens with mode "rb" (0x98F928). `fread` is `FUN_00945bfe`.

Byte order: `FUN_0090c140` reads the last 4 bytes of the file and restores the file position. If they equal 0xF1FFFFF1 the loader byte-swaps. Any other value means little-endian (0x5B5E2B). All 40 shipped files are little-endian.

### Layout (file offsets for version ≥ 0x98EC5C)

| File offset | Size | Block offset | Content | Evidence |
|---|---|---|---|---|
| 0x000 | 4 | (local) | version + 0x075BCD15 when `obfuscated`. Valid range 0x98EC54..0x98EC5E. Shipped: 0x98EC5C or 0x98EC5E | 0x5B5EE7 |
| 0x004 | 0x32 | +0x33 | internal name, each byte NOT-ed (0xFF stays 0xFF), up to the first 0 | 0x5B5F65 to 0x5B6017 |
| 0x036 | 0xC8 | +0x65 | unknown (author string "Marc Vaughan" sits here) | 0x5B6020 |
| 0x0FE | 4 | +0x187B | formation code (only if version ≥ 0x98EC5A, else 0) | 0x5B60A6 |
| 0x102 | 0x16 | +0x12D | 11 slot masks, u16 | 0x5B6161 |
| 0x118 | 0x0B | +0x143 | 11 bytes (only if version ≥ 0x98EC5C, else zero) | 0x5B61FA |
| 0x123 | 0x16 | +0x14E | 11 second masks, u16 | 0x5B6282 |
| 0x139 | 0x420 | +0x164 | position grid per slot (11 × 2 × 3 × 4 × 4 bytes) | 0x5B6308 |
| then | … | +0x584, +0x188B, +0x5B4 | flags and instructions; size depends on version | 0x5B6472 to 0x5B6721 |

The brief's offset 0x103 for the first masks is off by one. The u16 at 0x102 is 0x0001 (GK), as the loader reads it.

### Name of a loaded block

At the end the loader calls `FUN_00944cff(path, 0, 0, block, 0)` (0x5B680D). This is the split-path routine, so block +0 becomes the file base name without extension (for example "442_default"). The internal name at +0x33 is empty in all 40 shipped files (parse below). For the presets, `FUN_00893dc0` then writes the display name over +0.

### Post-processing that can change the masks

After the read, for each slot 0..10 the loader calls `FUN_005b3c70` (get position) and `FUN_005b3560` (set position) (0x5B67DF to 0x5B6803). `FUN_005b3560` picks the best mask from the table at 0x9CC368 for the slot's grid position. If it differs from the stored one, it writes a new value to +0x12D (and +0x14E, +0x143) and calls `FUN_005b59a0`, which recomputes +0x187B. So the in-memory masks can differ from the file masks. Whether this happens for any shipped file is unknown (the table walk was not simulated).

## 3. Safe read path for the runtime

Use the blocks in memory. Evidence that this changes nothing:

- `[0xDE1D08]` is set once at game start (`FUN_0082a9c0`) and the presets stay for the session. The "Set To Default" handler `FUN_008a9950` only reads the preset (`FUN_00896570(i, 1)`) and copies it into the screen's working copy at screen +0xE44.
- `FUN_00896570` is a bounds check and pointer arithmetic. No allocation, no random number, no write except the error flag on a bad index.

Read in C (offsets from the code above, structs packed to 1):

```c
uint8_t *store = *(uint8_t **)0xDE1D08;
int n = *(int *)(store + 0x5FD);              /* 17 */
uint8_t *base = *(uint8_t **)(store + 0x5F9);
for (int i = 0; i < n; i++) {
    uint8_t *blk = base + i * 0x18E3;
    const char *name = (const char *)blk;    /* "4-4-2" etc. */
    const uint16_t *mask = (const uint16_t *)(blk + 0x12D); /* 11 */
}
```

Bypass `FUN_00896570` only if the port has its own bounds check. A file parse is a fallback only. It would miss the mask fix of `FUN_005b3560`, and it needs the code's name table, because the files hold no display names.

## 4. The manager's current tactic

`FUN_005fbd00(human manager)` returns the active slot's block. The game menu does not mark the current preset (every item uses the same flags). To mark one in Best XI, compare `_strcmpi(active + 0, preset + 0)`. This works after "Set To Default", because `FUN_008a9950` copies the whole preset block, name included. After "Load" the name is the file base name, and after an edit the name stays. Inference: a fallback match on the 11 masks at +0x12D is more reliable than the name. When nothing matches, select no item or default to index 0.

Unknown: when the screen's working copy (screen +0xE44) is written back to the slot. Best XI reads the slot, so it shows the last committed tactic.

## Verification of the parse

Script: scratch `pct.py` (not in the repo). It reads the format of section 2 and prints the masks for all 40 files. Results for three presets:

| File | Masks at 0x102 |
|---|---|
| 442_default.pct | 0001 0804 0084 0104 0404 0110 0810 0410 0140 0440 0090 (same as the brief) |
| 532_default.pct | 0001 0808 0088 0104 0404 0204 0410 0210 0140 0440 0110 |
| sweeper_default.pct | 0001 0808 0088 0104 0404 0202 0410 0210 0140 0440 0110 |
| 4132.pct | 0001 0804 0084 0104 0404 0110 0410 0208 0440 0140 0210 |

Every file gives 11 masks with one GK (0x0001) first and valid side bits. Every file has an empty internal name. Versions: 0x98EC5C (sizes 1428 and 1432) and 0x98EC5E (size 1476, with the 0xF2FFFFF2 end marker). In 11 files the second mask set equals the first. In the others it holds a nearby alternative position.

Not verified: the in-memory values after `FUN_005b3560`. A runtime log of the 17 blocks would settle it.

## Leave out of the port

- The 28 AI blocks at `+0x5F1` for the Best XI list.
- "Load Preset" (shown only when `[0xB7434A]` is set, purpose unknown) and the network paths in `FUN_005fbd00` and `FUN_005fb810` (null manager, `[0xB74C26]`).
