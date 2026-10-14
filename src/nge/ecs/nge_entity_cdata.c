/* nge-dataseg: PAGE_50 */
/*
 * Entity tables in slot 0 (the ECS data page, see ecs/nge_ecs.h): read every
 * frame through the macros in nge_entity.h. Uninitialised: nge_entity_init()
 * sets them. See "ECS data in slot 0" in docs/memory_map.md.
 */
#include "ecs/nge_entity.h"

nge_entity_flags_t   nge_entity_flags[NGE_ENTITY_MAX];
nge_component_mask_t nge_entity_components[NGE_ENTITY_MAX];
nge_entity_id_t      nge_entity_alive[NGE_ENTITY_MAX];
uint8_t              nge_entity_alive_count;
