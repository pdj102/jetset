/* nge-dataseg: PAGE_50 */
/*
 * Location tables in slot 0 (the ECS data page, see ecs/nge_ecs.h): read
 * every frame by several systems through the NGE_LOCATION_* macros.
 * Uninitialised: set by nge_location_set(). See "ECS data in slot 0" in
 * docs/memory_map.md.
 */
#include "ecs/components/nge_location.h"

uint8_t nge_location_x[NGE_ENTITY_MAX];
uint8_t nge_location_y[NGE_ENTITY_MAX];
uint8_t nge_location_fx[NGE_ENTITY_MAX];
uint8_t nge_location_fy[NGE_ENTITY_MAX];
