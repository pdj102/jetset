#ifndef ATTACK_H
#define ATTACK_H

/**
 * @file attack.h
 * @brief The attack component: how an entity, or a weapon, attacks.
 *
 * One layout with two uses (ATTACK_*): a melee attack (COMPONENT_MELEE,
 * range 1) and later a ranged attack (COMPONENT_RANGED, the weapon's range,
 * using the ammo in the ammo slot). An entity can have both. A creature's
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

/* Uses of the component: the tables' rows */
#define ATTACK_MELEE  0
#define ATTACK_KINDS  1   /**< Uses with tables (ATTACK_RANGED comes with ranged weapons). */

/** Component bits: the game's sixth and (later) seventh components (see nge_entity.h). */
#define COMPONENT_MELEE   NGE_COMPONENT(NGE_COMPONENT_USER + 5)
#define COMPONENT_RANGED  NGE_COMPONENT(NGE_COMPONENT_USER + 6)   /**< Reserved. */

/** The component bit of a use. */
#define ATTACK_COMPONENT(kind)  ((kind) == ATTACK_MELEE ? COMPONENT_MELEE : COMPONENT_RANGED)

/** The d20 rolls that always hit and always miss. */
#define ATTACK_ROLL_HIT   20
#define ATTACK_ROLL_MISS  1

/** Base attack: the attack part of a creature's or an item's base record. */
typedef struct attack_base {
    int8_t  to_hit;   /**< Added to the d20 roll. */
    uint8_t dice;     /**< Damage dice, 0 for no attack. */
    uint8_t sides;    /**< Sides on each die, 1 or more. */
    int8_t  bonus;    /**< Added to the damage roll. */
    uint8_t type;     /**< Damage type (DMG_*). */
    uint8_t range;    /**< Tiles: 1 for melee. */
} attack_base_t;

/** A base record's attack part gives the component: it has damage dice. */
#define ATTACK_BASE_HAS(b)  ((b)->dice != 0)

/* Tables in slot 0 (attack_cdata.c): [use][entity], meaningful only with the use's component */
extern int8_t  attack_to_hit[ATTACK_KINDS][NGE_ENTITY_MAX];
extern uint8_t attack_dice[ATTACK_KINDS][NGE_ENTITY_MAX];
extern uint8_t attack_sides[ATTACK_KINDS][NGE_ENTITY_MAX];
extern int8_t  attack_bonus[ATTACK_KINDS][NGE_ENTITY_MAX];
extern uint8_t attack_type[ATTACK_KINDS][NGE_ENTITY_MAX];
extern uint8_t attack_range[ATTACK_KINDS][NGE_ENTITY_MAX];

/** The entity has an attack of this use. */
#define ATTACK_HAS(id, kind)  NGE_ENTITY_HAS_ANY_COMPONENTS(id, ATTACK_COMPONENT(kind))

/**
 * @brief Give an entity in use an attack of this use (ATTACK_MELEE), or
 *        replace it. A melee attack's range must be 1.
 * @param b In common memory (a copy on the stack): the call maps page 110.
 */
void attack_add(nge_entity_id_t id, uint8_t kind, const attack_base_t *b) NGE_BANKED;

/** @brief Remove the attack of this use. */
void attack_remove(nge_entity_id_t id, uint8_t kind) NGE_BANKED;

/**
 * @brief Does a d20 roll (1-20) with id's attack of this use hit target
 *        (with health)? 20 always hits, 1 always misses, otherwise roll +
 *        to_hit >= the target's AC. The one place for the hit rule: the
 *        attacker's own bonuses go here later.
 */
uint8_t attack_hits(nge_entity_id_t id, uint8_t kind, nge_entity_id_t target, uint8_t roll) NGE_BANKED;

/** @brief Roll id's damage with its attack of this use: dice d sides + bonus, 1 to 255. */
uint8_t attack_damage(nge_entity_id_t id, uint8_t kind) NGE_BANKED;

#endif
