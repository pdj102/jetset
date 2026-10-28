#ifndef AI_H
#define AI_H

/**
 * @file ai.h
 * @brief The AI component: the game decides the entity's turns (every
 *        monster). See docs/ranged_ai_components.md.
 *
 * A marker for now, with no tables: what an AI needs (a behaviour by
 * number, a target, a state) comes with the AI module, and its data goes
 * where that module reads it. The creature generator gives it to every
 * creature but the player (game/gen). A monster's turn still waits
 * (TIMER_MONSTER, game/timers.h).
 *
 * Banking: the functions are banked (NGE_BANKED, page 110, with the other
 * component functions).
 */

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/** Entity id has the AI component. */
#define AI_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_AI)

/** @brief Give an entity in use the AI component: the game decides its turns. */
void ai_add(nge_entity_id_t id) NGE_BANKED;

/** @brief Remove the AI component. */
void ai_remove(nge_entity_id_t id) NGE_BANKED;

#endif
