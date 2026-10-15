#ifndef ITEMS_H
#define ITEMS_H

/**
 * @file items.h
 * @brief The item component: what kind of item an entity is.
 *
 * Placeholder kinds until there are item generators. The table is in slot 0
 * (the ECS data page, items_cdata.c), so common code (messages) reads it
 * directly and it is saved with the other ECS data.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/** Component bit: the game's first component (see nge_entity.h). */
#define COMPONENT_ITEM  NGE_COMPONENT(NGE_COMPONENT_USER + 0)

/* Item kinds */
#define ITEM_POTION  0
#define ITEM_SWORD   1
#define ITEM_KINDS   2   /**< Number of kinds. */

/** Kind of each entity with COMPONENT_ITEM, in slot 0: set when the item is created. */
extern uint8_t item_kind[NGE_ENTITY_MAX];

#endif
