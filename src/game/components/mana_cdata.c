/* nge-dataseg: PAGE_50 */
/*
 * The mana component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by mana_add(). See "ECS data in
 * slot 0" in docs/memory_map.md.
 */
#include "game/components/mana.h"

uint8_t mana_mp[NGE_ENTITY_MAX];
uint8_t mana_mp_max[NGE_ENTITY_MAX];
