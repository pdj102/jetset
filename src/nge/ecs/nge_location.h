#ifndef NGE_LOCATION_H
#define NGE_LOCATION_H

/**
 * @file nge_location.h
 * @brief Location component: an entity's tile position in its area's map.
 *
 * x is the map column and y the map row, in tiles (maps are at most 80 x 48,
 * see map/nge_map.h). An entity placed in an area has the area as its owner
 * (ecs/nge_owner.h) and this component; an entity in an inventory or in the
 * world has no location. Ownership and location are independent: when an
 * entity is picked up, the game removes its location.
 *
 * The tables are in slot 0, in the ECS data page (ecs/nge_ecs.h), as they
 * are read every frame by several systems (movement, collision, drawing).
 * Systems may read and write them directly through the macros, for
 * entities that have NGE_COMPONENT_LOCATION. A new entity has no location.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/** Component bit: component number 0, reserved for nge (see nge_entity.h). */
#define NGE_COMPONENT_LOCATION  NGE_COMPONENT(0)

/* Tables in slot 0 (nge_location_cdata.c): meaningful only for entities with NGE_COMPONENT_LOCATION */
extern uint8_t nge_location_x[NGE_ENTITY_MAX];   /**< Map column. */
extern uint8_t nge_location_y[NGE_ENTITY_MAX];   /**< Map row. */

/* ---- Unchecked macros: lvalues ---- */

#define NGE_LOCATION_X(id)  (nge_location_x[id])
#define NGE_LOCATION_Y(id)  (nge_location_y[id])

/* ---- Checked functions ---- */

/** @brief Give an entity in use (alive or dying) a location, adding the component. */
void nge_location_set(nge_entity_id_t id, uint8_t x, uint8_t y);

/** @brief Remove an entity's location (the component). */
void nge_location_remove(nge_entity_id_t id);

#endif
