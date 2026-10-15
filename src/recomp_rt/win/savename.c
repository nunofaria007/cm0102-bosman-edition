/* Save names (#34): Save Game suggests "Club YYYY-MM-DD" from the player's club and the game date.
 * The player can edit the name in the text entry as before.
 *
 * Save Game (side bar handler, menu item 0x3fe at 0x763eda) copies the last save name, game options
 * + 0x113, into the name buffer 0xdd6ed4 (0x763f2d..0x763f5f). It then calls the file dialog
 * FUN_0058f500 (cdecl, 14 arguments) at 0x763fb3 with the buffer as argument 5 and the save routine
 * 0x838e10 as argument 6. The wrap writes the suggested name into the buffer when the dialog is
 * opened with that buffer and that routine. Other dialogs (printouts, scout searches) do not change.
 *
 * The club: the manager of the player slot that has the turn (0xb74c26 + slot * 0x300, slot at
 * 0xb77c7a), club pointer at manager + 0x39, short name at club + 0x38, long name at club + 4. The side
 * bar reads the same fields at 0x762ff0..0x763017. A manager with no club keeps the original name. */
#include "win.h"

#include <stdio.h>
#include <string.h>

#define GAME_DATE 0xae2c90u        /* s16 day of the year from 0, s16 year */
#define PLAYER_MANAGER 0xb74c26u   /* per player slot, 0x300 bytes apart: the manager */
#define PLAYER_SLOT 0xb77c7au      /* s16: the player slot that has the turn */
#define NAME_BUFFER 0xdd6ed4u      /* the text of the name entry of the file dialog */
#define SAVE_ROUTINE 0x00838e10u   /* the save callback of Save Game */
#define CLUB_MAX 24                /* characters of the club name in the suggestion */

static int leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

/* Copies a club name without the characters that a file name cannot hold on Windows. */
static void copy_name(char *out, const char *in, int max)
{
    int n = 0;
    for (; *in && n < max; in++)
        if (!strchr("\\/:*?\"<>|", *in) && (unsigned char)*in >= 0x20) out[n++] = *in;
    while (n > 0 && out[n - 1] == ' ') n--;
    out[n] = 0;
}

/* The suggested name in out, or 0 when the player has no club. */
static int suggest(char *out, size_t size)
{
    uint32_t slot = (uint32_t)(int16_t)rd16(g_mem, PLAYER_SLOT);
    uint32_t manager = rd32(g_mem, PLAYER_MANAGER + slot * 0x300u);
    uint32_t club = manager ? rd32(g_mem, manager + 0x39) : 0;
    if (!club) return 0;
    char name[CLUB_MAX + 1];
    copy_name(name, (const char *)(g_mem + club + 0x38), CLUB_MAX);
    if (!name[0]) copy_name(name, (const char *)(g_mem + club + 4), CLUB_MAX);
    if (!name[0]) return 0;

    int day = (int16_t)rd16(g_mem, GAME_DATE), year = (int16_t)rd16(g_mem, GAME_DATE + 2);
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int month = 0;
    while (month < 11 && day >= days[month] + (month == 1 && leap(year))) day -= days[month] + (month == 1 && leap(year)), month++;
    snprintf(out, size, "%s %04d-%02d-%02d", name, year, month + 1, day + 1);
    return 1;
}

void o_0058f500(void);

/* The file dialog, cdecl (14 arguments). */
RT_WRAP(0058f500, file_dialog)
{
    char name[64];
    if (rt_arg(5) == NAME_BUFFER && rt_arg(6) == SAVE_ROUTINE && suggest(name, sizeof name))
        memcpy(g_mem + NAME_BUFFER, name, strlen(name) + 1);
    o_0058f500();
}
