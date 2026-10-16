#ifndef INVENTORY_H
#define INVENTORY_H

/**
 * @file inventory.h
 * @brief The inventory component: an entity that can hold items (the
 *        player, some creatures, later chests), and moving items between
 *        the floor and holders.
 *
 * There are no lists of items: an item records where it is (game/
 * components/item.h). A holder's inventory is the items it owns that
 * aren't equipped, in no particular order (ID order when walked with
 * inv_next()); its equipment is the items it owns in a slot. A holder can
 * stow up to inv_capacity items; equipped items don't count. inv_slots is
 * the mask of equipment slots it has (SLOT_BIT()).
 *
 * inv_stow() and inv_place() move an item without checking the rules: the
 * actions (game/actions/act.h) check them first.
 *
 * The tables are in slot 0 (inventory_cdata.c). Banking: every function is
 * banked (NGE_BANKED, page 110, with the shared generator helpers): they
 * run on commands and actions, not every frame.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

/** Component bit: the game's fourth component (see nge_entity.h). */
#define COMPONENT_INVENTORY  NGE_COMPONENT(NGE_COMPONENT_USER + 3)

/* Tables in slot 0 (inventory_cdata.c): meaningful only with COMPONENT_INVENTORY */
extern uint8_t  inv_capacity[NGE_ENTITY_MAX];   /**< Items it can stow, 1 or more. */
extern uint16_t inv_slots[NGE_ENTITY_MAX];      /**< The equipment slots it has (SLOT_BIT() mask). */

/** The entity has the inventory component. */
#define INV_HAS(id)  NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_INVENTORY)

/**
 * @brief Give an entity in use the inventory component (or replace it).
 * @param capacity Items it can stow, 1 or more.
 * @param slots The equipment slots it has, a mask of SLOT_BIT(); 0 for none.
 */
void inv_add(nge_entity_id_t id, uint8_t capacity, uint16_t slots) NGE_BANKED;

/** @brief The number of items holder has stowed (alive, owned by it, not equipped). */
uint8_t inv_count(nge_entity_id_t holder) NGE_BANKED;

/**
 * @brief The next item holder has stowed, in ID order, after `after` (use
 *        NGE_ENTITY_INVALID for the first).
 * @return The item, or NGE_ENTITY_INVALID if there are no more.
 */
nge_entity_id_t inv_next(nge_entity_id_t holder, nge_entity_id_t after) NGE_BANKED;

/** @brief The item holder has equipped in slot (SLOT_WEAPON...), or NGE_ENTITY_INVALID. */
nge_entity_id_t inv_equipped(nge_entity_id_t holder, uint8_t slot) NGE_BANKED;

/** @brief Stow item in holder's inventory: owned by holder, off the map, not equipped. */
void inv_stow(nge_entity_id_t item, nge_entity_id_t holder) NGE_BANKED;

/** @brief Put item on the floor: owned by area at tile (x, y), not equipped. */
void inv_place(nge_entity_id_t item, nge_owner_t area, uint8_t x, uint8_t y) NGE_BANKED;

/**
 * @brief Put every item holder owns (stowed or equipped) on the floor of
 *        its tile, in its area: when it dies. Does nothing if holder isn't
 *        on the map (its items stay with it).
 * @return The number of items placed.
 */
uint8_t inv_drop_all(nge_entity_id_t holder) NGE_BANKED;

#endif
