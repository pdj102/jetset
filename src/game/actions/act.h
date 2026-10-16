#ifndef ACT_H
#define ACT_H

/**
 * @file act.h
 * @brief Actions: what an entity does (move; later attack, pick up, open,
 *        consume...), the same for the player and for monsters.
 *
 * Every action has the same API: it takes a context (act_t, on the
 * caller's stack) and a mode, and returns ACT_SUCCESS or ACT_FAIL:
 *
 *     act_t a = { 0 };
 *     a.actor = id;
 *     a.dx = 1;
 *     if (act_move(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED_CREATURE) {
 *         ...a.target is in the way: attack it?
 *     }
 *     if (a.cost != 0) {
 *         ...the turn is used: a.cost ticks
 *     }
 *
 * Inside, an action always takes four steps:
 * 1. context: look up what it needs (components, the target tile...);
 * 2. validate: the preconditions, with no randomness. ACT_CHECK mode stops
 *    here: success if the action could be done, or the reason it can't;
 * 3. resolve: roll any dice and decide the outcome, changing nothing yet;
 * 4. apply: change the game state (this can't fail), post events about
 *    what happened, set the cost.
 * Check mode changes nothing, posts nothing and draws no random numbers, so
 * the AI can weigh up its options with it.
 *
 * Failure: a->reason says why. Each reason is one of two kinds, defined
 * once (act.c):
 * - refused: a precondition failed and nothing changed: no cost, no
 *   events (blocked by a wall, not consumable);
 * - attempted: the action was tried and didn't succeed: the turn is used
 *   and events say so (an attack that missed).
 * The action sets a->cost from that (0 when refused), so callers decide
 * whether the entity used its turn from cost alone; act_attempted() tells
 * the two kinds apart for callers that care.
 *
 * Actions don't call each other for choices: the caller combines them
 * (bump to attack: a move refused with ACT_R_BLOCKED_CREATURE, then a melee
 * on a.target). Consequences (damage, later) are effects with the same
 * pattern and no cost, which an action's apply step may call.
 *
 * Callers: player commands, monster AI, and timer and event handlers (a
 * delayed action is a timer whose handler fills a context and calls
 * act_do()).
 *
 * Banking: each action is banked in the actions page pair (NGE_BANKED, page
 * 112); act_do() and act_attempted() are in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

/** An action's context: inputs set by the caller, outputs set by the action. */
typedef struct act {
    /* In: what each action reads is listed with it */
    nge_entity_id_t actor;    /**< Who acts. */
    nge_entity_id_t target;   /**< An entity acted on; out: the blocker when refused for one. */
    nge_entity_id_t item;     /**< An item used (a weapon, a key). */
    int8_t  dx;               /**< A direction... */
    int8_t  dy;
    uint8_t x;                /**< ...or a tile. */
    uint8_t y;
    uint8_t kind;             /**< Extra input, e.g. a damage type... */
    uint8_t amount;           /**< ...and amount. */
    /* Out */
    uint8_t reason;           /**< ACT_R_*: why it failed; ACT_R_NONE on success. */
    uint8_t cost;             /**< Ticks the action used: 0 if refused, or in check mode. */
} act_t;

/* Results */
#define ACT_SUCCESS  1
#define ACT_FAIL     0

/* Modes */
#define ACT_DO     0   /**< Do the action. */
#define ACT_CHECK  1   /**< Only check its preconditions: change nothing. */

/* Actions, for act_do() */
#define ACT_MOVE   0
#define ACT_COUNT  1

/* Reasons (refused or attempted: the table in act.c) */
#define ACT_R_NONE              0   /**< Succeeded. */
#define ACT_R_BLOCKED           1   /**< Refused: the map blocks the way (a wall, the edge). */
#define ACT_R_BLOCKED_CREATURE  2   /**< Refused: an entity blocks the way; a->target is it. */
#define ACT_R_COUNT             3

/**
 * @brief Move one tile.
 *
 * In: actor (alive, placed in an area, with stats), dx, dy (each -1, 0 or
 * 1, not both 0). Succeeds if the actor can enter the tile
 * (nge_block_can_enter(), with its pass flags): it moves there, costing its
 * speed (game/components/stats.h),
 * and posts EV_MOVED and EV_STOOD_ON for each entity on the tile. Refused
 * with ACT_R_BLOCKED or ACT_R_BLOCKED_CREATURE (a->target: the blocker).
 */
uint8_t act_move(act_t *a, uint8_t mode) NGE_BANKED;

/** @brief Call action (ACT_MOVE...) by number: for the AI and delayed actions. */
uint8_t act_do(uint8_t action, act_t *a, uint8_t mode);

/** @brief 1 if reason is an attempted failure (the turn is used), 0 if refused or none. */
uint8_t act_attempted(uint8_t reason);

#endif
