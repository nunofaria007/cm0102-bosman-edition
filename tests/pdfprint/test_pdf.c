/* Test of the PDF writer (src/recomp_rt/win/pdf.c): the text that a PDF holds equals the text that
 * went in. Usage: pdf_test */
#include "pdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

/* Reads the lines of the page streams back: one "(text) Tj" for each line, and "%%PAGE" before the
 * first line of each page. Out gets the same text form as the input: lines with '\n', pages with
 * '\f' alone on a line between them. */
static char *extract(const char *pdf, size_t n)
{
    char *out = calloc(n + 16, 1);
    size_t o = 0;
    int pages = 0;
    for (const char *p = pdf; p < pdf + n; p++) {
        if (!strncmp(p, "BT\n", 3)) {
            if (pages++) out[o++] = '\f', out[o++] = '\n';
        } else if (*p == '(' && p > pdf && p[-1] == '\n') {
            for (p++; *p != ')'; p++) {
                if (*p == '\\') p++;
                out[o++] = *p;
            }
            out[o++] = '\n';
        }
    }
    return out;
}

/* The structure of a PDF: each "N 0 obj" has its "endobj" before the next object, the objects count
 * 1 up, the xref offsets point at the objects, the trailer size is right, "/Type /Page" matches only the
 * page objects and each stream has the length that it gives. */
static void structure(const char *pdf, size_t sheets)
{
    size_t objs = 0, off[64] = {0};
    for (const char *p = pdf; (p = strstr(p, " 0 obj\n")); p += 7) {
        const char *s = p;
        while (s > pdf && s[-1] != '\n') s--;
        size_t id = (size_t)strtoul(s, NULL, 10);
        CHECK(id == objs + 1);
        if (id < 64) off[id] = (size_t)(s - pdf);
        const char *e = strstr(p, "endobj\n"), *next = strstr(p + 7, " 0 obj\n");
        CHECK(e != NULL && (!next || e < next));
        objs++;
    }
    CHECK(objs == 3 + 2 * sheets);
    const char *x = strstr(pdf, "\nxref\n");
    CHECK(x != NULL);
    if (x) {
        size_t count = 0;
        CHECK(sscanf(x + 6, "0 %zu\n", &count) == 1 && count == objs + 1);
        const char *line = strstr(x + 6, "\n") + 1;
        CHECK(!strncmp(line, "0000000000 65535 f \n", 20));
        line += 20;
        for (size_t i = 1; i <= objs && i < 64; i++, line += 20) {
            size_t o = (size_t)strtoul(line, NULL, 10);
            char want[32];
            snprintf(want, sizeof want, "%zu 0 obj\n", i);
            CHECK(o == off[i] && !strncmp(pdf + o, want, strlen(want)));
            CHECK(line[17] == 'n');
        }
        size_t start = 0;
        const char *sx = strstr(line, "startxref\n");
        CHECK(sx && sscanf(sx + 10, "%zu", &start) == 1 && start == (size_t)(x + 1 - pdf));
        char size[40];
        snprintf(size, sizeof size, "/Size %zu ", objs + 1);
        CHECK(strstr(line, size) != NULL);
    }
    size_t pages = 0, kinds = 0;
    for (const char *p = pdf; (p = strstr(p, "/Type /Page")); p += 11) {
        if (p[11] == 's') kinds++;
        else pages++;
    }
    CHECK(pages == sheets && kinds == 1);
    for (const char *p = pdf; (p = strstr(p, "<< /Length ")); p += 11) {
        size_t len = (size_t)strtoul(p + 11, NULL, 10);
        const char *s = strstr(p, "stream\n") + 7;
        CHECK(!strncmp(s + len, "endstream\nendobj\n", 17));
    }
}

static void roundtrip(const char *text, int per_page, const char *expect)
{
    size_t n;
    char *pdf = pdf_build(text, strlen(text), per_page, &n);
    CHECK(pdf != NULL);
    if (!pdf) return;
    CHECK(!strncmp(pdf, "%PDF-1.4", 8));
    CHECK(n > 20 && !strncmp(pdf + n - 6, "%%EOF\n", 6));
    size_t pages = 1;
    for (const char *p = expect; *p; p++) pages += *p == '\f';
    structure(pdf, pages);
    char *back = extract(pdf, n);
    if (strcmp(back, expect)) printf("got:\n%s\nexpected:\n%s\n", back, expect);
    CHECK(!strcmp(back, expect));
    free(back);
    free(pdf);
}

int main(void)
{
    /* One page, text as it is. */
    roundtrip("Squad - Arsenal\n  1  Seaman   GK  20\n", 0, "Squad - Arsenal\n  1  Seaman   GK  20\n");
    /* Brackets and backslashes are escaped and come back. */
    roundtrip("a (b) c\\d\n", 0, "a (b) c\\d\n");
    /* A form feed ends a page. */
    roundtrip("one\n\f\ntwo\n", 0, "one\n\f\ntwo\n");
    roundtrip("one\n\ftwo\n", 0, "one\n\f\ntwo\n");
    /* A page also ends after lines_per_page lines. */
    roundtrip("1\n2\n3\n4\n5\n", 2, "1\n2\n\f\n3\n4\n\f\n5\n");
    /* An empty line stays a line. */
    roundtrip("a\n\nb\n", 0, "a\n\nb\n");
    /* A last line without a newline stays. */
    roundtrip("a\nb", 0, "a\nb\n");
    /* A long page splits on its own at the sheet's height, with no line lost. */
    {
        char text[20000] = "";
        for (int i = 0; i < 200; i++) { char l[16]; snprintf(l, sizeof l, "line %d\n", i); strcat(text, l); }
        size_t n;
        char *pdf = pdf_build(text, strlen(text), 0, &n);
        char *back = extract(pdf, n);
        char *flat = calloc(strlen(back) + 1, 1);
        size_t f = 0;
        for (const char *p = back; *p; p++) { if (*p == '\f') p++; else flat[f++] = *p; }
        CHECK(!strcmp(flat, text));
        CHECK(strchr(back, '\f') != NULL);
        free(flat); free(back); free(pdf);
    }
    /* Empty text gives a PDF with one empty page. */
    {
        size_t n;
        char *pdf = pdf_build("", 0, 0, &n);
        CHECK(pdf && strstr(pdf, "/Type /Page") != NULL);
        free(pdf);
    }

    /* File names. */
    char name[128];
    CHECK(pdf_file_name(name, sizeof name, "Squad", "Arsenal", 2001, 7, 20) && !strcmp(name, "Squad - Arsenal 2001-07-20.pdf"));
    CHECK(pdf_file_name(name, sizeof name, "Table", "K\xc3\xb6ln", 2002, 12, 1) && !strcmp(name, "Table - K\xc3\xb6ln 2002-12-01.pdf"));
    CHECK(pdf_file_name(name, sizeof name, "Squad", "A/B:C*D", 2001, 7, 20) && !strcmp(name, "Squad - A B C D 2001-07-20.pdf"));
    CHECK(!pdf_file_name(name, 10, "Squad", "Arsenal", 2001, 7, 20));

    printf(failures ? "FAILED\n" : "ok\n");
    return failures != 0;
}
