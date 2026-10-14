/* nge-dataseg: PAGE_50 */
/*
 * The owner table in slot 0 (the ECS data page, see ecs/nge_ecs.h): read
 * every frame through NGE_OWNER_OF and NGE_OWNER_FOR_EACH_OWNED.
 * Uninitialised: nge_entity_init() and nge_entity_create() set it. See
 * "ECS data in slot 0" in docs/memory_map.md.
 */
#include "ecs/nge_owner.h"

nge_owner_t nge_owner_of[NGE_ENTITY_MAX];
