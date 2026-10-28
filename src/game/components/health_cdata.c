/* nge-dataseg: PAGE_50 */
/*
 * The health component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by health_add(). See "ECS data in
 * slot 0" in docs/memory_map.md.
 */
#include "game/components/health.h"

uint8_t health_hp[NGE_ENTITY_MAX];
uint8_t health_hp_max[NGE_ENTITY_MAX];
uint8_t health_immune[NGE_ENTITY_MAX];
uint8_t health_resist[NGE_ENTITY_MAX];
uint8_t health_vuln[NGE_ENTITY_MAX];
uint8_t health_ac[NGE_ENTITY_MAX];
uint8_t health_hp_max_base[NGE_ENTITY_MAX];
uint8_t health_immune_base[NGE_ENTITY_MAX];
uint8_t health_resist_base[NGE_ENTITY_MAX];
uint8_t health_vuln_base[NGE_ENTITY_MAX];
uint8_t health_ac_base[NGE_ENTITY_MAX];
