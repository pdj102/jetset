#ifndef NGE_ENTITY_INTERNAL_H
#define NGE_ENTITY_INTERNAL_H

/*
 * Shared by the entity module's files; not for use outside it:
 * - nge_entity.c (common memory): the checked flag and component functions;
 * - nge_entity_cdata.c (slot 0, the ECS data page): the public tables, used
 *   every frame;
 * - nge_entity_life.c (page 48): the lifecycle functions;
 * - nge_entity_pdata.c (page 49): the lifecycle bookkeeping below.
 */

#include "ecs/nge_entity.h"

/*
 * Page data (page 49): used only by the lifecycle functions in page 48,
 * which have page 49 mapped in slot 7 while they run.
 */
extern uint8_t         nge_entity_alive_pos[NGE_ENTITY_MAX];  /* index of each alive ID in nge_entity_alive */
extern nge_entity_id_t nge_entity_free_stack[NGE_ENTITY_MAX];
extern uint8_t         nge_entity_free_top;
extern nge_entity_id_t nge_entity_dying_stack[NGE_ENTITY_MAX];
extern uint8_t         nge_entity_dying_top;

/* In common memory: assert that id is alive or dying (may have flags and components) */
void nge_entity_check_in_use(nge_entity_id_t id);

#endif
