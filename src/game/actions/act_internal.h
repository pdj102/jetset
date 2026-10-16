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

/* End an action that failed for reason: cost full_cost if attempted, 0 if refused. Returns ACT_FAIL */
uint8_t act_fail(act_t *a, uint8_t reason, uint8_t full_cost);

/* End an action that succeeded, costing cost ticks. Returns ACT_SUCCESS */
uint8_t act_succeed(act_t *a, uint8_t cost);

#endif
