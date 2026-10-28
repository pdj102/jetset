#ifndef ATTACK_H
#define ATTACK_H

/**
 * @file attack.h
 * @brief The attack component: how an entity, or a weapon, attacks.
 *
 * One layout with two uses (ATTACK_*): a melee attack (COMPONENT_MELEE,
 * range 1) and a ranged attack (COMPONENT_RANGED, range 2 or more; later
 * the weapon's range, using the ammo in the ammo slot). An entity can have
 * both. No kind has a ranged attack yet, and no action fires one
 * (docs/ranged_ai_components.md). A creature's
 * own melee attack is its natural one (fists, a bite); a weapon (a sword)
 * has one too, and an attacker uses its wielded weapon's (SLOT_WEAPON)
 * before its own (game/actions/act_melee.c).
 *
 * An attack hits if 1d20 + to_hit >= the target's AC (game/components/
 * health.h); a roll of 20 always hits and a 1 always misses. A hit does
 * dice d sides + bonus damage of its type, at least 1.
 *
 * The tables are in slot 0 (attack_cdata.c), one row per use. Banking: the
 * functions are banked (NGE_BANKED, page 110, with the other component
 * functions).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* Uses of the component: the tables' rows */
#define ATTACK_MELEE      0
#define ATTACK_RANGED     1
#define ATTACK_USE_COUNT  2

/** The component bit of a use. */
#define ATTACK_COMPONENT(use)  ((use) == ATTACK_MELEE ? COMPONENT_MELEE : COMPONENT_RANGED)

/** The d20 rolls that always hit and always miss. */
#define ATTACK_ROLL_HIT   20
#define ATTACK_ROLL_MISS  1

/** Kind attack: the attack part of a creature's or an item's kind record (melee only, so far). */
typedef struct attack_kind {
    int8_t  to_hit;   /**< Added to the d20 roll. */
    uint8_t dice;     /**< Damage dice, 0 for no attack. */
    uint8_t sides;    /**< Sides on each die, 1 or more. */
    int8_t  bonus;    /**< Added to the damage roll. */
    uint8_t type;     /**< Damage type (DMG_*). */
    uint8_t range;    /**< Tiles: 1 for melee, 2 or more for ranged. */
} attack_kind_t;

/** A kind record's attack part gives the component: it has damage dice. */
#define ATTACK_KIND_HAS(k)  ((k)->dice != 0)

/* Tables in slot 0 (attack_cdata.c): [use][entity], meaningful only with the use's component */
extern int8_t  attack_to_hit[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_dice[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_sides[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern int8_t  attack_bonus[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_type[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_range[ATTACK_USE_COUNT][NGE_ENTITY_MAX];

/** The entity has an attack of this use. */
#define ATTACK_HAS(id, use)  NGE_ENTITY_HAS_COMPONENT(id, ATTACK_COMPONENT(use))

/**
 * @brief Give an entity in use an attack of this use (ATTACK_*), or
 *        replace it. A melee attack's range must be 1, a ranged one's 2 or
 *        more.
 * @param k In common memory (a copy on the stack): the call maps page 110.
 */
void attack_add(nge_entity_id_t id, uint8_t use, const attack_kind_t *k) NGE_BANKED;

/** @brief Remove the attack of this use. */
void attack_remove(nge_entity_id_t id, uint8_t use) NGE_BANKED;

/**
 * @brief Does a d20 roll (1-20) with id's attack of this use hit target
 *        (with health)? 20 always hits, 1 always misses, otherwise roll +
 *        to_hit >= the target's AC. The one place for the hit rule: the
 *        attacker's own bonuses go here later.
 */
uint8_t attack_hits(nge_entity_id_t id, uint8_t use, nge_entity_id_t target, uint8_t roll) NGE_BANKED;

/** @brief Roll id's damage with its attack of this use: dice d sides + bonus, 1 to 255. */
uint8_t attack_damage(nge_entity_id_t id, uint8_t use) NGE_BANKED;

/**
 * @brief The melee weapon id wields (SLOT_WEAPON, with a melee attack),
 *        whose attack it makes before its own; NGE_ENTITY_INVALID if none
 *        (no inventory, nothing wielded, or not a melee weapon).
 */
nge_entity_id_t attack_weapon(nge_entity_id_t id) NGE_BANKED;

#endif
