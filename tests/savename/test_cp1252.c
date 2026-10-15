/* Unit check of the club name of the save name suggestion (src/recomp_rt/win/cp1252.h): a name in code
 * page 1252 becomes UTF-8, so that the host can create the file. */
#include "cp1252.h"

#include <stdio.h>

static int failed;

static void check(const char *in, int max, size_t size, const char *want)
{
    char out[64];
    memset(out, 'x', sizeof out);
    cp1252_file_name(out, size < sizeof out ? size : sizeof out, in, max);
    if (strcmp(out, want)) {
        printf("FAIL \"%s\": got \"%s\", want \"%s\"\n", in, out, want);
        failed = 1;
    }
}

int main(void)
{
    check("Arsenal", 24, 49, "Arsenal");
    check("K\xf6ln", 24, 49, "K\xc3\xb6ln");                      /* o with diaeresis */
    check("M\xe1laga", 24, 49, "M\xc3\xa1laga");                  /* a with acute */
    check("N\xfc" "rnberg", 24, 49, "N\xc3\xbc" "rnberg");
    check("Fenerbah\xe7" "e", 24, 49, "Fenerbah\xc3\xa7" "e");
    check("Be\x9a" "ikta\x9a", 24, 49, "Be\xc5\xa1" "ikta\xc5\xa1");   /* 0x9a is s with caron in code page 1252 */
    check("A\x80" "B", 24, 49, "A\xe2\x82\xac" "B");              /* 0x80 is the euro sign, three bytes */
    check("A\x81" "B", 24, 49, "AB");                              /* 0x81 has no character */
    check("A/B:C*D", 24, 49, "ABCD");                              /* characters a file name cannot hold */
    check("Club  ", 24, 49, "Club");                               /* trailing spaces */
    check("K\xf6ln K\xf6ln", 6, 49, "K\xc3\xb6ln K");              /* the limit counts characters */
    check("\xf6\xf6\xf6\xf6", 24, 6, "\xc3\xb6\xc3\xb6");          /* the buffer limit counts bytes and keeps whole characters */
    if (!failed) printf("ok   cp1252 file names\n");
    return failed;
}
