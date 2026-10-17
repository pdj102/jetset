/* nge-dataseg: PAGE_50 */
/*
 * The attributes' base values and the status masks in slot 0 (the ECS data
 * page, see ecs/nge_ecs.h). Uninitialised: set by health_add(),
 * stats_add() and the effects. See "ECS data in slot 0" in
 * docs/memory_map.md.
 */
#include "game/components/attrs.h"

uint8_t attr_base[ATTR_COUNT][NGE_ENTITY_MAX];
uint8_t status_mask[NGE_ENTITY_MAX];
