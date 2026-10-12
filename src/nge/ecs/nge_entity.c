#include "ecs/nge_entity.h"

#include "core/nge_util.h"

/* IDs are uint8_t with 0xFF reserved for NGE_ENTITY_INVALID */
typedef char nge_entity_max_check[(NGE_ENTITY_MAX >= 1 && NGE_ENTITY_MAX <= 250) ? 1 : -1];

nge_entity_flags_t   nge_entity_flags[NGE_ENTITY_MAX];
nge_component_mask_t nge_entity_components[NGE_ENTITY_MAX];
nge_entity_id_t      nge_entity_alive[NGE_ENTITY_MAX];
uint8_t              nge_entity_alive_count;

/* Position of each alive entity in nge_entity_alive */
static uint8_t alive_pos[NGE_ENTITY_MAX];

static nge_entity_id_t free_stack[NGE_ENTITY_MAX];
static uint8_t free_top;

static nge_entity_id_t dying_stack[NGE_ENTITY_MAX];
static uint8_t dying_top;

/* An entity that may have flags and components: alive or dying */
static void check_in_use(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

void nge_entity_init(void)
{
    uint8_t i;

    /* Top of the free stack is ID 0, so IDs are handed out 0, 1, 2... */
    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        nge_entity_flags[i] = NGE_ENTITY_FREE;
        nge_entity_components[i] = 0;
        free_stack[i] = NGE_ENTITY_MAX - 1 - i;
    }
    free_top = NGE_ENTITY_MAX;
    nge_entity_alive_count = 0;
    dying_top = 0;
}

nge_entity_id_t nge_entity_create(void)
{
    nge_entity_id_t id;

    if (free_top == 0) {
        return NGE_ENTITY_INVALID;
    }
    id = free_stack[--free_top];

    nge_entity_flags[id] = NGE_ENTITY_ALIVE;
    nge_entity_components[id] = 0;
    alive_pos[id] = nge_entity_alive_count;
    nge_entity_alive[nge_entity_alive_count++] = id;
    return id;
}

uint8_t nge_entity_destroy(nge_entity_id_t id)
{
    uint8_t pos;
    nge_entity_id_t last;

    check_in_use(id);

    if (NGE_ENTITY_STATE(id) != NGE_ENTITY_ALIVE ||
        (nge_entity_flags[id] & NGE_ENTITY_PERSISTENT)) {
        return 0;
    }

    /* Remove from the alive list: the last entry moves into the gap */
    pos = alive_pos[id];
    last = nge_entity_alive[--nge_entity_alive_count];
    nge_entity_alive[pos] = last;
    alive_pos[last] = pos;

    nge_entity_flags[id] = (nge_entity_flags[id] & NGE_ENTITY_FLAG_MASK) | NGE_ENTITY_DYING;
    dying_stack[dying_top++] = id;
    return 1;
}

nge_entity_id_t nge_entity_next_dying(void)
{
    if (dying_top == 0) {
        return NGE_ENTITY_INVALID;
    }
    return dying_stack[--dying_top];
}

void nge_entity_release(nge_entity_id_t id)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) == NGE_ENTITY_DYING);

    nge_entity_flags[id] = NGE_ENTITY_FREE;
    nge_entity_components[id] = 0;
    free_stack[free_top++] = id;
}

uint8_t nge_entity_free_count(void)
{
    return free_top;
}

uint8_t nge_entity_dying_count(void)
{
    return dying_top;
}

void nge_entity_set_flags(nge_entity_id_t id, nge_entity_flags_t f)
{
    check_in_use(id);
    nge_util_assert((f & NGE_ENTITY_STATE_MASK) == 0);

    nge_entity_flags[id] |= f;
}

void nge_entity_clear_flags(nge_entity_id_t id, nge_entity_flags_t f)
{
    check_in_use(id);
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
    check_in_use(id);

    nge_entity_components[id] |= m;
}

void nge_entity_clear_components(nge_entity_id_t id, nge_component_mask_t m)
{
    check_in_use(id);

    nge_entity_components[id] &= (nge_component_mask_t)~m;
}

uint8_t nge_entity_has_components(nge_entity_id_t id, nge_component_mask_t m)
{
    nge_util_assert(id < NGE_ENTITY_MAX);

    return NGE_ENTITY_HAS_COMPONENTS(id, m);
}
