/* A small PDF writer for printouts (#33). See pdf.h. */
#include "pdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHEET_W 595.0          /* A4 in points */
#define SHEET_H 842.0
#define MARGIN 36.0
#define FONT_MAX 10.0
#define FONT_MIN 6.0
#define LEADING 1.2            /* line height, as a multiple of the font size */
#define COURIER_ADVANCE 0.6    /* width of a Courier character, as a multiple of the font size */

typedef struct { char *p; size_t n, cap; } Buf;

static int put(Buf *b, const void *s, size_t n)
{
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (b->n + n + 1 > cap) cap *= 2;
        char *q = realloc(b->p, cap);
        if (!q) return 0;
        b->p = q, b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
    return 1;
}

static int putf(Buf *b, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static int putf(Buf *b, const char *fmt, ...)
{
    char tmp[256];
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    __builtin_va_end(ap);
    return n > 0 && put(b, tmp, (size_t)n);
}

typedef struct { size_t start, len; } Line;   /* a line of the text */
typedef struct { size_t first, count; } Page; /* lines of one page */

int pdf_file_name(char *out, size_t room, const char *kind, const char *club, int year, int month, int day)
{
    char c[64];
    size_t n = 0;
    for (const char *s = club; *s && n < 40; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch < 0x20 || strchr("/\\:*?\"<>|", ch)) ch = ' ';
        /* Do not cut inside a UTF-8 character. */
        if (ch >= 0xc0) {
            size_t k = ch >= 0xf0 ? 4 : ch >= 0xe0 ? 3 : 2;
            if (n + k > 40) break;
            memcpy(c + n, s, k);
            n += k, s += k - 1;
            continue;
        }
        c[n++] = (char)ch;
    }
    while (n && c[n - 1] == ' ') n--;
    c[n] = 0;
    int w = snprintf(out, room, "%s - %s %04d-%02d-%02d.pdf", kind, c, year, month, day);
    return w > 0 && (size_t)w < room;
}

char *pdf_build(const char *text, size_t len, int lines_per_page, size_t *out_len)
{
    /* Split into lines and pages. */
    size_t nlines = 0, cap = 256, npages = 0, pcap = 16;
    Line *lines = malloc(cap * sizeof *lines);
    Page *pages = malloc(pcap * sizeof *pages);
    Buf b = {0};
    if (!lines || !pages) goto fail;
    size_t longest = 0;
    #define NEW_PAGE() do { \
        if (npages == pcap) { pcap *= 2; Page *q = realloc(pages, pcap * sizeof *pages); if (!q) goto fail; pages = q; } \
        pages[npages].first = nlines; pages[npages].count = 0; npages++; } while (0)
    NEW_PAGE();
    int in_page_has_text = 0;
    double font = FONT_MAX;
    for (size_t i = 0; i < len;) {
        size_t s = i;
        while (i < len && text[i] != '\n') i++;
        size_t e = i;
        if (i < len) i++;
        if (e > s && text[s] == '\f') {
            s++;
            if (in_page_has_text || pages[npages - 1].count) NEW_PAGE();
            in_page_has_text = 0;
            if (s == e) continue;       /* the form feed stood alone on its line */
        }
        if (e > s && text[e - 1] == '\r') e--;
        if (nlines == cap) { cap *= 2; Line *q = realloc(lines, cap * sizeof *lines); if (!q) goto fail; lines = q; }
        lines[nlines].start = s, lines[nlines].len = e - s;
        if (e - s > longest) longest = e - s;
        nlines++;
        pages[npages - 1].count++;
        in_page_has_text = 1;
    }
    /* Font: fit the longest line, within the limits. */
    if (longest) {
        font = (SHEET_W - 2 * MARGIN) / (longest * COURIER_ADVANCE);
        if (font > FONT_MAX) font = FONT_MAX;
        if (font < FONT_MIN) font = FONT_MIN;
    }
    int fit = (int)((SHEET_H - 2 * MARGIN) / (font * LEADING));
    int per = lines_per_page > 0 && lines_per_page < fit ? lines_per_page : fit;

    /* Break each page into sheets of per lines. */
    typedef struct { size_t first, count; } Sheet;
    size_t nsheets = 0, scap = 16;
    Sheet *sheets = malloc(scap * sizeof *sheets);
    if (!sheets) goto fail;
    for (size_t p = 0; p < npages; p++) {
        size_t first = pages[p].first, left = pages[p].count;
        do {
            size_t c = left < (size_t)per ? left : (size_t)per;
            if (nsheets == scap) { scap *= 2; Sheet *q = realloc(sheets, scap * sizeof *sheets); if (!q) { free(sheets); goto fail; } sheets = q; }
            sheets[nsheets].first = first, sheets[nsheets].count = c;
            nsheets++;
            first += c, left -= c;
        } while (left);
    }

    /* Objects: 1 catalog, 2 pages, 3 font, then a page and a stream for each sheet. */
    size_t nobj = 3 + 2 * nsheets;
    size_t *off = calloc(nobj + 1, sizeof *off);
    if (!off) { free(sheets); goto fail; }
    if (!put(&b, "%PDF-1.4\n", 9)) goto fail2;
    #define BEGIN(id) do { off[id] = b.n; if (!putf(&b, "%zu 0 obj\n", (size_t)(id))) goto fail2; } while (0)
    BEGIN(1);
    if (!put(&b, "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n", 41)) goto fail2;
    BEGIN(2);
    if (!put(&b, "<< /Type /Pages /Kids [", 23)) goto fail2;
    for (size_t s = 0; s < nsheets; s++) if (!putf(&b, "%zu 0 R ", 4 + 2 * s)) goto fail2;
    if (!putf(&b, "] /Count %zu >>\nendobj\n", nsheets)) goto fail2;
    BEGIN(3);
    if (!put(&b, "<< /Type /Font /Subtype /Type1 /BaseFont /Courier /Encoding /WinAnsiEncoding >>\nendobj\n", 80)) goto fail2;
    for (size_t s = 0; s < nsheets; s++) {
        size_t pobj = 4 + 2 * s, sobj = pobj + 1;
        BEGIN(pobj);
        if (!putf(&b, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 3 0 R >> >> /Contents %zu 0 R >>\nendobj\n", sobj)) goto fail2;
        Buf c = {0};
        if (!putf(&c, "BT\n/F1 %.2f Tf\n%.2f TL\n%.2f %.2f Td\n", font, font * LEADING, MARGIN, SHEET_H - MARGIN - font)) { free(c.p); goto fail2; }
        for (size_t k = 0; k < sheets[s].count; k++) {
            Line l = lines[sheets[s].first + k];
            if (!put(&c, "(", 1)) { free(c.p); goto fail2; }
            for (size_t j = 0; j < l.len; j++) {
                unsigned char ch = (unsigned char)text[l.start + j];
                if (ch < 0x20 || ch == 0x7f) ch = ' ';
                if (ch == '(' || ch == ')' || ch == '\\') if (!put(&c, "\\", 1)) { free(c.p); goto fail2; }
                if (!put(&c, &ch, 1)) { free(c.p); goto fail2; }
            }
            if (!put(&c, ") Tj T*\n", 8)) { free(c.p); goto fail2; }
        }
        if (!put(&c, "ET\n", 3)) { free(c.p); goto fail2; }
        BEGIN(sobj);
        if (!putf(&b, "<< /Length %zu >>\nstream\n", c.n) || !put(&b, c.p, c.n) || !put(&b, "endstream\nendobj\n", 17)) { free(c.p); goto fail2; }
        free(c.p);
    }
    size_t xref = b.n;
    if (!putf(&b, "xref\n0 %zu\n0000000000 65535 f \n", nobj + 1)) goto fail2;
    for (size_t i = 1; i <= nobj; i++) if (!putf(&b, "%010zu 00000 n \n", off[i])) goto fail2;
    if (!putf(&b, "trailer\n<< /Size %zu /Root 1 0 R >>\nstartxref\n%zu\n%%%%EOF\n", nobj + 1, xref)) goto fail2;
    free(off); free(sheets); free(lines); free(pages);
    *out_len = b.n;
    return b.p;
fail2:
    free(off); free(sheets);
fail:
    free(lines); free(pages); free(b.p);
    return NULL;
}
