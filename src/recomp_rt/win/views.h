/* Shared parts of the Team views screen (views.c), its Best XI tab (best_xi.c) and the tactic list
 * (tactics.c). */
#ifndef RECOMP_VIEWS_H
#define RECOMP_VIEWS_H

#include <stddef.h>
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

/* The comparison of two players (compare.c): variant B in the box x1..y2 (classic numbers). club not 0
 * makes each name box a drop-down with the club's squad (the Compare tab), at page page1 and page2. text: VIEWS_TEXT_SIZE bytes of
 * the game's memory. */
void compare_build(uint32_t text, uint32_t staff1, uint32_t staff2, int x1, int y1, int x2, int y2, uint32_t club,
                   int page1, int page2);
/* A click on a drop-down item: the side (1 or 2) with its new player in *value, or a page item of side 1
 * or 2 (3 or 4) with the page in *value, or 0. Each drop-down shows its squad in pages that fit the screen. */
int compare_input(uint32_t club, uint32_t id, uint32_t data, uint32_t *value);
/* The first two players of the club's squad, 0 when there are none. */
void compare_first_two(uint32_t club, uint32_t *a, uint32_t *b);
/* Forgets the comparison built last: call it when a screen or tab is built that is not a comparison. */
void compare_clear(void);
/* After the game draws a widget: the polygon and the frames of the comparison built last. */
void compare_widget_drawn(uint32_t widget);
/* Parts of best_xi.c for the Squad depth tab. views_formation builds the Formation button and list and
 * gives the chosen tactic; it returns 1 + its index in the list, 0 when there is none. views_text: a
 * text widget with no panel (flags 1 left, 0xc centred). The score text is the number of Q156, 1 to 20. */
int views_formation(uint32_t buf, uint32_t bar, Tactic *chosen);
void views_text(uint32_t buf, int x1, int y1, int x2, int y2, uint32_t text_colour, uint32_t flags, const char *text);
/* A box filled with one colour (RGB565) and no text. */
void views_box(uint32_t buf, int x1, int y1, int x2, int y2, uint32_t colour);
void views_short_name(uint32_t staff, char *out, size_t size);
int views_shown_score(int value);      /* 0 for a player with no score */
void views_score_text(int shown, char *out, size_t size, uint32_t *colour);   /* "-" when 0 */
void views_position_text(uint16_t mask, char *out, size_t size);
uint32_t views_human_manager(void);

/* Tab 2: the Formation button and a row for each position of the chosen tactic with its top players, on a
 * dark panel at x1..y2 (classic numbers). */
void squad_depth_build(uint32_t club, uint32_t team, uint32_t buf, int x1, int y1, int x2, int y2, uint32_t bar);

#endif
