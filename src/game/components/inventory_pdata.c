/* nge-dataseg: PAGE_111 */
/*
 * The inventory component's tables, page data in page 111 (slot 7 while
 * the component functions in page 110 run), moved out of slot 0 as they
 * are read on commands and AI turns, not every frame (docs/slot0_tables.md,
 * D6). Other pages read them with inv_capacity_of() and inv_slots_of().
 * Uninitialised: set by inv_add(). See "Page data" in docs/memory_map.md.
 */
#include "game/components/inventory_internal.h"

uint8_t  inv_capacity[NGE_ENTITY_MAX];
uint16_t inv_slots[NGE_ENTITY_MAX];
