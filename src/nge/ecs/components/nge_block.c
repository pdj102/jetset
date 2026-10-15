/*
 * Block component functions, in common memory: called on every move and
 * every monster step. The tables are in slot 0 (nge_block_cdata.c).
 */
#include "ecs/components/nge_block.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"

void nge_block_set(nge_entity_id_t id, nge_block_t block, nge_block_t pass)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_block_of[id] = block;
    nge_block_pass[id] = pass;
    nge_entity_components[id] |= NGE_COMPONENT_BLOCK;
}

void nge_block_remove(nge_entity_id_t id)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_entity_components[id] &= (nge_component_mask_t)~NGE_COMPONENT_BLOCK;
}

nge_entity_id_t nge_block_find(nge_owner_t area, uint8_t x, uint8_t y, nge_block_t pass,
                               nge_entity_id_t ignore)
{
    nge_entity_id_t id;

    NGE_LOCATION_FOR_EACH_AT(id, area, x, y, NGE_COMPONENT_BLOCK) {
        if (id != ignore && (nge_block_of[id] & (nge_block_t)~pass) != 0) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

uint8_t nge_block_can_enter(const nge_map_t *m, nge_owner_t area, uint8_t x, uint8_t y,
                            nge_block_t pass, nge_entity_id_t ignore, nge_entity_id_t *blocker)
{
    nge_entity_id_t id = NGE_ENTITY_INVALID;
    uint8_t enter = 0;

    /* The map first: no entity search if a wall or the edge blocks */
    if ((nge_map_block(m, x, y) & (nge_block_t)~pass) == 0) {
        id = nge_block_find(area, x, y, pass, ignore);
        enter = id == NGE_ENTITY_INVALID;
    }
    if (blocker != NULL) {
        *blocker = id;
    }
    return enter;
}
