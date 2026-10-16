/* nge-dataseg: PAGE_50 */
/*
 * The inventory component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by inv_add(). See "ECS data in slot
 * 0" in docs/memory_map.md.
 */
#include "game/components/inventory.h"

uint8_t  inv_capacity[NGE_ENTITY_MAX];
uint16_t inv_slots[NGE_ENTITY_MAX];
