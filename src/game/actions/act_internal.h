#ifndef ACT_INTERNAL_H
#define ACT_INTERNAL_H

/*
 * Shared by the action files; not for use outside them:
 * - act.c (common memory): act_do(), act_attempted() and the reason table;
 * - act_shared.c (page 112): helpers for the actions, not banked: call them
 *   only from code in page 112;
 * - act_<action>.c (page 112): each action.
 */

#include "game/actions/act.h"

/* Reason flags (act_reason_flags[]) */
#define ACT_RF_ATTEMPTED  0x01   /* the action was tried: the turn is used */

extern const uint8_t act_reason_flags[ACT_R_COUNT];

/*
 * The shared endings (act_shared.c, page 112). Functions, not macros of
 * comma expressions: zsdcc (z88dk, Oct 2026) dropped the assignments in
 * `return (a->reason = r, a->cost = c, ACT_FAIL);`
 */

/*
 * End an action that failed for reason: used if the reason is attempted,
 * not if refused. Validation (which check mode runs) fails only with
 * refused reasons. Returns ACT_FAIL
 */
uint8_t act_fail(act_t *a, uint8_t reason);

/* End an action that succeeded: the turn is used. Returns ACT_SUCCESS */
uint8_t act_succeed(act_t *a);

/* End a check (ACT_CHECK) that found the action possible: nothing used. Returns ACT_SUCCESS */
uint8_t act_checked(act_t *a);

/* The distance between two tiles, in moves: the larger of the x and y distances */
uint8_t act_distance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

#endif
