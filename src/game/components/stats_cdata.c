/* nge-dataseg: PAGE_50 */
/*
 * The stats component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by stats_add(). See "ECS data in slot
 * 0" in docs/memory_map.md.
 */
#include "game/components/stats.h"

uint8_t stats_hp[NGE_ENTITY_MAX];
uint8_t stats_hp_max[NGE_ENTITY_MAX];
uint8_t stats_speed[NGE_ENTITY_MAX];
