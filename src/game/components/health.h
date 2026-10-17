#ifndef HEALTH_H
#define HEALTH_H

/**
 * @file health.h
 * @brief The health component: an entity that can be damaged, and is
 *        destroyed when its hit points reach 0 (the player, monsters,
 *        later chests and doors).
 *
 * Each has hit points (hp, at most hp_max), an armour class (ac: what an
 * attack roll must reach to hit it, game/components/attack.h) and three
 * masks of damage types (DMG_BIT()): those it is immune to, resists and is
 * vulnerable to. Anything that can be damaged can be attacked, so AC is
 * here. Its base values come from its kind's base record (health_base_t,
 * game/gen).
 *
 * Damage is an amount and a type (DMG_*), decided by the caller: an attack,
 * a trap, poison. What the entity takes:
 *
 *     immune               0
 *     resistant            half, rounded up (1 stays 1)
 *     vulnerable           double (at most 255)
 *     resistant and vulnerable: they cancel, the amount as it is
 *
 * health_damage() always posts EV_DAMAGED, even when 0 is taken: p1 the
 * amount taken, p2 the outcome and type packed (EV_DMG_OUTCOME(),
 * EV_DMG_TYPE()). At 0 hp it posts EV_DIED and destroys the entity, unless
 * it is persistent (NGE_ENTITY_PERSISTENT: the player), when the game
 * reacts to EV_DIED. An entity at 0 hp takes no more damage and can't be
 * healed.
 *
 * hp_max, ac and the masks are attributes (game/components/attrs.h): the
 * base's values are their bases, and effects_recompute() (game/effects/
 * effects.h) derives them from the base and the modifiers of equipment and
 * effects. Change them only through the effects.
 *
 * The tables are in slot 0 (health_cdata.c). Banking: the functions are
 * banked (NGE_BANKED, page 110, with the other component functions).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

/** Component bit: the game's fifth component (see nge_entity.h). */
#define COMPONENT_HEALTH  NGE_COMPONENT(NGE_COMPONENT_USER + 4)

/* Damage types: a number in events and calls, a bit (DMG_BIT()) in the masks */
#define DMG_BLUNT   0
#define DMG_SLASH   1
#define DMG_PIERCE  2
#define DMG_FIRE    3
#define DMG_COLD    4
#define DMG_POISON  5
#define DMG_ACID    6
#define DMG_MAGIC   7
#define DMG_COUNT   8

/** Damage type t as a mask bit. */
#define DMG_BIT(t)  ((uint8_t)(1 << (t)))

/* Outcomes: what the entity's masks did to the damage */
#define DMG_NORMAL      0
#define DMG_RESISTED    1
#define DMG_VULNERABLE  2
#define DMG_IMMUNE      3

/* EV_DAMAGED's p2: the outcome in the high 4 bits, the type in the low 4 */
#define EV_DMG_PACK(outcome, type)  ((uint8_t)(((outcome) << 4) | (type)))
#define EV_DMG_OUTCOME(p2)          ((uint8_t)((p2) >> 4))
#define EV_DMG_TYPE(p2)             ((uint8_t)((p2) & 0x0F))

/** Base health: the health part of a kind's base record. */
typedef struct health_base {
    uint8_t hp;         /**< Hit points when unhurt (hp_max), the same for every entity of the kind; 0 for none. */
    uint8_t immune;     /**< DMG_BIT() mask. */
    uint8_t resist;
    uint8_t vuln;
    uint8_t ac;         /**< Armour class: 10 unarmoured, 1 a door (hard to miss). */
} health_base_t;

/** A base record's health part gives the component: it has some hit points. */
#define HEALTH_BASE_HAS(b)  ((b)->hp != 0)

/* Tables in slot 0 (health_cdata.c): read-only for game code; meaningful only with COMPONENT_HEALTH */
extern uint8_t health_hp[NGE_ENTITY_MAX];       /**< Hit points now: 0 is dead. */
extern uint8_t health_hp_max[NGE_ENTITY_MAX];   /**< Hit points when unhurt, 1 or more (derived: ATTR_HP_MAX). */
extern uint8_t health_immune[NGE_ENTITY_MAX];   /**< Damage types it takes nothing from. */
extern uint8_t health_resist[NGE_ENTITY_MAX];   /**< ...takes half from. */
extern uint8_t health_vuln[NGE_ENTITY_MAX];     /**< ...takes double from. */
extern uint8_t health_ac[NGE_ENTITY_MAX];       /**< Armour class. */

/**
 * @brief Give an entity in use the health component from base health:
 *        unhurt with the base's hit points (1 or more), its masks and AC,
 *        which are also its attributes' bases; no statuses.
 * @param b In common memory (a copy on the stack): the call maps page 110.
 */
void health_add(nge_entity_id_t id, const health_base_t *b) NGE_BANKED;

/**
 * @brief What amount of damage of type would do to an entity with health,
 *        changing nothing: the amount it would take, and in *outcome
 *        (if not NULL) DMG_NORMAL, DMG_RESISTED, DMG_VULNERABLE or DMG_IMMUNE.
 */
uint8_t health_dealt(nge_entity_id_t id, uint8_t amount, uint8_t type, uint8_t *outcome) NGE_BANKED;

/**
 * @brief Damage an entity with health (see above): posts EV_DAMAGED, and
 *        at 0 hp EV_DIED and destroys it (unless persistent). Does nothing
 *        to an entity at 0 hp.
 * @param source The entity that caused it, or NGE_ENTITY_INVALID (a trap).
 * @return The amount taken.
 */
uint8_t health_damage(nge_entity_id_t target, nge_entity_id_t source, uint8_t amount,
                      uint8_t type) NGE_BANKED;

/**
 * @brief Restore hit points, up to hp_max: posts EV_HEALED (p1: the hit
 *        points restored) if any were. Does nothing to an entity at 0 hp.
 * @return The hit points restored.
 */
uint8_t health_heal(nge_entity_id_t target, uint8_t amount) NGE_BANKED;

#endif
