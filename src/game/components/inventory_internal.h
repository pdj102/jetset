#ifndef INVENTORY_INTERNAL_H
#define INVENTORY_INTERNAL_H

/*
 * The inventory component's tables, in page 111 (inventory_pdata.c):
 * readable only by the component's functions in page 110 (inventory.c),
 * and the tests. Other code calls inv_capacity_of() and inv_slots_of().
 * Meaningful only with COMPONENT_INVENTORY.
 */

#include "game/components/inventory.h"

extern uint8_t  inv_capacity[NGE_ENTITY_MAX];   /* items it can stow, 1 or more */
extern uint16_t inv_slots[NGE_ENTITY_MAX];      /* the equipment slots it has (SLOT_BIT() mask) */

#endif
