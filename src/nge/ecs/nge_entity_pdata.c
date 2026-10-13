/* nge-dataseg: PAGE_49 */
/*
 * Entity page data: the lifecycle bookkeeping, in page 49 (slot 7 while the
 * lifecycle functions in page 48 run). Uninitialised: nge_entity_init()
 * sets it. See "Page data" in docs/memory_map.md.
 */
#include "ecs/nge_entity_internal.h"

uint8_t         nge_entity_alive_pos[NGE_ENTITY_MAX];
nge_entity_id_t nge_entity_free_stack[NGE_ENTITY_MAX];
uint8_t         nge_entity_free_top;
nge_entity_id_t nge_entity_dying_stack[NGE_ENTITY_MAX];
uint8_t         nge_entity_dying_top;
