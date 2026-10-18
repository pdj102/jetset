/* nge-dataseg: PAGE_50 */
/*
 * The feature component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by feature_add(). See "ECS data in
 * slot 0" in docs/memory_map.md.
 */
#include "game/components/feature.h"

uint8_t feature_state[NGE_ENTITY_MAX];
