/* Test of the score of a player in a slot (src/recomp_rt/win/pick_score.h): a hidden attribute does not
 * count, a player whose key attributes are all hidden has no score, and he ranks after every player with a
 * score. Usage: pickscore_test */
#include "pick_score.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

int main(void)
{
    static const uint8_t keys[] = {3, 5, 8, 0};         /* three key attributes (numbers 3, 5 and 8) */
    uint8_t attr[32];

    /* All visible: the sum of the keys times fit times 60 over their count. */
    memset(attr, 0, sizeof attr);
    attr[3] = 12, attr[5] = 16, attr[8] = 14;
    CHECK(pick_value(attr, keys, 20) == (12 + 16 + 14) * 20 * 60 / 3);          /* 16800: shown 14 */
    CHECK(pick_shown(pick_value(attr, keys, 20)) == 14);

    /* One key hidden: the others count, the hidden one does not (it is not a zero). */
    attr[5] = 0;
    CHECK(pick_value(attr, keys, 20) == (12 + 14) * 20 * 60 / 2);               /* 15600: shown 13 */
    CHECK(pick_shown(pick_value(attr, keys, 20)) == 13);

    /* Every key hidden: no score, shown as "-" (0). Other attributes do not matter. */
    attr[3] = 0, attr[8] = 0;
    attr[1] = 20, attr[2] = 20;
    CHECK(pick_value(attr, keys, 20) == PICK_UNSCORED);
    CHECK(pick_value(attr, keys, 1) == PICK_UNSCORED);
    CHECK(pick_shown(PICK_UNSCORED) == 0);

    /* The order: cannot take the slot, then no score, then every score (a score of 0 included). */
    CHECK(pick_rank(-1) == -1);
    CHECK(pick_rank(PICK_UNSCORED) == 0);
    CHECK(pick_rank(0) == 1);                           /* a fit of 0 gives a score of 0: still a score */
    CHECK(pick_rank(60) > pick_rank(0));
    CHECK(pick_rank(PICK_UNSCORED) < pick_rank(0));
    CHECK(pick_rank(-1) < pick_rank(PICK_UNSCORED));

    /* The number: rounded, 1 to 20; a low value still shows 1, never "-". */
    CHECK(pick_shown(0) == 1);
    CHECK(pick_shown(60) == 1);
    CHECK(pick_shown(599) == 1 && pick_shown(1800) == 2);
    CHECK(pick_shown(24000) == 20 && pick_shown(30000) == 20);

    printf(failures ? "FAILED\n" : "pickscore: ok\n");
    return failures != 0;
}
