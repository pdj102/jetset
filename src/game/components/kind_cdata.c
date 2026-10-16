/* nge-dataseg: PAGE_50 */
/*
 * The kind component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by kind_add(). See "ECS data in slot
 * 0" in docs/memory_map.md.
 */
#include "game/components/kind.h"

uint8_t kind_class[NGE_ENTITY_MAX];
uint8_t kind_index[NGE_ENTITY_MAX];
