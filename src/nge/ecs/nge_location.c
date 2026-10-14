/*
 * Location component functions, in common memory. The tables are in slot 0
 * (nge_location_cdata.c).
 */
#include "ecs/nge_location.h"

#include "core/nge_util.h"

void nge_location_set(nge_entity_id_t id, uint8_t x, uint8_t y)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_location_x[id] = x;
    nge_location_y[id] = y;
    nge_entity_components[id] |= NGE_COMPONENT_LOCATION;
}

void nge_location_remove(nge_entity_id_t id)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_entity_components[id] &= (nge_component_mask_t)~NGE_COMPONENT_LOCATION;
}
