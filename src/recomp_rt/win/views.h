/* Shared parts of the Team views screen (views.c), its Best XI tab (best_xi.c) and the tactic list
 * (tactics.c). */
#ifndef RECOMP_VIEWS_H
#define RECOMP_VIEWS_H

#include <stdint.h>

#define VIEWS_TEXT_SIZE 0x80u       /* the text buffer that views.c gives best_xi_build */
#define VIEWS_BOX_PX 62             /* a player's name box, screen pixels: the text does not scale */
#define VIEWS_ROW_PX 17             /* a row of the list beside the pitch, screen pixels */
#define VIEWS_FORMATION_X1 620      /* the Formation button: (620, 15)-(785, 35) */
#define VIEWS_TACTICS 64            /* the most tactics in the Formation list */

/* A tactic of the Formation list: its name as the game's Tactics drop-down shows it and the position
 * mask of each of its 11 slots. */
typedef struct {
    char name[64];
    uint16_t masks[11];
} Tactic;

/* The tactics of the manager's Tactics drop-down, the current tactic first. Returns the count. */
int tactics_list(uint32_t manager, Tactic *out, int max);

/* The pitch as views_widget_drawn draws it in the box x1..y2 (classic numbers), in classic numbers. It is
 * wide enough for five name boxes in a row (Q154). */
void views_pitch_rect(int *x1, int *y1, int *x2, int *y2);

/* Tab 1: the Formation button, the eleven on the pitch x1..y2 and the list of the other players in the
 * panel lx1..ly2 (classic numbers). Returns the number of tactics in the list, 0 when there is none. */
int best_xi_build(uint32_t club, uint32_t team, uint32_t buf, int x1, int y1, int x2, int y2, int lx1, int ly1,
                  int lx2, int ly2, uint32_t bar, uint32_t fg);
/* A click on a widget of id and data: 1 when it was a Formation item (rebuild the screen). */
int best_xi_input(uint32_t id, uint32_t data);

#endif
