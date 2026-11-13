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
 * `return (a->reason = r, a->used = u, ACT_FAIL);`
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
 * End an action that succeeded and uses no turn: look only (docs/action_look.md
 * D2). The same as act_checked(), so it costs nothing. A #define to a
 * function, not a comma expression (T-ZSDCC-2). Returns ACT_SUCCESS
 */
#define act_succeed_free(a)  act_checked(a)

/*
 * Steps 3 and 4 of an attack, on a->target by a->actor, once validated: the
 * d20 roll against attacker's attack of this use (ATTACK_MELEE, ATTACK_RANGED),
 * then on a hit the damage (sets a->damage and a->amount) and the hit effect of
 * hooks, the entity whose GEN_HOOK_HIT applies. to_hit and to_damage add to the
 * roll and the damage (attack_hits(), attack_damage()). The events carry
 * a->item. A miss fails with ACT_R_MISSED. Returns ACT_SUCCESS or ACT_FAIL
 */
uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks, int8_t to_hit,
                   int8_t to_damage);

/* Line modes (act_line()'s how): who a line reaches. See docs/actions.md, section 4.11 */
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

/*
 * Who a shape (SPELL_*) reaches, for act_aim() and act_cast(): found by
 * act_shape_find() in validation, then applied by act_shape_apply(). On the
 * action's stack. See docs/items_aimed.md, section 4.3
 */
typedef struct act_shape {
    uint8_t shape;                       /* in: SPELL_* */
    uint8_t cx, cy;                      /* out: a tile's or an area's centre */
    uint8_t count;                       /* out: the creatures in ids[] */
    nge_entity_id_t ids[ACT_AREA_MAX];   /* out: an area's creatures, nearest first */
} act_shape_t;

/*
 * Validate: who s->shape reaches from a->actor towards (a->x, a->y) within
 * range. Sets a->target (the actor for self and tile) and s's outputs. No
 * random numbers, so a check gets the same answer. Returns ACT_R_NONE, or
 * why not: act_line()'s reasons; for a tile, ACT_R_BLOCKED_ENTITY (a->target:
 * the creature) or ACT_R_BLOCKED, and ACT_R_NO_TARGET for the actor's own tile
 */
uint8_t act_shape_find(act_t *a, act_shape_t *s, uint8_t range);

/* Apply: effect ef at power to whom act_shape_find() found, with the actor as its source */
void act_shape_apply(act_t *a, const act_shape_t *s, uint8_t ef, int8_t power);

/* 1 if id can't leave its tile (STATUS_ROOTED): an actor without stats has no statuses */
uint8_t act_rooted(nge_entity_id_t id);

/* 1 if id has any of statuses s (STATUS_*): an actor without stats has none */
uint8_t act_status(nge_entity_id_t id, uint8_t s);

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
