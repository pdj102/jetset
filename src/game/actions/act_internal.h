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
 * then on a hit the damage (sets a->kind and a->amount) and the hit effect of
 * hooks, the entity whose GEN_HOOK_HIT applies. to_hit and bonus add to the
 * roll and the damage (attack_hits(), attack_damage()). The events carry
 * a->item. A miss fails with ACT_R_MISSED. Returns ACT_SUCCESS or ACT_FAIL
 */
uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks, int8_t to_hit,
                   int8_t bonus);

/*
 * The line of fire, for validation: from the actor, placed in an area,
 * towards (a->x, a->y) within range (nge_map_within()). Sets a->target to the
 * first alive entity with health along it, or NGE_ENTITY_INVALID. No random
 * numbers, so a check gets the same answer. Returns ACT_R_NONE, or why not:
 * ACT_R_OUT_OF_RANGE, ACT_R_NO_LINE (something solid first, the chosen tile
 * included), or ACT_R_NO_TARGET (nobody there, or the actor's own tile)
 */
uint8_t act_line(act_t *a, uint8_t range);

/* 1 if id can't leave its tile (STATUS_ROOTED): an actor without stats has no statuses */
uint8_t act_rooted(nge_entity_id_t id);

#endif
