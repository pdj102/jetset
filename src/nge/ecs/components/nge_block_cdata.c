/* nge-dataseg: PAGE_50 */
/*
 * Block tables in slot 0 (the ECS data page, see ecs/nge_ecs.h): read every
 * turn by movement, monster AI and targeting. Uninitialised: set by
 * nge_block_set(). See "ECS data in slot 0" in docs/memory_map.md.
 */
#include "ecs/components/nge_block.h"

nge_block_t nge_block_of[NGE_ENTITY_MAX];
nge_block_t nge_block_pass[NGE_ENTITY_MAX];
