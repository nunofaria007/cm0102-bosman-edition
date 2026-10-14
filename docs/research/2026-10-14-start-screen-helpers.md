# Start screen helpers of the original

Serves: `docs/specs/2026-10-14-start-screen-spec.md`, slices 2, 3 and 4.

Method: objdump disassembly of the original (Intel syntax). Every fact names the address that shows it. "Inference" marks a conclusion that the code does not show directly. All numbers are hex unless the text says otherwise. `S` is the screen stack object at 0xb74450. `ch` is the current channel (s16 at 0xb77c7a, `S+0x382a`). `D` is the screen manager of the current record, read as `[S + ch*0x300 + 0x7fc]` (helpers in scrman, 0x803f6d) or `[0xb74c4c + ch*0x300]` (other files, 0x5ed51e). Both give record +0x28.

Two calls appear everywhere below. Their argument order comes from `2026-10-13-display-and-scrman-inventory.md`:

- `area_create` = `FUN_0055fba0`, thiscall on D, 11 stack arguments, `ret 0x2c`: (x1, y1, x2, y2, columns u8, column weights ptr, rows u8, row weights ptr, flags, colour u16, owner s16). Returns the s16 area index in AX.
- `widget_create` = `FUN_0055f990`, thiscall on D, 18 stack arguments, `ret 0x48`: (flags, x1, y1, x2, y2, column, row, style, colour, highlight, text_flags, font, text_colour, text, max_len (+0x40), id (+0x08), data (+0x48), area). Returns the s16 widget index in AX.

Colour globals (`DAT_00aebdec` yellow, `DAT_00ae3184` light grey, `DAT_00aebe04` navy, `DAT_00ae317e` grey, `DAT_00ae31a8` red, `DAT_00aebde8`) are 0 in the file and are set at run time. The port already has their values in `include/gui_utils.h`.

## 1. Text function `FUN_0066f4e0`

### Signature

`char *FUN_0066f4e0(char *dst, const char *format, ...)`, cdecl, variadic. The caller pops (example 0x81bb0e, `add esp, 0x8`). Evidence: `[esp+0x58c]` is argument 1 and `[esp+0x590]` is argument 2 after a frame of 0x588 bytes (0x66f519, 0x66f520). The variadic list starts at `[esp+0x594]` (0x66f7e3).

Return value: `dst + n`, where n is the number of leading spaces of `format`. The epilogue loads `[esp+0x8]` after four pops, which is the local at `[esp+0x18]` (0x66fb9d). That local is set to `dst + n` at 0x66f54e. Some callers use the return value as the string (0x81c9eb, 0x81c9fe).

### Globals it reads

| Global | Value on a plain start | Meaning |
|---|---|---|
| `DAT_009eb2dc` | -1 in the file. Set at start-up, see below. | Language index. -1 and 0 select the plain path (0x66f557, 0x66f562). |
| `DAT_00b679f4` | 0. Nothing in the code writes it (only reads at 0x66f58d, 0x66f7d9). | When non-zero the function builds the stripped format and returns without formatting (0x66f8ec). Dead for the port. |
| 0xb6619c | Scratch buffer, 0x828 bytes up to 0xb669c4 (inference from the next global). | Holds the stripped format. No bounds check. |

What 0x9eb2dc selects: when it is neither -1 nor 0, the function calls `FUN_0066fbb0(&local, format+n, 0)` (0x66f570). That function looks the format up in the loaded language table (not read in detail). If it returns non-zero and the found text is not empty, the function takes a translation path (0x66f58d..0x66f7d4) that substitutes the arguments into the found text. Otherwise it takes the plain path. The value is set at start-up by `FUN_0066e790` (called at 0x5d7304 with the path `CM3_DATA\language.ldb`). It reads the language of `GetKeyboardLayout` (import at 0x9671d8, call 0x66e7c0) through a jump table (0x66e7e9). English gives 1 for sub-language US (0x0409) and 0 for others such as UK (0x66e7f0..0x66e804). Then `FUN_0066eb40` loads the file and writes the global again (0x66ec5d, 0x66ef7b, 0x66ef92, 0x66f0be). The final value after that load is unknown. For a differential test, set 0x9eb2dc to -1 in the original before each call.

### Plain path (0x9eb2dc is -1 or 0)

```c
char *text_format(char *dst, const char *fmt, ...) {
    int n = 0;
    while (fmt[n] == ' ') { dst[n] = ' '; n++; }          /* 0x66f533..0x66f547 */
    const char *s = fmt + n;
    char *out = dst + n;
    va_list va = &first_vararg;                            /* 0x66f7e3 */
    int k = 0, j = 0, in_note = 0;                         /* eax, edi, ebp */
    if (s[0] != 0) for (;;) {                              /* 0x66f7f6 */
        char c = s[k];
        if (c == '{' || c == '[') {                        /* 0x66f801 */
            va += 4;                                       /* skip one argument, 0x66f80f */
            if (s[k+1] == '}' || s[k+1] == ']') goto next; /* drop c */
        }
        if (c == '}' || c == ']') {                        /* 0x66f82a */
            if (k == 0 || s[k-1] == '{' || s[k-1] == '[') goto next;  /* drop c */
        }
        if (c == '<') {                                    /* 0x66f852 */
            while (c != 0) { c = s[++k]; if (c == '%') break; }
            in_note = 1;
        }
        c = s[k];                                          /* 0x66f86a */
        if (c == ' ' && in_note) {
            while (c != 0) { c = s[++k]; if (c == '>') break; }
            c = s[++k]; in_note = 0;                       /* char after '>' */
            /* repeat the '{' '[' '}' ']' tests of above on this c, 0x66f88b..0x66f8c6 */
        }
        buf[j++] = c;                                      /* 0x66f8c8, buf = 0xb6619c */
        if (s[k] == 0) break;
    next:
        if (s[++k] == 0) break;                            /* 0x66f8d6 */
    }
    buf[j] = 0;                                            /* 0x66f8e5 */
    vsprintf(out, buf, va);                                /* FUN_009470ce, 0x66f901 */
    if (strstr(out, "{") || strstr(out, "["))              /* 0x66f90c, 0x66f91e */
        if (strlen(out) < 1000) tag_pass(out);             /* 0x66f93e..0x66fb50 */
    return out;
}
```

`FUN_009470ce` is the MSVC `vsprintf`: it builds a string FILE with flag 0x42 and count 0x7fffffff and calls `_output` at 0x947c67 (0x9470e7..0x9470f6). So `%s`, `%d`, `%c`, `%x`, `%u` and widths behave as MSVC printf. A port can use the host `vsnprintf` for these. Floating point and `%p` can differ between MSVC and the host (inference).

`strstr` is `FUN_00945a60`. The search strings are "{" (0x9ed914) and "[" (0x9ed910).

### When a text needs more than plain printf

The port can use plain `vsprintf` with the leading spaces copied when both of these hold:

1. The format contains none of `{`, `[`, `}`, `]`, `<`.
2. The formatted result contains neither `{` nor `[` (a `%s` argument can bring one in, and then the tag pass at 0x66f93e runs).

Exact rules for these characters in the format:

- `{` and `[` start a tag. Each one, anywhere in the format, moves the argument pointer on by 4 before `vsprintf` runs. An empty tag `{}`, `[]`, `{]` or `[}` is removed. A tag with content stays in the stripped format and is replaced later by the tag pass, which calls `FUN_006701f0` (0x66fa57, 0x66fb27). The tag pass is out of scope and was not read in detail.
- `}` or `]` at index 0 of the format after the leading spaces is removed.
- `<` starts a translator note. The text from `<` up to the next `%` is removed. After that, the first space ends the kept part, and the text from that space up to and including the next `>` is removed. If no `%` follows, the rest of the format is lost. If no space comes after the `%`, a later `>` is kept.

Example from the side bar: "Version\n<%s - version>" (0xa5a7b4) becomes the format "Version\n%s". With the argument "3.9.68" the result is "Version\n3.9.68". The start screen reaches only this `<` rule and plain `%s` and `%d`. The port needs the `<` rule for the side bar.

### Calls

`FUN_0066fbb0` (translation lookup, only when 0x9eb2dc is not -1 or 0), `FUN_009470ce` (vsprintf), `FUN_00945a60` (strstr), `FUN_006701f0` (tag lookup, tag pass only), `FUN_00944c94` (free of three locals at 0x66fb6c..0x66fb8e, all null on the plain path).

## 2. Heading `FUN_00803f30` and sub-heading `FUN_00803f80`

Both are thiscall on S (ECX = 0xb74450 at every caller, example 0x81bb2b). They read D from `[S + ch*0x300 + 0x7fc]`. They return the widget index from `widget_create` in AX.

```c
/* FUN_00803f30, ret 0xc */
s16 heading(S, u16 colour, u16 text_colour, char *text) {
    return widget_create(D, 1, 100, 10, 790, 70, 0, 0, 0x30,
                         colour, 0, 0xc, 7, text_colour, text, 0, 0, 0, -1);
}   /* 0x803f30..0x803f79 */

/* FUN_00803f80, ret 8. The second argument is not read. */
s16 sub_heading(S, char *text, int unused) {
    return widget_create(D, 1, 100, 80, 790, 125, 0, 0, 2,
                         0, 0, 0xc, 6, DAT_00aebdec, text, 0, 0, 0, -1);
}   /* 0x803f80..0x803fc9 */
```

Each colour argument is pushed as a 32-bit slot. The function loads only the low 16 bits for `DAT_00aebdec` (0x803f84). The upper bits of that slot are whatever EDX held (inference: `widget_create` keeps only 16 bits).

The start screen calls `heading(DAT_00ae31a8, DAT_00ae3184, 0xdd70a8)` (0x81bb24..0x81bb30) after it formats "Championship Manager 2001/02" (0xa80288) into 0xdd70a8. It calls `sub_heading(0xde1f64, 0)` (0x81bb53..0x81bb5f) after it formats "Setup Game" (0xa8027c) into 0xde1f64.

## 3. Button helpers

All are thiscall on S, callee pop. None calls `FUN_00804030`.

```c
/* FUN_008040a0, ret 0x2c: button in an area cell */
s16 cell_button(S, s16 area, int column, int row, int style, u16 colour,
                int text_flags, int font, u16 text_colour, char *text, int id, int data) {
    return widget_create(D, 2, 0, 0, 0, 0, column, row, style, colour, colour,
                         text_flags, font, text_colour, text, 0, id, data, area);
}   /* 0x8040a0..0x8040fd */

/* FUN_00804100, ret 0x24: label in an area cell */
s16 cell_label(S, s16 area, int column, int row, int style, u16 colour,
               int text_flags, int font, u16 text_colour, char *text) {
    return widget_create(D, 1, 0, 0, 0, 0, column, row, style, colour, 0,
                         text_flags, font, text_colour, text, 0, 0, 0, area);
}   /* 0x804100..0x804158 */

/* FUN_00804030, ret 0x30: free button, no area. Confirmed from 0x804030..0x804090. */
s16 free_button(S, x1, y1, x2, y2, style, u16 colour, text_flags, font,
                u16 text_colour, char *text, id, data) {
    return widget_create(D, 2, x1, y1, x2, y2, 0, 0, style, colour, colour,
                         text_flags, font, text_colour, text, 0, id, data, -1);
}
```

The highlight of `cell_button` is the colour (0x8040cd, 0x8040ce push the same EAX). The highlight of `cell_label` is 0 (0x80413c).

Another helper that later screens use: `FUN_00468b40(clicked)`, thiscall on S, `ret 4`. It returns the s16 id (widget +0x08) of the clicked widget, sign-extended. When clicked is -1 or the id is 0, it returns the injected id `S+0x26278e` (0x468b40..0x468b88).

## 4. Menu builder `FUN_005ed300`, item constructor and destructor

### Item (0xd4 bytes, packed)

| Offset | Size | Meaning | Evidence |
|---|---|---|---|
| +0x00 | s16 | Font. -1 = use the default font argument. | 0x4568f2, 0x5ed60b |
| +0x02 | u16 | Text colour. 0 = use the default colour. | 0x4568f7, 0x5ed5cc |
| +0x04 | 200 | Text | 0x5ed62f |
| +0xcc | s32 | Id (widget +0x08) | 0x5ed628 |
| +0xd0 | s32 | Data (widget +0x48) | 0x5ed621 |

`FUN_004568f0`, thiscall on the item, no stack arguments, returns the item in EAX: `item->font = -1; item->colour = 0;` (0x4568f0..0x4568fd). It does not clear the text, id or data.

`FUN_005a53c0` is a bare `ret` (0x5a53c0). The destructor does nothing.

Arrays use the MSVC vector helpers `FUN_009463af(array, size, count, ctor, dtor)` (constructor loop) and `FUN_009462bb(array, size, count, dtor)` (destructor loop).

The start screen sets font, colour, text and id of items 0..8 only. It never writes +0xd0 (0x81bb64..0x81bcba). The data of every menu button is therefore uninitialised stack memory of the build function. A differential test must ignore widget +0x48 of the 9 menu buttons, or fill both sides with the same value.

### Signature

`int FUN_005ed300(int x1, int y1, int x2, int y2, int columns, int rows, int default_font, int count, item *items, u32 bold_mask, int sort)`, cdecl, 11 stack arguments, plain `ret` (0x5ed773). Returns 1 on success, 0 on error. Offsets: arguments 1..11 are `[esp+0x240]..[esp+0x268]` after the frame of 0x23c bytes (0x5ed4da..0x5ed507, 0x5ed3c0).

### Pseudocode

```c
int menu_build(x1, y1, x2, y2, cols, rows, def_font, count, items, mask, sort) {
    if (count <= 0 || items == 0) {                         /* 0x5ed329, 0x5ed338 */
        error_box("Error", "v3.9.68 gui_utils.cpp 193");   /* 0x5ed6fe..0x5ed74c, line 0xc1 */
        DAT_00b67a34 = 0; return 0;
    }
    item *a = new item[count];          /* operator new 0x944cf1, ctor loop, 0x5ed351..0x5ed37e */
    for (i = 0; i < count; i++) memcpy(&a[i], &items[i], 0xd4);   /* 0x5ed3a6..0x5ed3b8 */

    if (sort) {                                             /* 0x5ed3c0 */
        for (i = 0; i < count - 1; i++)
            for (j = i + 1; j < count; j++)
                if (strcmp(a[i].text, a[j].text) > 0) {     /* inline, unsigned bytes, 0x5ed40c..0x5ed437 */
                    swap(&a[i], &a[j], 0xd4);               /* FUN_0090ba90, 0x5ed449 */
                    bi = mask & (1u << (i & 31));            /* 0x5ed461 */
                    bj = mask & (1u << (j & 31));
                    if (bi && !bj) mask += (1u << j) - (1u << i);   /* 0x5ed476 */
                    else if (!bi && bj) mask += (1u << i) - (1u << j); /* 0x5ed48d */
                }
    }

    s16 area = area_create(D, x1, y1, x2, y2, cols, 0, rows, 0, 1, 0, -1);   /* 0x5ed524 */

    int col = 0, row = 0, wx1 = 0, wx2 = 0, w = <garbage>;  /* w is never read before set */
    int left = count;
    int ncol = cols & 0xff;                                  /* 0x5ed54b */
    for (k = 0; k < count; k++, left--) {
        if (k % ncol == 0 && left < ncol) {                  /* centred last row, 0x5ed56d */
            w   = (x2 - 2*ncol - x1 + 3) / ncol;             /* C division, truncates */
            wx1 = (x2 - (w + 2)*left - x1 + 3) / 2 + x1;     /* /2 truncates toward 0 */
            wx2 = w + wx1 - 1;
        }
        if (mask & (1u << (k & 31))) { style = 0x822; tc = a[k].colour ? a[k].colour : DAT_00aebdec; }
        else                         { style = 0x22;  tc = a[k].colour ? a[k].colour : DAT_00ae3184; }
        font = (a[k].font == -1) ? def_font : a[k].font;    /* 0x5ed60b..0x5ed615 */
        widget_create(D, 2, wx1, 0, wx2, 0, col, row, style,
                      DAT_00aebe04, DAT_00aebe04, 0xc, font, tc,
                      a[k].text, 0, a[k].id, a[k].data, area);  /* 0x5ed61c..0x5ed66e */
        if (wx1 != 0 && wx2 != 0) { wx1 += w + 2; wx2 += w + 2; }   /* 0x5ed673..0x5ed67f */
        if (++col == ncol) { col = 0; row++; }               /* 0x5ed683..0x5ed6a1 */
    }
    delete[] a;                         /* dtor loop 0x9462bb, operator delete 0x944c94 */
    return 1;
}
```

The fill is row by row: column first, then the next row. Only the last row can be centred, because only there is `left < ncol` at a row start. The column number of a centred item is still its column. Its x1 and x2 are set explicitly, and y1 and y2 stay 0. The area must have room for the rows. The builder does not check `rows`.

On the start screen (0x81bc98..0x81bcc5): `menu_build(110, 145, 780, 535, 2, 6, 3, 9, items, 0, 0)`. No sort, no bold. Items 0..7 go to rows 0..3, columns 0 and 1. Item 8 ("Web Sites") is centred: w = 669/2 = 334, x1 = 337/2 + 110 = 278, x2 = 611, column 0, row 4.

The sibling `FUN_005ed780` builds a different menu with headers (it reads `[esp+0x43c]` as a selected id and makes buttons with flags 0x4002). The start screen does not use it.

## 5. Back and Next bar `FUN_005edcc0`

`int FUN_005edcc0(int back, int next)`, cdecl, plain `ret`, returns 1 (0x5eddd8, 0x5ede0e). The arguments are at `[esp+0x7dc]` and `[esp+0x7e0]` after the frame (0x5edd26, 0x5edd9d).

```c
int back_next_bar(int back, int next) {
    u8 weights[2] = {3, 1};                                   /* 0x5edcfc..0x5edd0b */
    s16 area = area_create(D, 100, 555, 790, 590, 2, weights, 1, 0, 1, 0, -1);  /* 0x5edd10 */
    char buf[0x7d0];
    text_format(buf, "Back");                                 /* 0x9880a8, 0x5edd21 */
    if (back) cell_button(S, area, 0, 0, 0x30, DAT_00ae317e, 0xc, 3, DAT_00ae3184, buf, -2, 0);
    else      cell_label (S, area, 0, 0, 0x30, DAT_00ae317e, 0x2c, 3, DAT_00ae3184, buf);
    text_format(buf, "Next");                                 /* 0x9880a0, 0x5edd98 */
    if (next) cell_button(S, area, 1, 0, 0x30, DAT_00ae317e, 0xc, 3, DAT_00ae3184, buf, -3, 0);
    else      cell_label (S, area, 1, 0, 0x30, DAT_00ae317e, 0x2c, 3, DAT_00ae3184, buf);
    return 1;
}
```

The weights pointer is `esp+6` of the frame, which holds the bytes {3, 1} (0x5edccf with 4 pushes made, written at 0x5edd06 and 0x5edd0b). The bytes at `esp+4` and `esp+5` ({1, 1}) are not part of the array.

The start screen calls `back_next_bar(FUN_007fec60(0) == 0, 0)` (0x81bccd..0x81bce1). The start screen is the only node, so `FUN_007fec60(0)` returns 1. Both cells are labels with text flags 0x2c on the start screen.

## 6. Background picker `FUN_0041bfb0` and the random function

### Picture table

`FUN_0041b3d0` (background.cpp, path string 0x989934) fills the table. The start-up code calls it once at 0x5d7a98. It opens `FUN_0090bde0("CM3_PICTURES", "pics.cfg")` with mode "r" (0x41b3da..0x41b3f9). It is a text file. There is no directory listing, so directory order does not matter. Only the order of names in `pics.cfg` matters.

The table at 0x989860 has 11 sections of 10 bytes: {char *name, s16 count, sub *subs}. Names (0x98992c..0x9898d0): 0 nations, 1 clubs, 2 stadiums, 3 comps, 4 awards, 5 staff, 6 training, 7 boardroom, 8 trophy, 9 transfer, 10 si. A sub is 16 bytes: {char *key, s16 count, s16 count2, char **names, char **names2} (0x41b5a3..0x41b5c4).

Parse rules, from the code:

- `FUN_0090cba0(file, buf)` reads one line (0x90cba0..0x90cc21). It skips tab, CR and space at the line start, and keeps them after that. It skips empty lines. `#` drops the rest of the line, and the line goes on with the next line (0x90cbd4..0x90cbe9). So a CR at the end of a line stays in the buffer.
- A line that starts with `[` is a section. The name runs to `]`. It is matched to the 11 names with `FUN_00951c50` (inference: `_stricmp`). The match sets the current section (0x41b438..0x41b4a8).
- Any other line is "key: name, name, ...". The key runs to `:` and is copied into a new sub (0x41b538..0x41b5c4). Each later character goes through a jump table (0x41b60e, table 0x41b838). `,`, `}` and the end of the line end a name. A space is dropped, even inside a name. `{` ends a name and marks it as a name of the second list. Every other character, CR too, goes into the name.
- `FUN_0041b8c0(sub, name, second)` keeps a name only if a file opens. It tries `PICTURES\name`, then `stadium\name`, then `old\name` (`_makepath` 0x945b13 with "stadium" 0x989a78 and "old" 0x989a74). If all fail and the name does not start with `_`, it tries the same three with `_name` (0x41b98d..0x41b9c5). Else it drops the name. A kept name is `_strdup` of the form that opened (0x41ba15, 0x41baa8), appended to `names` or `names2`.
- A sub with no names is freed and the section count goes down again. The section pointer stays non-null (0x41b667..0x41b688).

`pics.cfg` in the user's Pictures folder has CRLF line ends. Only `[NATIONS]` has a line for the start screen: "England: boro3.rgn, boro5.rgn, ... holywell03.RGN" with 35 names. The CR stays on the last name "holywell03.RGN\r", so its open fails and the name is dropped (inference: no file has that name on Windows or macOS). Result: section 0 has one sub with 34 names, in the file order from "boro3.rgn" to "holywell02.RGN". Sections 1..5 have no lines, so their pointer is null.

Names such as "boro3.rgn" differ in case from the files ("Boro3.RGN"). On a case-sensitive file system the open fails and the name is dropped. On the default macOS file system it opens.

### `DAT_00989858`

It is 1 in the file (data section). No instruction writes it, and no pointer to it exists outside the 15 reads (search for the bytes 58 98 98 00). It is a constant 1.

### `FUN_0041bfb0`

`void FUN_0041bfb0(void)`, no arguments, plain `ret` (0x41c087). It saves ECX at entry only to make a local (0x41bfb0).

```c
void pick_background(void) {
    const char *t = get_title(S);                         /* FUN_007fee80, 0x41bfc2 */
    if (t[0] != '\\' && t[0] != 0) return;                /* keep the picture on a rebuild */
    if (DAT_00989858 == 0) { set_title(S, ":"); return; } /* 0x989adc */
    s16 tries = 0;
    s16 sec = rnd(6);                                     /* 0x41bfee */
    do {
        if (table[sec].subs != 0 && sec != 2) {           /* 2 = stadiums, 0x41c00a */
            s16 si = rnd(table[sec].count);               /* 0x41c018 */
            sub *b = &table[sec].subs[si];
            if (b->names != 0) {
                s16 ni = rnd(b->count);                   /* 0x41c03e */
                if (b->names[ni] != 0) {
                    set_title(S, path("CM3_PICTURES", b->names[ni]));   /* 0x41c0ab, 0x41c0b9 */
                    return;
                }
            }
        }
        tries++; if (++sec >= 6) sec = 0;                 /* 0x41c059..0x41c069 */
    } while (tries < 6);
    set_title(S, ":");                                    /* 0x41c073 */
}
```

`set_title` is `FUN_007feeb0` and `path` is `FUN_0090bde0`. With the shipped `pics.cfg` the loop always ends in section 0: every start value from 0 to 5 reaches 0 within 6 tries. One pick makes three random calls: `rnd(6)`, `rnd(1)` (always 0) and `rnd(34)`. Only the last one chooses the picture.

### Random `FUN_0090cfc0`

`int FUN_0090cfc0(int n)`, cdecl, plain `ret`. It saves ECX and ESI (0x90cfc0).

```c
int rnd(int n) {
    if (n == 0) return 0;                                 /* no state change, 0x90cfc6 */
    u32 *p = DAT_00de1f60 + 1;                            /* 0x90cfda */
    DAT_00de1f60 = p;
    if (p > DAT_00de2798) {                               /* end = 0xad4130, unsigned compare */
        DAT_00de1f60 = p = (u32 *)0xaa2454;
        DAT_00de1f5c = msvc_rand() & 0xffff;              /* 0x90cff1 */
    }
    if (n >= -0xffff && n <= 0xffff) {
        int r = (DAT_00de1f5c + *p) % n;                  /* signed idiv, 0x90d020 */
        return n > 0 ? r : -r;
    }
    /* |n| > 0xffff: a floating point path, 0x90d02d..0x90d09c. Not needed here. */
}
```

The pool at 0xaa2454 is 51,000 initialised dwords in the data section of the original (0xc738 entries, 0x90d0d6). Example values: 0x6230, 0x47d8 at the start.

Seed `FUN_0090d0a0(seed)`, cdecl:

```c
void rnd_seed(int seed) {
    if (seed == 0) seed = time(NULL);                     /* FUN_00946ebb, 0x90d0aa */
    msvc_srand(seed);                                     /* FUN_009469fa */
    sprintf(0xde1f64, "Random Number seed: %d", seed);    /* 0xad437c */
    DAT_00de1f60 = 0xaa2454 + 4 * (msvc_rand() % 51000);  /* 0x90d0d0..0x90d0e4 */
    DAT_00de2798 = 0xad4130;
    DAT_00de1f5c = msvc_rand() % 0xffff;                  /* 0x90d0ea..0x90d10c */
    if (DAT_00de1f60 outside [0xaa2454, 0xad4130]) DAT_00de1f60 = 0xaa2454;
}
```

`msvc_rand` is `FUN_00946a04`: `seed = seed * 0x343fd + 0x269ec3; return (seed >> 16) & 0x7fff;`, with the seed at 0xad6c30 (1 in the file).

The seed is set twice before the start screen: `rnd_seed(0)` in `FUN_005cc6b0` (0x5cc6b2) and again in `FUN_0081b890` (0x81b9b2). The second uses the value after `-seed` on the command line when there is one (0x81b963..0x81b99e), else 0, which means the time. So a plain start seeds from the clock. Other code between the seed and the build can call `rnd`. That count is unknown.

For the differential test, write the same `DAT_00de1f60` and `DAT_00de1f5c` into both sides, or give both sides the same title before the build. Then a title that is not "\\" makes the picker return at once.

## 7. Side bar `FUN_0075db00`, start screen part

`void FUN_0075db00(u8 mode, int arg2)`, cdecl, plain `ret` (0x761256). `mode` is `[esp+0x370]` at entry of the body (0x75dba2), read as a byte. `arg2` is only read for a manager record (0x75e038). The start screen calls `FUN_0075db00(4, 0)` (0x81bb35..0x81bb39).

Globals it tests, values on a fresh start:

| Global | Value | Where set | Effect on the start screen |
|---|---|---|---|
| `DAT_00dd7118` | 0 | Set to 1 only after the start screen loop ends (0x81ba8d) | 0 forces mode 1 (0x75db59 to 0x75dbf1) |
| record key `[0xb74c26 + ch*0x300]` | 0 | Push of the start screen | No manager record, so no mode 2 to 4 and no "Chat" |
| `DAT_00dd711c` | 0 (BSS) | 0x829fe3, 0x82cd6a, 0x830509, in game setup (inference) | 0 makes "Add Manager" a label. The other three tests are then skipped. |
| `FUN_00838c60` | not called | | Needs the database (`DAT_00ae23bc`). It shows an error box when there is none (0x838c60..0x838c9e). |
| `FUN_00838de0` | not called | | Returns 1 only when the record key's manager has byte +0x5f == 5. Returns 0 for key 0 (0x838de0..0x838e05). |
| `DAT_00b63c98` | 0 (BSS) | 0x828a2e, 0x82b9b2, 0x8302f8 | Not read on the start screen |
| `DAT_00b77c90` (`S+0x3840`, network session) | 0 | Network code | 0 means no "Chat" |
| `FUN_00803d50()` | 0 | Sum of flagged-node counts. The start screen has flag 0. | 0 enables "Restart Game" and "Exit Game" |
| player id `[0xb74c24 + ch*0x300]` | 0 | | 0 enables "Restart Game" |
| `DAT_009cc2c4` (font setting) | -2 in the file. Written only by 0x5b1faf. | | Selects the arrow texts, see below |

Steps in order (all `D` calls are on the record's screen manager):

1. `time(0xb74310)` (0x75db2c). The global gets the current time. A differential test must ignore it or fix the clock.
2. `S+0x37ee = 0x823930`, `S+0x37f2 = 0x423`, `S+0x37f6 = 0x424` (0x75db3b..0x75db4f).
3. mode = 1.
4. a1 = `area_create(D, 0, 0, 89, 599, 1, 0, 1, 0, 1, 0, -1)` (0x75dc22).
5. `widget_create(D, 0x400, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0xc, 1, 0, path("CM3_DATA", "game.mbr"), 0, 0, 0, a1)` (0x75dc27..0x75dc67). This is the side bar picture.
6. a2 = `area_create(D, 5, 10, 85, 590, 1, 0, 13, 0, 1, 0, -1)` (0x75dc95).
7. Row 0: `text_format(0xde1f64, "Version\n<%s - version>", FUN_0090d130())` (0x75dcb3..0x75dcc3). `FUN_0090d130` takes no arguments and returns the string "3.9.68" at 0xad4394 (0x90d130). The text is "Version\n3.9.68". Then `cell_label(S, a2, 0, 0, 0x1061, 0, 0xc, 1, DAT_00aebdec, 0xde1f64)` (0x75dcee).
8. Row 1: a3 = `area_create(D, 5, 10, 85, 590, 2, 0, 13, 0, 1, 0, -1)` (0x75dd25). Then the two arrow buttons in a3:
   - When `DAT_009cc2c4` != -1 (0x75dd31): text = `sprintf("%c", 0x28)`, then `cell_button(S, a3, 0, 1, 0x1021, 0, 0xc, 0, DAT_00aebdec, 0xde1f64, -2, 0)`. Then the same with 0x29, column 1, id -3 (0x75ddaa..0x75de05). Font 0 and yellow text.
   - When `DAT_009cc2c4` == -1: `cell_button(S, a3, 0, 1, 0x1021, 0, 0xc, 1, DAT_00ae3184, "<<<", -2, 0)` and `cell_button(S, a3, 1, 1, 0x1021, 0, 0xc, 1, DAT_00ae3184, ">>>", -3, 0)`. Font 1 and light grey text. "<<<" is 0xa16444 and ">>>" is 0xa16440.
   - The port behaves as -1 (text and fonts spec). A differential test must set 0x9cc2c4 to -1 in the original.
9. Row 2: `text_format(0xde1f64, "Add\nManager")` (0xa5a7a8). On the start screen: `cell_label(S, a2, 0, 2, 0x1021, 0, 0x2c, 1, DAT_00ae3184, 0xde1f64)` (0x75de9c). The button form (id 0x3fb, text flags 0xc, same other values) needs `DAT_00dd711c` set.
10. "Chat" (id 0x421, text colour `DAT_00aebde8`) is skipped. It needs `DAT_00b77c90` and a record key (0x75dea1..0x75df13). When it is made, it takes the next row and moves the later rows down by one.
11. Row 3: `text_format(0xde1f64, "Restart\nGame")` (0xa5a790). Then `cell_button(S, a2, 0, 3, 0x1021, 0, 0xc, 1, DAT_00aebdec, 0xde1f64, 0x42f, 0)` (0x75df80). The label form uses text flags 0x2c.
12. Row 4: `text_format(0xde1f64, "Exit\nGame")` (0xa5a784). Then `cell_button(S, a2, 0, 4, 0x1021, 0, 0xc, 1, DAT_00aebdec, 0xde1f64, 0x402, 0)` (0x75e002). The label form uses text flags 0x2c.
13. No manager record, so it jumps to 0x7611ff. Key 0 and no manager skip the two tail blocks (0x76120e, 0x761238). Return.

The rows use the 13-row grid of a2 and a3. Both areas cover the same rectangle. The "<<<" and ">>>" buttons are in row 1 of a3. All other cells are in a2.

## 8. Start screen input

### `FUN_0081bd70(int clicked)`

cdecl, plain `ret`. `clicked` is `[esp+0xf1c]` after a frame of 0xf18 bytes, read as s16 (0x81c8d9). For a plain start (no command line, no files, no pending dialog result):

1. If slot 0 != 0: slot 0 = 0, `FUN_00823890(0)`, return -4 (0x81bd81..0x81bdb0).
2. `strstr(cmdline 0xae2c98, "-load")` is null (0x81bdb1). When it is found, the code writes `+` over the `-` (0x81bdcb), so this runs once.
3. `_access("quick_start.cfg", 0)` fails (`FUN_0090c120`, 0x81c076). `_access("auto_setup.cfg", 0)` fails (0x81c469). These two checks run on every call. They use the current directory (inference from the bare names).
4. `S+0x3834` (0xb77c84) is 0, so the pending result part (0x81c4cc..0x81c8d5) is skipped. The main loop sets `S+0x3834` to 1 only for the call with -1 after a pop.
5. id = clicked != -1 ? (s16) widget[clicked]+0x08 : 0. If id == 0: id = `S+0x26278e` (0x81c8d9..0x81c920).
6. 1..9: the menu actions of the 2026-10-13 inventory, item 6c. Each returns -4. 
7. -3: return 0 (0x81cdc6). This catches ">>>" of the side bar before the side bar code sees it.
8. Else: r = `FUN_007615d0(clicked)`. If r == 0: r = `FUN_007643d0(clicked)`. Return r (0x81cdd9..0x81cdf9).

### `FUN_007615d0(int clicked)`, side bar input

cdecl, plain `ret`. On the start screen (key 0, `DAT_00dd7118` 0, `S+0x3834` 0):

- id 0x424 returns -11 (0x76161d..0x76162b). The start screen has no widget with this id.
- With no click it walks 0x762070 → 0x7621ef → 0x762296 → 0x762389. `FUN_007ffe40(0)` returns the done flag of the record with key 0, which is 0.
- `FUN_00725ca0(clicked)` handles only id 0x3e8 and returns 0 for all start screen ids (0x725e5a, 0x725f58).
- Then the id chain from 0x762c58:
  - -2 ("<<<"): returns -2 only when `FUN_007fec60(0)` == 0 (0x762c97..0x762cb0). On the start screen it returns 1, so the chain goes on and nothing else matches -2.
  - -3: returns -3 when `FUN_007febb0()` == 0 (0x762cfc..0x762d13). Not reached from the start screen (step 7 above).
  - 0x3fb ("Add Manager"): with key 0 it calls `FUN_00820d40()` and returns -4 (0x76400d..0x764032). On the start screen the cell is a label with id 0, so it cannot reach this.
  - 0x42f ("Restart Game"): it pushes the confirm screen with "Restart the game ?" (0xa5a838) and code 0x3d, and returns -4 (0x7642ad..0x7643b1).
  - 0x402 ("Exit Game"): see below. Returns -4.
  - Else it returns 0 (0x7643be).

### `FUN_007643d0(int clicked)`

cdecl, plain `ret`. It handles ids 0x426, 0x7d0..0x7e7 and 0x421 (0x76441d..0x764f37), then asks `FUN_00468b40` for ids 0x7d2 and 0x7e4. None is on the start screen. It returns 0 for every start screen widget and for -1 (0x764ffd).

### Results on the start screen

| Click | Id | Result of `FUN_0081bd70` | Main loop effect |
|---|---|---|---|
| Menu item | 1..9 | -4 after its action | Rebuild |
| "<<<" | -2 | 0 | Redraw (`FUN_00802d00(0,0)`) |
| ">>>" | -3 | 0 | Redraw |
| "Add Manager" (label) | 0 | 0 | Redraw. Inference: the hit test does not return labels, so this is not reached. |
| "Restart Game" | 0x42f | -4 | Rebuild, confirm screen on top |
| "Exit Game" | 0x402 | -4 | Rebuild, confirm screen on top |
| Back, Next (labels) | 0 | 0 | As "Add Manager" |
| No click | -1 | 0 | Nothing |

### "Exit Game" (id 0x402) in detail

The original does not quit at once.

1. If `FUN_00803d50()` != 0, nothing happens and the function returns 0 (0x76433e).
2. Text = "Exit the game ?" (0xa5a810) for player 0, else "Exit the network game ?" (0xa5a820).
3. `FUN_00591840(title, heading, text, 1, DAT_00ae3184, 1, DAT_00ae31a8, 0xb)` (0x764392..0x7643a6). Title is "Championship Manager 2001/02" from `FUN_00839570`. Heading is "Please Confirm" (0x9909b8). `FUN_00839590(x)` returns `DAT_00ae3184` for x != 0, else `DAT_00ae31a8`.
4. `FUN_00591840` pushes a screen with build 0x591b70, input 0x591dc0, flag 1 (0x59188b). It stores its arguments in slots 0..7. Slot 7 is the code 0xb. That screen has the buttons "No" (id 1, column 0) and "Yes" (id 2, column 1) (0x591d37..0x591db2).
5. Return -4.
6. On "Yes" or "No", the confirm input sets `DAT_00dd6be0` = slot 7 (0xb) and `DAT_00dd6be4` = the button id, and returns -9 (0x591e7d..0x591e94).
7. The loop pops the confirm screen and calls `FUN_0081bd70(-1)` with `S+0x3834` = 1. That reaches `FUN_007615d0(-1)`, where 0x762abb handles code 0xb. It clears both globals. For "Yes" (2), player 0, `FUN_00803d50()` == 0 and no manager record, it calls `FUN_005cc7d0()`, `FUN_005a8f60()` and `exit(0)` (`FUN_00945934`) (0x762c42..0x762c4d). The program ends inside the input function. The main loop never sees -7.
8. For "No" (1), it continues with id 0, returns 0, and the loop rebuilds the start screen (r == 0 after a pop sets rebuild).

The spec decision "Exit Game quits" leaves out the confirm screen. That is a difference from the original. `FUN_005cc7d0` and `FUN_005a8f60` are the shut-down calls (inference from their use before `exit` at 0x81b927 and 0x41b71a).

## 9. Default screen after the start screen loop

After the start screen loop ends, `FUN_0081b890` stores the default screen build 0x81ff10, input 0x820950, flag 0, no leave or exit, and 5 slot values from 0xdd7148 (0x81ba74..0x81ba97). The values at 0xdd7148..0xdd7158 are 0 in the file. The build makes the "Manager Status" screen (0xa5a47c). It lists the human managers with a state each ("Playing", "Waiting", "On Holiday", "On Holiday (Watching)", "Not Playing", "Not Playing (locked)"), shows "Server IP Address" in a network game, and has the buttons "Back" and "Finish" (0xa80558). With no manager it shows "There are currently no human managers. Click on 'Add Manager' ..." (0xa805e0, 0xa8063c). The push of `FUN_007fe600` uses this default only when the record key and first node are 0, so it is what the screen stack falls back to once a game exists. The start screen never shows it.

## 10. Open doubts

- The final value of `DAT_009eb2dc` after `FUN_0066eb40` loads `language.ldb` is unknown. On a US English keyboard layout it starts at 1, which selects the translation lookup. Force -1 in the differential test.
- `FUN_0066fbb0` (translation lookup) and `FUN_006701f0` (tag lookup) were not read.
- The tag pass (0x66f93e..0x66fb50) was read only far enough to know when it runs.
- The count of `rnd` calls between the seed at 0x81b9b2 and the start screen build is unknown. A port with its own random source cannot pick the same picture as the original for the same clock. The pool at 0xaa2454 is data of the original. The project rule forbids copying it into the repo, so a port that wants the same sequence must read it from the user's `cm0102.exe` at run time.
- The widget data (+0x48) of the 9 menu buttons is uninitialised stack memory in the original.
- Whether the hit test returns label widgets (flags 1) is not checked here. The input functions treat a label as id 0.
- `FUN_00820d40` ("Add Manager" with no record) and the "Restart Game" confirm path (code 0x3d at 0x762915) were not read. Neither is reachable from the start screen as built on a fresh start, except the restart confirm screen itself.
- `widget_create` keeps 16 bits of the colour and font slots. This is an inference from the field sizes. The upper bits that the helpers push are garbage in places (0x803f84, 0x5ed646).

Not needed by a single-player port: the translation path of the text function, `DAT_00b679f4`, the "Chat" button and `DAT_00b77c90`, "Exit the network game ?", side bar modes 2 to 4 (0x75e04b..0x7611ff), and the hook `S+0x37ee`/`S+0x37f2` (no widget with id 0x423 on the start screen).
