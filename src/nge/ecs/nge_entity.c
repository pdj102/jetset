/*
 * Entity functions in common memory: the checked flag and component
 * functions. The tables are in slot 0 (nge_entity_cdata.c); the lifecycle
 * functions are banked, in nge_entity_life.c.
 */
#include "ecs/nge_entity_internal.h"

#include "core/nge_util.h"

/* IDs are uint8_t with 0xFF reserved for NGE_ENTITY_INVALID */
typedef char nge_entity_max_check[(NGE_ENTITY_MAX >= 1 && NGE_ENTITY_MAX <= 250) ? 1 : -1];

/* An entity that may have flags and components: alive or dying */
void nge_entity_check_in_use(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

void nge_entity_set_flags(nge_entity_id_t id, nge_entity_flags_t f)
{
    nge_entity_check_in_use(id);
    nge_util_assert((f & NGE_ENTITY_STATE_MASK) == 0);

    nge_entity_flags[id] |= f;
}

void nge_entity_clear_flags(nge_entity_id_t id, nge_entity_flags_t f)
{
    nge_entity_check_in_use(id);
    nge_util_assert((f & NGE_ENTITY_STATE_MASK) == 0);

    nge_entity_flags[id] &= (nge_entity_flags_t)~f;
}

uint8_t nge_entity_has_flags(nge_entity_id_t id, nge_entity_flags_t f)
{
    nge_util_assert(id < NGE_ENTITY_MAX);

    return NGE_ENTITY_HAS_FLAGS(id, f);
}

void nge_entity_set_components(nge_entity_id_t id, nge_component_mask_t m)
{
    nge_entity_check_in_use(id);

    nge_entity_components[id] |= m;
}

void nge_entity_clear_components(nge_entity_id_t id, nge_component_mask_t m)
{
    nge_entity_check_in_use(id);

    nge_entity_components[id] &= (nge_component_mask_t)~m;
}

uint8_t nge_entity_has_components(nge_entity_id_t id, nge_component_mask_t m)
{
    nge_util_assert(id < NGE_ENTITY_MAX);

    return NGE_ENTITY_HAS_COMPONENTS(id, m);
}
