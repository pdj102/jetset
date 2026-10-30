#ifndef STATS_H
#define STATS_H

/**
 * @file stats.h
 * @brief The stats component: a creature's speed and its six abilities
 *        (strength, dexterity, constitution, intelligence, wisdom,
 *        charisma), the stats every creature has that can change during
 *        play.
 *
 * Not here: hit points and AC (game/components/health.h), to hit and
 * damage (game/components/attack.h), mana points (game/components/
 * mana.h); later, level and experience (the player). Stats fixed by kind
 * stay in the kind record (game/gen).
 *
 * An ability score is 1-30, 10-11 average; its modifier, STATS_MOD(), is
 * (score - 10) / 2 rounded down: -5 at 1, +10 at 30. Speed and the
 * abilities are attributes (game/components/attrs.h): stats_add() sets
 * their bases and values from their kind values (stats_kind_t, part of a
 * creature's kind record in game/gen/), and effects change them
 * (game/effects/effects.h).
 *
 * A creature's statuses (stats_status) are flags its effects grant while
 * they last (poisoned): the OR of the statuses of its effects, so a status
 * stays set while any effect grants it, and a cure ends those effects
 * instead of clearing a bit. Rules ask STATUS_HAS(). An entity without
 * stats has none, though effects still act on it (poison damages a chest).
 *
 * A condition is a status that impairs a creature (STATUS_CONDITIONS):
 * poisoned; blinded and unconscious are reserved, their rules to come. It
 * ends when the effects granting it end: their time runs out, a saving
 * throw is passed (1d20 + an ability's modifier against the effect's DC),
 * or a cure.
 *
 * The tables are in slot 0 (the ECS data page, stats_cdata.c): read
 * every turn by the timer handlers and the stats window, and by the rules
 * that use abilities and statuses.
 *
 * Banking: stats_add() and stats_remove() are banked (NGE_BANKED, page 110,
 * with the shared generator helpers): they run when an entity is created
 * or changes, not every turn.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* Abilities: rows of stats_ability */
#define ABIL_STR    0   /**< Strength. */
#define ABIL_DEX    1   /**< Dexterity. */
#define ABIL_CON    2   /**< Constitution. */
#define ABIL_INT    3   /**< Intelligence. */
#define ABIL_WIS    4   /**< Wisdom. */
#define ABIL_CHA    5   /**< Charisma. */
#define ABIL_COUNT  6

/* An ability score's range */
#define STATS_ABILITY_MIN  1
#define STATS_ABILITY_MAX  30

/** An ability score's modifier: (score - 10) / 2 rounded down (an arithmetic shift). */
#define STATS_MOD(score)  ((int8_t)(((int16_t)(score) - 10) >> 1))

/** Kind stats: the stats part of a creature's kind record. */
typedef struct stats_kind {
    uint8_t speed;                  /**< Ticks between turns, whatever the action: 10 normal, 5 fast, 20 slow (1-255). */
    uint8_t ability[ABIL_COUNT];    /**< Scores by ABIL_*, 1-30. */
} stats_kind_t;

/* Statuses: bits of stats_status, granted by effects (game/effects/effects.h) */
#define STATUS_POISONED     0x01
#define STATUS_BLINDED      0x02   /**< Granted, but no rule reads it yet (docs/attributes.md, "Not built yet"). */
#define STATUS_UNCONSCIOUS  0x04   /**< Granted, but no rule reads it yet (docs/attributes.md, "Not built yet"). */

/** The statuses that are conditions (impairments). */
#define STATUS_CONDITIONS  (STATUS_POISONED | STATUS_BLINDED | STATUS_UNCONSCIOUS)

/* Tables in slot 0 (stats_cdata.c): read-only for game code; meaningful only with COMPONENT_STATS */
extern uint8_t stats_speed[NGE_ENTITY_MAX];                 /**< Ticks between turns: its timer's period. Derived (ATTR_SPEED). */
extern uint8_t stats_ability[ABIL_COUNT][NGE_ENTITY_MAX];   /**< Scores by ABIL_*, 1-30. Derived (ATTR_ABILITY()). */
extern uint8_t stats_status[NGE_ENTITY_MAX];                /**< STATUS_* granted by its effects. Derived. */

/* The attributes' bases (attrs.h): read-only outside this component and the effects */
extern uint8_t stats_speed_base[NGE_ENTITY_MAX];
extern uint8_t stats_ability_base[ABIL_COUNT][NGE_ENTITY_MAX];

/** Creature id (with stats) has any of statuses s. */
#define STATUS_HAS(id, s)  ((stats_status[id] & (s)) != 0)

/**
 * @brief Give an entity in use the stats component from kind stats: the
 *        kind values of its speed and abilities, as their attributes'
 *        bases and values; no statuses.
 * @param k In common memory (a copy on the stack): the call maps page 110
 *          over slots 6 and 7.
 */
void stats_add(nge_entity_id_t id, const stats_kind_t *k) NGE_BANKED;

/** @brief Remove the component. */
void stats_remove(nge_entity_id_t id) NGE_BANKED;

#endif
