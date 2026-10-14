/* nge-dataseg: PAGE_50 */
/*
 * Location tables in slot 0 (the ECS data page, see ecs/nge_ecs.h): read
 * every frame by several systems through NGE_LOCATION_X and NGE_LOCATION_Y.
 * Uninitialised: set by nge_location_set(). See "ECS data in slot 0" in
 * docs/memory_map.md.
 */
#include "ecs/nge_location.h"

uint8_t nge_location_x[NGE_ENTITY_MAX];
uint8_t nge_location_y[NGE_ENTITY_MAX];
