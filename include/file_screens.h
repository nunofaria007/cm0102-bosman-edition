#ifndef FILE_SCREENS_H
#define FILE_SCREENS_H

#include "cm_types.h"

/*
 * The "Please Confirm" screen of file_screens.cpp: a question with "No" and
 * "Yes", or "Ok" alone. Its answer waits in two globals for the screen that
 * opened it, which reads them after the main loop closes the dialog.
 */

extern s32 g_dialog_code;   /* DAT_00dd6be0, says which question it was */
extern s32 g_dialog_button; /* DAT_00dd6be4, 1 = No, 2 = Yes, 3 = Ok */

/*
 * FUN_00591840. Pushes the screen with the flag set. Slots 0 to 2 keep their
 * own copies of the texts, slot 3 holds `buttons` (0 = "Ok" alone, 1 = "No"
 * and "Yes"), slots 4 and 5 the colours, slot 7 the code. Nothing happens
 * when a text is NULL (the original shows an error box) or the push fails.
 */
void confirm_push(const char *title, const char *heading, const char *text,
                  s32 buttons, u16 text_colour, u16 colour, s32 code);

/* FUN_00591b70. */
void confirm_build(void);

/*
 * FUN_00591dc0. A button with an id of 1 to 5 stores the code and the id
 * and returns SCREEN_POP. Other clicks go to the side bar. The keys of the
 * original (Return, Escape, "y", "n" and others) are not ported.
 */
s32 confirm_input(s32 clicked);

#endif
