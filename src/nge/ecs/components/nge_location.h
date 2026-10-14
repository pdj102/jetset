#ifndef NGE_LOCATION_H
#define NGE_LOCATION_H

/**
 * @file nge_location.h
 * @brief Location component: an entity's position in its area's map.
 *
 * x is the map column and y the map row, in tiles (maps are at most 80 x 48,
 * see map/nge_map.h); fx and fy are the pixel within the tile, 0-7, for
 * smooth movement. The position in map pixels is x * 8 + fx
 * (NGE_LOCATION_PX) and y * 8 + fy (NGE_LOCATION_PY).
 *
 * An entity placed in an area has the area as its owner (ecs/nge_owner.h)
 * and this component; an entity in an inventory or in the world has no
 * location. Ownership and location are independent: when an entity is
 * picked up, the game removes its location.
 *
 * The tables are in slot 0, in the ECS data page (ecs/nge_ecs.h), as they
 * are read every frame by several systems (movement, collision, drawing).
 * Systems may read and write them directly through the macros, for
 * entities that have NGE_COMPONENT_LOCATION, keeping fx and fy in 0-7. A
 * new entity has no location.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/** Component bit: component number 0, reserved for nge (see nge_entity.h). */
#define NGE_COMPONENT_LOCATION  NGE_COMPONENT(0)

/** Pixels per tile: the fine position is 0 to NGE_LOCATION_FINE-1. */
#define NGE_LOCATION_FINE  8

/* Tables in slot 0 (nge_location_cdata.c): meaningful only for entities with NGE_COMPONENT_LOCATION */
extern uint8_t nge_location_x[NGE_ENTITY_MAX];    /**< Map column. */
extern uint8_t nge_location_y[NGE_ENTITY_MAX];    /**< Map row. */
extern uint8_t nge_location_fx[NGE_ENTITY_MAX];   /**< Pixel within the tile, 0-7, rightwards. */
extern uint8_t nge_location_fy[NGE_ENTITY_MAX];   /**< Pixel within the tile, 0-7, downwards. */

/* ---- Unchecked macros: lvalues, except PX and PY ---- */

#define NGE_LOCATION_X(id)   (nge_location_x[id])
#define NGE_LOCATION_Y(id)   (nge_location_y[id])
#define NGE_LOCATION_FX(id)  (nge_location_fx[id])
#define NGE_LOCATION_FY(id)  (nge_location_fy[id])

/** X in map pixels. */
#define NGE_LOCATION_PX(id)  ((uint16_t)nge_location_x[id] * NGE_LOCATION_FINE + nge_location_fx[id])
/** Y in map pixels. */
#define NGE_LOCATION_PY(id)  ((uint16_t)nge_location_y[id] * NGE_LOCATION_FINE + nge_location_fy[id])

/* ---- Checked functions ---- */

/**
 * @brief Place an entity in use (alive or dying) on a tile, adding the
 *        component. The fine position is 0: the tile's top-left pixel.
 */
void nge_location_set(nge_entity_id_t id, uint8_t x, uint8_t y);

/** @brief Set the pixel within the tile (0-7 each) of an entity with a location. */
void nge_location_set_fine(nge_entity_id_t id, uint8_t fx, uint8_t fy);

/**
 * @brief Move an entity with a location by (dx, dy) pixels, carrying into
 *        the tile position. The game keeps it inside the map.
 */
void nge_location_move(nge_entity_id_t id, int8_t dx, int8_t dy);

/** @brief Remove an entity's location (the component). */
void nge_location_remove(nge_entity_id_t id);

#endif
