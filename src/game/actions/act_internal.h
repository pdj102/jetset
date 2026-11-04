#ifndef ACT_INTERNAL_H
#define ACT_INTERNAL_H

/*
 * Shared by the action files; not for use outside them:
 * - act.c (common memory): act_do(), act_attempted() and the reason table;
 * - act_shared.c (BANK_56): helpers for the actions, not banked: call them
 *   only from code in BANK_56;
 * - act_<action>.c (BANK_56): each action.
 */

#include "game/actions/act.h"

/* Reason flags (act_reason_flags[]) */
#define ACT_RF_ATTEMPTED  0x01   /* the action was tried: the turn is used */

extern const uint8_t act_reason_flags[ACT_R_COUNT];

/*
 * The shared endings (act_shared.c, BANK_56). Functions, not macros of
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

/*
 * Steps 3 and 4 of an attack, on a->target by a->actor, once validated: the
 * d20 roll against attacker's attack of this use (ATTACK_MELEE, ATTACK_RANGED),
 * then on a hit the damage (sets a->damage and a->amount) and the hit effect of
 * hooks, the entity whose GEN_HOOK_HIT applies. to_hit and bonus add to the
 * roll and the damage (attack_hits(), attack_damage()). The events carry
 * a->item. A miss fails with ACT_R_MISSED. Returns ACT_SUCCESS or ACT_FAIL
 */
uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks, int8_t to_hit,
                   int8_t bonus);

/* Line modes (act_line()'s how): who a line reaches. See docs/actions.md, section 6.13 */
#define ACT_LINE_BOLT    0   /* the first creature on the line: what flies */
#define ACT_LINE_DIRECT  1   /* the creature on the chosen tile, past any on the way */
#define ACT_LINE_COUNT   2

/*
 * The line of fire, for validation: from the actor, placed in an area,
 * towards (a->x, a->y) within range (nge_map_within()). Sets a->target to the
 * first alive entity with health along it (ACT_LINE_BOLT) or on the chosen
 * tile (ACT_LINE_DIRECT), or NGE_ENTITY_INVALID. No random numbers, so a
 * check gets the same answer. Returns ACT_R_NONE, or why not:
 * ACT_R_OUT_OF_RANGE, ACT_R_NO_LINE (something solid first, the chosen tile
 * included), or ACT_R_NO_TARGET (nobody there, or the actor's own tile)
 */
uint8_t act_line(act_t *a, uint8_t range, uint8_t how);

/* 1 if id can't leave its tile (STATUS_ROOTED): an actor without stats has no statuses */
uint8_t act_rooted(nge_entity_id_t id);

/*
 * Steps 1 and 2 of the feature actions: the feature a->target, or (with
 * a->target NGE_ENTITY_INVALID) the one in direction a->dx, a->dy, put in
 * a->target. It must be alive, found (not FEAT_HIDDEN: unknown to the actor
 * is not there), able to do `can` (FCAN_*), in the actor's area, next to
 * the actor or on its tile. Returns ACT_R_NONE, or why not:
 * ACT_R_NOTHING_THERE, `cant` (it can't do that), ACT_R_OUT_OF_RANGE
 */
uint8_t act_find_feature(act_t *a, uint8_t can, uint8_t cant);

#endif
