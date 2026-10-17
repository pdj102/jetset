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
 *     if (a.used) {
 *         ...the turn is used
 *     }
 *
 * Inside, an action always takes four steps:
 * 1. context: look up what it needs (components, the target tile...);
 * 2. validate: the preconditions, with no randomness. ACT_CHECK mode stops
 *    here: success if the action could be done, or the reason it can't;
 * 3. resolve: roll any dice and decide the outcome, changing nothing yet;
 * 4. apply: change the game state (this can't fail), post events about
 *    what happened.
 * Check mode changes nothing, posts nothing and draws no random numbers, so
 * the AI can weigh up its options with it.
 *
 * Failure: a->reason says why. Each reason is one of two kinds, defined
 * once (act.c):
 * - refused: a precondition failed and nothing changed: the turn isn't
 *   used, no events (blocked by a wall, not consumable);
 * - attempted: the action was tried and didn't succeed: the turn is used
 *   and events say so (an attack that missed).
 * The action sets a->used from that (1 on success), so callers decide
 * whether the entity used its turn from used alone; act_attempted() tells
 * the two kinds apart for callers that care. Every action takes the same
 * time, one turn of the actor (its speed: game/components/stats.h).
 *
 * Actions don't call each other for choices: the caller combines them
 * (bump to attack: a move refused with ACT_R_BLOCKED_CREATURE, then a melee
 * on a.target). Consequences (damage, later) are effects with the same
 * pattern that use no turn, which an action's apply step may call.
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
    uint8_t used;             /**< 1 if the actor used its turn; 0 if refused, or in check mode. */
} act_t;

/* Results */
#define ACT_SUCCESS  1
#define ACT_FAIL     0

/* Modes */
#define ACT_DO     0   /**< Do the action. */
#define ACT_CHECK  1   /**< Only check its preconditions: change nothing. */

/* Actions, for act_do() */
#define ACT_MOVE    0
#define ACT_PICKUP  1
#define ACT_DROP    2
#define ACT_EQUIP   3
#define ACT_UNEQUIP 4
#define ACT_MELEE   5
#define ACT_COUNT   6

/* Reasons (refused or attempted: the table in act.c) */
#define ACT_R_NONE              0   /**< Succeeded. */
#define ACT_R_BLOCKED           1   /**< Refused: the map blocks the way (a wall, the edge). */
#define ACT_R_BLOCKED_CREATURE  2   /**< Refused: an entity blocks the way; a->target is it. */
#define ACT_R_NO_INVENTORY      3   /**< Refused: the actor can't carry things. */
#define ACT_R_NOTHING_HERE      4   /**< Refused: no item on the actor's tile (to pick up). */
#define ACT_R_NOT_ITEM          5   /**< Refused: the target can't be carried. */
#define ACT_R_FULL              6   /**< Refused: the inventory is full; a->target is the item. */
#define ACT_R_NOT_CARRIED       7   /**< Refused: the actor doesn't have the item. */
#define ACT_R_EQUIPPED          8   /**< Refused: the item is equipped (unequip it first). */
#define ACT_R_ALREADY_EQUIPPED  9   /**< Refused: the item is equipped already. */
#define ACT_R_CANT_EQUIP        10  /**< Refused: the item fits no equipment slot (a potion). */
#define ACT_R_NO_SLOT           11  /**< Refused: the actor has no slot the item fits (or not the one asked for). */
#define ACT_R_SLOT_TAKEN        12  /**< Refused: every slot it fits is in use; a->target is the item there. */
#define ACT_R_NOT_EQUIPPED      13  /**< Refused: the item isn't equipped. */
#define ACT_R_NO_ATTACK         14  /**< Refused: no weapon with a melee attack, and none of its own. */
#define ACT_R_NO_TARGET         15  /**< Refused: nothing there to attack. */
#define ACT_R_CANT_DAMAGE       16  /**< Refused: the target can't be damaged (no health, or dead). */
#define ACT_R_OUT_OF_RANGE      17  /**< Refused: the target is out of the attack's reach. */
#define ACT_R_MISSED            18  /**< Attempted: the attack missed (the turn is used). */
#define ACT_R_COUNT             19

/**
 * @brief Move one tile.
 *
 * In: actor (alive, placed in an area), dx, dy (each -1, 0 or 1, not both
 * 0). Succeeds if the actor can enter the tile (nge_block_can_enter(),
 * with its pass flags): it moves there and posts EV_MOVED, and EV_STOOD_ON
 * for each entity on the tile. Refused
 * with ACT_R_BLOCKED or ACT_R_BLOCKED_CREATURE (a->target: the blocker).
 */
uint8_t act_move(act_t *a, uint8_t mode) NGE_BANKED;

/**
 * @brief Pick up an item from the floor of the actor's tile and stow it
 *        (game/components/inventory.h).
 *
 * In: actor (alive, placed in an area), target (the item, or
 * NGE_ENTITY_INVALID for the first item on the tile). Succeeds if the
 * actor has an inventory with room and the item is on its tile: posts
 * EV_PICKED_UP; a->target is the item. Refused with ACT_R_NO_INVENTORY,
 * ACT_R_NOTHING_HERE (no item there, or target isn't on the tile),
 * ACT_R_NOT_ITEM or ACT_R_FULL.
 */
uint8_t act_pickup(act_t *a, uint8_t mode) NGE_BANKED;

/**
 * @brief Drop a stowed item on the floor of the actor's tile.
 *
 * In: actor (alive, placed in an area), item. Succeeds if the actor has
 * the item stowed: posts EV_DROPPED. Refused with ACT_R_NOT_CARRIED or
 * ACT_R_EQUIPPED.
 */
uint8_t act_drop(act_t *a, uint8_t mode) NGE_BANKED;

/**
 * @brief Equip a stowed item: wield a weapon, put on a ring.
 *
 * In: actor (alive), item, kind (the slot to use, SLOT_* in game/
 * components/item.h; SLOT_NONE for the first free slot the item fits).
 * Succeeds if the actor has the item stowed, it fits a slot the actor
 * has, and that slot is free: posts EV_EQUIPPED (p1: the slot); a->kind
 * is the slot used. Equipped items don't count towards the capacity.
 * Refused with ACT_R_NO_INVENTORY, ACT_R_NOT_CARRIED,
 * ACT_R_ALREADY_EQUIPPED, ACT_R_CANT_EQUIP, ACT_R_NO_SLOT or
 * ACT_R_SLOT_TAKEN (a->target: the item in the slot).
 */
uint8_t act_equip(act_t *a, uint8_t mode) NGE_BANKED;

/**
 * @brief Take off an equipped item, back into the inventory.
 *
 * In: actor (alive), item. Succeeds if the actor has the item equipped and
 * room to stow it: posts EV_UNEQUIPPED. Refused with ACT_R_NOT_CARRIED,
 * ACT_R_NOT_EQUIPPED or ACT_R_FULL.
 */
uint8_t act_unequip(act_t *a, uint8_t mode) NGE_BANKED;

/**
 * @brief Attack an entity next to the actor (game/components/attack.h).
 *
 * In: actor (alive, placed in an area), and target, or (with target
 * NGE_ENTITY_INVALID) a direction dx, dy: the first entity with health on
 * that tile. Uses the melee attack of the weapon in the actor's
 * SLOT_WEAPON, or else its own. Out: target; item, the weapon
 * (NGE_ENTITY_INVALID for its own attack); on a hit amount, the damage
 * taken, and kind, its type.
 *
 * A hit (1d20 + to hit >= the target's AC; 20 always hits, 1 always
 * misses) posts EV_HIT (p1: the roll, p2: the weapon) and damages the
 * target (health_damage(): EV_DAMAGED, maybe EV_DIED): success. A miss
 * posts EV_MISSED and fails with ACT_R_MISSED, using the turn. Refused with
 * ACT_R_NO_ATTACK, ACT_R_NO_TARGET, ACT_R_CANT_DAMAGE or ACT_R_OUT_OF_RANGE.
 */
uint8_t act_melee(act_t *a, uint8_t mode) NGE_BANKED;

/** @brief Call action (ACT_MOVE...) by number: for the AI and delayed actions. */
uint8_t act_do(uint8_t action, act_t *a, uint8_t mode);

/** @brief 1 if reason is an attempted failure (the turn is used), 0 if refused or none. */
uint8_t act_attempted(uint8_t reason);

#endif
