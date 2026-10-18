#ifndef NGE_COMPONENTS_H
#define NGE_COMPONENTS_H

/**
 * @file nge_components.h
 * @brief Components: what they are, how their data is kept, and the
 *        component numbers, nge's and the range left for the game.
 *
 * A component is a kind of data an entity can have (a location, a
 * sprite, hit points). It has:
 *
 * - a number, 0 to NGE_COMPONENT_MAX-1, and its mask bit
 *   NGE_COMPONENT(n) (ecs/nge_entity.h). An entity's mask
 *   (nge_entity_components[]) says which components it has. Numbers 0 to
 *   NGE_COMPONENT_USER-1 are nge's, listed here; the game's start at
 *   NGE_COMPONENT_USER and are listed in one place of its own;
 * - its data: one table per field, indexed by entity ID
 *   (nge_location_x[], nge_location_y[]), not an array of structs, so each
 *   access is one byte-table read on the Z80. A table is meaningful only
 *   for entities with the component's bit;
 * - a place for that data, by who reads it (see "Where component data
 *   goes" in docs/memory_map.md): data read every frame by several systems,
 *   or by common code, in slot 0 (the module's *_cdata.c, page 50); data
 *   used by one system in that system's page data (its *_pdata.c);
 * - unchecked macros for per-frame code that reads the tables directly
 *   (NGE_LOCATION_X(id)), and checked functions for everything else,
 *   which assert on the entity's state and the component's bit;
 * - functions that add and remove it, keeping the bit and the tables in
 *   step. The module's init resets its tables (see "Module
 *   initialisation" in src/nge/README.md).
 *
 * A component shared by several systems is in ecs/components/; one
 * private to a system is part of that system, in ecs/systems/. Its number
 * and bit are here either way, so the next free one is never in doubt.
 */

#include "ecs/nge_entity.h"

/** Component numbers are below this: one bit each in nge_component_mask_t. */
#define NGE_COMPONENT_MAX   16
/** The game's first component number: those below are nge's. */
#define NGE_COMPONENT_USER  4

/* nge's components: numbers 0 to NGE_COMPONENT_USER-1 */
#define NGE_COMPONENT_LOCATION  NGE_COMPONENT(0)   /**< Position in an area's map (ecs/components/nge_location.h). */
#define NGE_COMPONENT_SPRITE    NGE_COMPONENT(1)   /**< Drawn as hardware sprites (ecs/systems/nge_render.h). */
#define NGE_COMPONENT_BLOCK     NGE_COMPONENT(2)   /**< What it blocks and can enter (ecs/components/nge_block.h). */
#define NGE_COMPONENT_TIMER     NGE_COMPONENT(3)   /**< Fires every period (ecs/systems/nge_timer.h). */

/* Every number has a bit in the mask type */
typedef char nge_component_max_check[(sizeof(nge_component_mask_t) * 8 == NGE_COMPONENT_MAX) ? 1 : -1];

#endif
