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

/* A handle of one component: one bit, in a table there is */
static void check_component(nge_component_t c)
{
    uint8_t bit = (uint8_t)c;

    (void)bit;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert((c >> 8) < NGE_COMPONENT_TABLES);
    nge_util_assert(bit != 0 && (bit & (uint8_t)(bit - 1)) == 0);
}

void nge_entity_set_component(nge_entity_id_t id, nge_component_t c)
{
    nge_entity_check_in_use(id);
    check_component(c);

    NGE_ENTITY_ADD_COMPONENT(id, c);
}

void nge_entity_clear_component(nge_entity_id_t id, nge_component_t c)
{
    nge_entity_check_in_use(id);
    check_component(c);

    NGE_ENTITY_REMOVE_COMPONENT(id, c);
}

uint8_t nge_entity_has_component(nge_entity_id_t id, nge_component_t c)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    check_component(c);

    return NGE_ENTITY_HAS_COMPONENT(id, c);
}
