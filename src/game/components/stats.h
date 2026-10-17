#ifndef STATS_H
#define STATS_H

/**
 * @file stats.h
 * @brief The stats component: a creature's abilities (its speed so far;
 *        later strength and the like). Hit points are the health component
 *        (game/components/health.h).
 *
 * Added from base data (stats_base_t, part of a creature's base record in
 * game/gen/) by stats_add(). The tables are in slot 0 (the ECS data page,
 * stats_cdata.c): read every turn by the timer handlers and, later, combat.
 *
 * Banking: stats_add() and stats_remove() are banked (NGE_BANKED, page 110,
 * with the shared generator helpers): they run when an entity is created
 * or changes, not every turn.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

/** Component bit: the game's second component (see nge_entity.h). */
#define COMPONENT_STATS  NGE_COMPONENT(NGE_COMPONENT_USER + 1)

/** Base stats: the stats part of a creature's base record. */
typedef struct stats_base {
    uint8_t speed;      /**< Ticks between turns, whatever the action: 10 normal, 5 fast, 20 slow (1-255). */
} stats_base_t;

/* Tables in slot 0 (stats_cdata.c): meaningful only with COMPONENT_STATS */
extern uint8_t stats_speed[NGE_ENTITY_MAX];    /**< Ticks between turns: its timer's period. */

/**
 * @brief Give an entity in use the stats component from base stats.
 * @param b In common memory (a copy on the stack): the call maps page 110
 *          over slots 6 and 7.
 */
void stats_add(nge_entity_id_t id, const stats_base_t *b) NGE_BANKED;

/** @brief Remove the component. */
void stats_remove(nge_entity_id_t id) NGE_BANKED;

#endif
