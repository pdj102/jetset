/*
 * Location component functions, in common memory. The tables are in slot 0
 * (nge_location_cdata.c).
 */
#include "ecs/components/nge_location.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"

/* An entity in use with a location */
static void check_located(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(nge_entity_components[id] & NGE_COMPONENT_LOCATION);
}

void nge_location_set(nge_entity_id_t id, uint8_t x, uint8_t y)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_location_x[id] = x;
    nge_location_y[id] = y;
    nge_location_fx[id] = 0;
    nge_location_fy[id] = 0;
    nge_entity_components[id] |= NGE_COMPONENT_LOCATION;
}

void nge_location_set_fine(nge_entity_id_t id, uint8_t fx, uint8_t fy)
{
    check_located(id);
    nge_util_assert(fx < NGE_LOCATION_FINE && fy < NGE_LOCATION_FINE);

    nge_location_fx[id] = fx;
    nge_location_fy[id] = fy;
}

void nge_location_move(nge_entity_id_t id, int8_t dx, int8_t dy)
{
    uint16_t px;
    uint16_t py;

    check_located(id);

    px = NGE_LOCATION_PX(id) + dx;
    py = NGE_LOCATION_PY(id) + dy;
    nge_location_x[id] = (uint8_t)(px / NGE_LOCATION_FINE);
    nge_location_fx[id] = (uint8_t)(px % NGE_LOCATION_FINE);
    nge_location_y[id] = (uint8_t)(py / NGE_LOCATION_FINE);
    nge_location_fy[id] = (uint8_t)(py % NGE_LOCATION_FINE);
}

void nge_location_remove(nge_entity_id_t id)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    nge_entity_components[id] &= (nge_component_mask_t)~NGE_COMPONENT_LOCATION;
}

nge_entity_id_t nge_location_find(nge_owner_t area, uint8_t x, uint8_t y,
                                  nge_component_mask_t mask, nge_entity_id_t from)
{
    const uint8_t *p;
    nge_entity_id_t id;

    mask |= NGE_COMPONENT_LOCATION;
    while (from < NGE_ENTITY_MAX) {
        /* Next entity in column x: the only search over every entity */
        p = memchr(&nge_location_x[from], x, NGE_ENTITY_MAX - from);
        if (p == NULL) {
            break;
        }
        id = (nge_entity_id_t)(p - nge_location_x);
        /* Free entities keep old locations: check the rest only for this one */
        if (nge_location_y[id] == y && NGE_ENTITY_IS_ALIVE(id) &&
            NGE_ENTITY_HAS_COMPONENTS(id, mask) && nge_owner_of[id] == area) {
            return id;
        }
        from = id + 1;
    }
    return NGE_ENTITY_INVALID;
}
