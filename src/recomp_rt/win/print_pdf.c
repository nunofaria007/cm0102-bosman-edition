/* Print to PDF (#33): the Printer choice of Print To becomes PDF.
 *
 * The original keeps Print To as three exclusive bits of the options flags (Options + 4):
 * Printer 0x1000000, File 0x2000000, Both 0x4000000 (setter 0x5d7c60, options screen draw 0x823950,
 * label "Printer" at 0xa80a88). We keep the bits, so game.cfg and saved games do not change. The label
 * says "PDF". The original's default is Printer when a printer exists (0x5d7bc0 asks 0x84c7f0, which
 * calls EnumPrintersA): net.c answers that one printer exists, so PDF is the default.
 *
 * Both printout writers write the text to a file first. With the print flag they read it back and send
 * the lines to the print job: 0x8033e0 begins it, 0x803710 adds a line, 0x803930 adds a page break (a
 * NULL entry), 0x803a90 prints with 0x84c830 (lines, count, columns), thiscall, ret 0xc. We replace
 * 0x84c830 and write the same lines as a PDF. Printer and Both print, File does not, so File keeps the
 * text file and its dialog, and Both writes the text file and the PDF.
 *
 * The file name comes from the writer that called the print: 0x7d7c80 (competition: 4 arguments, cdecl;
 * name at competition + 4, flags 1 table, 2 goals, 4 assists, 8 average rating, 0x10 man of the match) and
 * 0x7d4460 (club: 5 arguments, cdecl; club + 4 long name, + 0x38 short name; flags 1 squad, 2 stats, 4
 * transfers, 8 fixtures). Both wraps only note the kind and the name, then call the original.
 *
 * The PDF goes to Prints in the save folder. The game shows its message box with the path. */
#include "win.h"
#include "cp1252.h"
#include "pdf.h"
#include "../host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LABEL_PRINTER 0xa80a88u    /* "Printer", 8 bytes with the end */
#define MESSAGE_BOX 0x5e8290u      /* cdecl (title, text, 0) */
#define NAME_CHARS 30
#define CLUB_UTF8 128

#define PLAYER_MANAGER 0xb74c26u   /* per player slot, 0x300 bytes apart: the manager */
#define PLAYER_SLOT 0xb77c7au      /* s16: the player slot that has the turn */
#define STAFF_CLUB 0x39u           /* staff: the club */

static char kind[32];              /* of the printout writer that started this print, else empty */
static char name[CLUB_UTF8];       /* its club or competition, else empty */

void o_00823950(void);
void o_007d7c80(void);
void o_007d4460(void);

/* "PDF" in place of "Printer", at each draw of the options screen. */
RT_WRAP(00823950, options_draw)
{
    char *label = (char *)g_mem + LABEL_PRINTER;
    if (!strcmp(label, "Printer")) memcpy(label, "PDF", 4);
    o_00823950();
}

static const char *kind_of(unsigned flags, unsigned mask, const char *const *names)
{
    const char *one = NULL;
    int n = 0;
    for (int i = 0; names[i]; i++)
        if (flags & (mask << i)) one = names[i], n++;
    return n == 1 ? one : "Printout";
}

static void note(uint32_t record, uint32_t name_at, const char *k)
{
    snprintf(kind, sizeof kind, "%s", k);
    name[0] = 0;
    cp1252_file_name(name, sizeof name, (const char *)(g_mem + record + name_at), NAME_CHARS);
}

/* Competition printout (league table and player tables). */
RT_WRAP(007d7c80, competition_printout)
{
    static const char *const kinds[] = {"Table", "Goals", "Assists", "Average rating", "Man of the match", NULL};
    note(rt_arg(0), 4, kind_of(rt_arg(1) & 0xff, 1, kinds));
    o_007d7c80();
    kind[0] = name[0] = 0;          /* the note belongs to this writer's print only */
}

/* Club printout (squad, stats, transfers, fixtures). */
RT_WRAP(007d4460, club_printout)
{
    static const char *const kinds[] = {"Squad", "Stats", "Transfers", "Fixtures", NULL};
    uint32_t club = rt_arg(0);
    note(club, 0x38, kind_of(rt_arg(2) & 0xff, 1, kinds));
    if (!name[0]) cp1252_file_name(name, sizeof name, (const char *)(g_mem + club + 4), NAME_CHARS);
    o_007d4460();
    kind[0] = name[0] = 0;
}

/* The texts of the message box are in runtime memory, as in compare_screen.c, not on the game's heap:
 * a block of the heap that the game does not know changes the addresses of every later allocation,
 * and so the later days of the game (Q152). */
#define BOX_TITLE_SIZE 16u
#define BOX_TEXT_SIZE 600u

static void box(const char *title, const char *text)
{
    /* Test runs only (tests/smoke/same_save_views.py): no box, to see what the box does to the game. */
    if (getenv("RECOMP_TEST_NO_PDF_BOX")) return;
    static uint32_t title_at, text_at;
    if (!title_at) {
        title_at = win_static_alloc(BOX_TITLE_SIZE);
        text_at = win_static_alloc(BOX_TEXT_SIZE);
    }
    char t[BOX_TEXT_SIZE];
    cp1252_from_utf8(t, sizeof t, text);
    snprintf((char *)g_mem + title_at, BOX_TITLE_SIZE, "%s", title);
    snprintf((char *)g_mem + text_at, BOX_TEXT_SIZE, "%s", t);
    uint32_t args[3] = {title_at, text_at, 0};
    rt_callback(MESSAGE_BOX, 3, args);
}

/* The club of the human manager, for a print that no printout writer named: its short name, else its
 * long name, in the file name form. */
static void club_of_manager(char *out, size_t size)
{
    out[0] = 0;
    uint32_t manager = rd32(g_mem, PLAYER_MANAGER + (uint32_t)(int32_t)(int16_t)rd16(g_mem, PLAYER_SLOT) * 0x300u);
    uint32_t club = manager ? rd32(g_mem, manager + STAFF_CLUB) : 0;
    if (!club) return;
    cp1252_file_name(out, size, (const char *)(g_mem + club + 0x38), NAME_CHARS);
    if (!out[0]) cp1252_file_name(out, size, (const char *)(g_mem + club + 4), NAME_CHARS);
}

/* Makes the folder and its parents. */
static void make_dirs(const char *path)
{
    char tmp[1200];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *c = tmp + 1; *c; c++)
        if (*c == '/') { *c = 0; host_mkdir(tmp); *c = '/'; }
    host_mkdir(tmp);
}

/* The text of a print job: each line with '\n', a NULL entry as a form feed line. A page break at the
 * end prints nothing, as in 0x84c830. */
static char *job_text(uint32_t lines, uint32_t count, size_t *len)
{
    size_t cap = 1024, n = 0;
    char *t = malloc(cap);
    if (!t) return NULL;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t line = rd32(g_mem, lines + 4 * i);
        const char *s = line ? (const char *)(g_mem + line) : "\f";
        size_t l = strlen(s);
        if (!line && i + 1 == count) break;
        if (n + l + 2 > cap) {
            while (n + l + 2 > cap) cap *= 2;
            char *q = realloc(t, cap);
            if (!q) { free(t); return NULL; }
            t = q;
        }
        memcpy(t + n, s, l);
        t[n + l] = '\n';
        n += l + 1;
    }
    *len = n;
    return t;
}

/* The print job 0x84c830 (lines, count, columns), thiscall, ret 0xc: writes a PDF. */
RT_REPLACE(0084c830, print_job)
{
    uint32_t lines = rt_arg(0), count = rt_arg(1);
    size_t len = 0, size = 0;
    char *text = job_text(lines, count, &len);
    char *pdf = text ? pdf_build(text, len, 0, &size) : NULL;
    free(text);
    if (!pdf) {
        kind[0] = name[0] = 0;
        box("Print", "The PDF could not be made.");
        rt_ret_stdcall(0, 3);
        return;
    }

    char dir[1100], base[200], path[1400];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);
    snprintf(dir, sizeof dir, "%s/Prints", win_save_folder());
    make_dirs(dir);
    /* A print that no writer named (another path of the game) gets "Print - <club> <date>.pdf". The
     * kind and the name belong to this print only: the next print starts clean. */
    char club[CLUB_UTF8];
    if (!name[0]) club_of_manager(club, sizeof club);
    else snprintf(club, sizeof club, "%s", name);
    char what[sizeof kind];
    snprintf(what, sizeof what, "%s", kind[0] ? kind : "Print");
    kind[0] = 0;
    name[0] = 0;
    if (!pdf_file_name(base, sizeof base, what, club[0] ? club : "Game", year, month, day))
        snprintf(base, sizeof base, "Printout.pdf");
    snprintf(path, sizeof path, "%s/%s", dir, base);
    for (int k = 2; k < 100; k++) {                /* a second print on the same day keeps the first */
        HostStat st;
        if (host_stat(path, &st) != 0) break;
        snprintf(path, sizeof path, "%s/%.*s (%d).pdf", dir, (int)(strlen(base) - 4), base, k);
    }
    FILE *f = host_fopen(path, "wb");
    int ok = f && fwrite(pdf, 1, size, f) == size;
    if (f && fclose(f) != 0) ok = 0;
    free(pdf);

    char msg[1600];
    if (ok) {
        fprintf(stderr, "recomp: printout PDF: %s\n", path);
        snprintf(msg, sizeof msg, "The PDF is saved in %s", path);
    } else {
        fprintf(stderr, "recomp: printout PDF failed: %s\n", path);
        snprintf(msg, sizeof msg, "The PDF could not be saved in %s", path);
    }
    box("Print", msg);
    rt_ret_stdcall(0, 3);
}
