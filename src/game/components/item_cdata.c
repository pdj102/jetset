/* nge-dataseg: PAGE_50 */
/*
 * The item component's table in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by item_add(). See "ECS data in slot
 * 0" in docs/memory_map.md.
 */
#include "game/components/item.h"

uint8_t item_slot[NGE_ENTITY_MAX];
