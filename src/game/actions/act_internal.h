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

/**
 * @brief End an action that failed.
 *
 * Validation (what check mode runs) fails only with refused reasons, so a
 * check never uses the turn.
 *
 * @param a       The action. Out: a->reason is set to reason, and a->used to
 *                1 if the reason is attempted (ACT_RF_ATTEMPTED in
 *                act_reason_flags[]), or 0 if it is refused.
 * @param reason  Why it failed: an ACT_R_*, not ACT_R_NONE (asserted).
 * @return ACT_FAIL, always.
 */
uint8_t act_fail(act_t *a, uint8_t reason);

/**
 * @brief End an action that succeeded: the turn is used.
 *
 * @param a  The action. Out: a->reason is set to ACT_R_NONE and a->used to 1.
 * @return ACT_SUCCESS, always.
 */
uint8_t act_succeed(act_t *a);

/**
 * @brief End a check (ACT_CHECK) that found the action possible: nothing is used.
 *
 * @param a  The action. Out: a->reason is set to ACT_R_NONE and a->used to 0.
 * @return ACT_SUCCESS, always.
 */
uint8_t act_checked(act_t *a);

/**
 * @brief End an action that succeeded and uses no turn: look only
 *        (docs/action_look.md D2).
 *
 * The same as act_checked(), so it costs nothing. A #define to a function,
 * not a comma expression (T-ZSDCC-2).
 *
 * @param a  The action. Out: a->reason is set to ACT_R_NONE and a->used to 0.
 * @return ACT_SUCCESS, always.
 */
#define act_succeed_free(a)  act_checked(a)

/**
 * @brief Steps 3 and 4 of an attack, once validated: resolve and apply.
 *
 * Rolls a d20 against a->target's AC with the attacker's attack of this use
 * (attack_hits()). On a hit, it deals the damage (attack_damage(),
 * health_damage()), then applies hooks' hit effect for this use, unless the hit
 * killed the target. The events (EV_HIT, EV_MISSED) are posted to a->target,
 * from a->actor, carrying a->item.
 *
 * @param a          The action: a->actor attacks a->target, and a->item is
 *                   what the events name. Out: on a hit, a->damage is the
 *                   damage type (DMG_*) and a->amount the damage taken.
 *                   a->reason and a->used as act_succeed() or act_fail().
 * @param attacker   The entity whose attack part is rolled: a->actor itself
 *                   (fists, a bite, a natural ranged attack), its weapon or
 *                   launcher, or a thrown item.
 * @param use        Which of attacker's attacks: ATTACK_MELEE or ATTACK_RANGED.
 * @param hooks      The entity whose hit effect for this use a hit applies
 *                   (gen_hit_of()): the weapon, the shot (an arrow), or the
 *                   creature itself (its melee_hit for a melee hit).
 * @param to_hit     Added to the roll: the actor's and the ammo's bonuses.
 * @param to_damage  Added to the damage: the actor's and the ammo's bonuses.
 * @return ACT_SUCCESS on a hit, or ACT_FAIL with ACT_R_MISSED on a miss. A
 *         miss is attempted: the turn is used.
 */
uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks, int8_t to_hit,
                   int8_t to_damage);

/* Line modes (act_line()'s how): who a line reaches. See docs/actions.md, section 4.11 */
#define ACT_LINE_BOLT    0   /* the first creature on the line: what flies */
#define ACT_LINE_DIRECT  1   /* the creature on the chosen tile, past any on the way */
#define ACT_LINE_COUNT   2

/**
 * @brief The line of fire, for validation: who a line from the actor reaches.
 *
 * Walks from a->actor, which must be placed in an area, towards the tile
 * (a->x, a->y). Anything solid or off the map stops the line; pits don't.
 * Uses no random numbers, so a check gets the same answer as the action.
 *
 * @param a      The action: from a->actor towards (a->x, a->y). Out:
 *               a->target is the alive entity with health that the line
 *               reaches, or NGE_ENTITY_INVALID.
 * @param range  The farthest the chosen tile may be (nge_map_within()).
 * @param how    Who the line reaches: ACT_LINE_BOLT, the first creature on
 *               the way; ACT_LINE_DIRECT, only one on the chosen tile.
 * @return ACT_R_NONE if it reaches a creature, or why not:
 *         ACT_R_OUT_OF_RANGE (the tile is beyond range), ACT_R_NO_LINE
 *         (something solid first, the chosen tile included), or
 *         ACT_R_NO_TARGET (nobody there, or the chosen tile is the actor's
 *         own).
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

/**
 * @brief Validate a shape: who it reaches from the actor.
 *
 * Self reaches the actor. A burst reaches the creatures around the actor,
 * not the actor, and so does SPELL_ALLIES, whose applying passes over
 * those with no AI. Bolt and direct are act_line(). A tile must be free, and a
 * ball catches every creature around the chosen tile, the actor too. Uses no
 * random numbers, so a check gets the same answer as the action.
 *
 * @param a      The action: from a->actor towards (a->x, a->y), which self
 *               and burst don't read. Out: a->target is the creature
 *               reached (the actor for self and tile), or the creature in
 *               the way for ACT_R_BLOCKED_ENTITY.
 * @param s      In: s->shape, a SPELL_* (asserted). Out: s->count is 0, or
 *               for an area the creatures in s->ids[]; s->cx, s->cy are the
 *               centre for a tile or an area.
 * @param range  The farthest the chosen tile may be. Self and burst ignore it.
 * @return ACT_R_NONE, or why not: act_line()'s reasons; for a tile,
 *         ACT_R_BLOCKED_ENTITY (a creature is on it) or ACT_R_BLOCKED, and
 *         ACT_R_NO_TARGET for the actor's own tile.
 */
uint8_t act_shape_find(act_t *a, act_shape_t *s, uint8_t range);

/**
 * @brief Apply an effect to whom act_shape_find() found, with the actor as its source.
 *
 * A tile applies it to the actor at that tile (effect_apply_at(): blink). An area posts EV_AREA,
 * then applies to each creature found that an effect before hasn't killed: for SPELL_ALLIES,
 * only those with AI.
 * Self, bolt and direct apply to a->target.
 *
 * @param a      The action that act_shape_find() validated: a->actor is the
 *               source, and a->target the creature for self, bolt and direct.
 * @param s      The shape as act_shape_find() filled it.
 * @param ef     The effect (EF_*).
 * @param power  Multiplies the amounts in the effect's definition.
 */
void act_shape_apply(act_t *a, const act_shape_t *s, uint8_t ef, int8_t power);

/**
 * @brief Whether an entity can't leave its tile (STATUS_ROOTED).
 *
 * @param id  The entity. One without stats has no statuses.
 * @return 1 if it is rooted, else 0.
 */
uint8_t act_rooted(nge_entity_id_t id);

/**
 * @brief Whether an entity has any of some statuses.
 *
 * @param id  The entity. One without stats has no statuses.
 * @param s   The statuses: a mask of STATUS_* bits.
 * @return 1 if it has any of them, else 0.
 */
uint8_t act_status(nge_entity_id_t id, uint8_t s);

/**
 * @brief Steps 1 and 2 of the feature actions: find and check the feature.
 *
 * The feature must be alive, found (not FEAT_HIDDEN: unknown to the actor is
 * not there), able to do can, in the actor's area, and next to the actor or
 * on its tile.
 *
 * @param a     The action: a->actor, which must be placed, and a->target, the
 *              feature, or NGE_ENTITY_INVALID to look in direction a->dx,
 *              a->dy (each -1 to 1). Out: a->target is the feature found
 *              there, or NGE_ENTITY_INVALID.
 * @param can   What the action needs it to do: an FCAN_* mask.
 * @param cant  The reason (ACT_R_*) to return when it isn't a feature or
 *              can't do that.
 * @return ACT_R_NONE, or why not: ACT_R_NOTHING_THERE (no feature, or a
 *         hidden one), cant, or ACT_R_OUT_OF_RANGE.
 */
uint8_t act_find_feature(act_t *a, uint8_t can, uint8_t cant);

#endif
