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
 * damage (game/components/attack.h); later, level and experience (the
 * player) and magic points (magic). Stats fixed by kind stay in the
 * kind's base record (game/gen).
 *
 * An ability score is 1-30, 10-11 average; its modifier, STATS_MOD(), is
 * (score - 10) / 2 rounded down: -5 at 1, +10 at 30. Speed and the
 * abilities are attributes (game/components/attrs.h): stats_add() sets
 * their bases and values from base data (stats_base_t, part of a
 * creature's base record in game/gen/), and effects change them
 * (game/effects/effects.h). The tables are in slot 0 (the ECS data page,
 * stats_cdata.c): read every turn by the timer handlers and, later, by
 * the rules that use abilities.
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

/** Base stats: the stats part of a creature's base record. */
typedef struct stats_base {
    uint8_t speed;                  /**< Ticks between turns, whatever the action: 10 normal, 5 fast, 20 slow (1-255). */
    uint8_t ability[ABIL_COUNT];    /**< Scores by ABIL_*, 1-30. */
} stats_base_t;

/* Tables in slot 0 (stats_cdata.c): meaningful only with COMPONENT_STATS; derived (attrs.h) */
extern uint8_t stats_speed[NGE_ENTITY_MAX];                 /**< Ticks between turns: its timer's period (ATTR_SPEED). */
extern uint8_t stats_ability[ABIL_COUNT][NGE_ENTITY_MAX];   /**< Scores by ABIL_*, 1-30 (ATTR_ABILITY()). */

/**
 * @brief Give an entity in use the stats component from base stats: its
 *        speed and abilities, which are also their attributes' bases.
 * @param b In common memory (a copy on the stack): the call maps page 110
 *          over slots 6 and 7.
 */
void stats_add(nge_entity_id_t id, const stats_base_t *b) NGE_BANKED;

/** @brief Remove the component. */
void stats_remove(nge_entity_id_t id) NGE_BANKED;

#endif
