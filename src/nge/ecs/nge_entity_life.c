/*
 * Entity lifecycle functions: banked in page 48, with their bookkeeping in
 * page 49 (nge_entity_pdata.c), which banked_call maps into slot 7 for each
 * call. The tables used every frame are in slot 0 (nge_entity_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_48
#pragma constseg PAGE_48
#endif

#include "ecs/nge_entity_internal.h"
#include "ecs/nge_owner.h"

#include "core/nge_util.h"

/* No components: entity id's byte in every table */
static void clear_components(nge_entity_id_t id)
{
    uint8_t t;

    for (t = 0; t < NGE_COMPONENT_TABLES; t++) {
        nge_entity_components[t][id] = 0;
    }
}

void nge_entity_init(void) NGE_BANKED
{
    uint8_t i;

    /* Top of the free stack is ID 0, so IDs are handed out 0, 1, 2... */
    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        nge_entity_flags[i] = NGE_ENTITY_FREE;
        clear_components(i);
        nge_owner_of[i] = NGE_OWNER_WORLD;
        nge_entity_free_stack[i] = NGE_ENTITY_MAX - 1 - i;
    }
    nge_entity_free_top = NGE_ENTITY_MAX;
    nge_entity_used_count = 0;
    nge_entity_dying_top = 0;
}

nge_entity_id_t nge_entity_create(void) NGE_BANKED
{
    nge_entity_id_t id;

    if (nge_entity_free_top == 0) {
        return NGE_ENTITY_INVALID;
    }
    id = nge_entity_free_stack[--nge_entity_free_top];

    nge_entity_flags[id] = NGE_ENTITY_ALIVE;
    clear_components(id);
    nge_owner_of[id] = NGE_OWNER_WORLD;
    nge_entity_used_pos[id] = nge_entity_used_count;
    nge_entity_used[nge_entity_used_count++] = id;
    return id;
}

uint8_t nge_entity_destroy(nge_entity_id_t id) NGE_BANKED
{
    nge_entity_check_in_use(id);

    if (NGE_ENTITY_STATE(id) != NGE_ENTITY_ALIVE ||
        (nge_entity_flags[id] & NGE_ENTITY_PERSISTENT)) {
        return 0;
    }

    /* It stays in the used list until it is released, so loops can destroy anything */
    nge_entity_flags[id] = (nge_entity_flags[id] & NGE_ENTITY_FLAG_MASK) | NGE_ENTITY_DYING;
    nge_entity_dying_stack[nge_entity_dying_top++] = id;
    return 1;
}

nge_entity_id_t nge_entity_next_dying(void) NGE_BANKED
{
    if (nge_entity_dying_top == 0) {
        return NGE_ENTITY_INVALID;
    }
    return nge_entity_dying_stack[--nge_entity_dying_top];
}

void nge_entity_release(nge_entity_id_t id) NGE_BANKED
{
    uint8_t pos;
    nge_entity_id_t last;

    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) == NGE_ENTITY_DYING);
    nge_util_assert(nge_owner_count(id) == 0);   /* see nge_owner.h */

    /* Remove from the used list: the last entry moves into the gap */
    pos = nge_entity_used_pos[id];
    last = nge_entity_used[--nge_entity_used_count];
    nge_entity_used[pos] = last;
    nge_entity_used_pos[last] = pos;

    nge_entity_flags[id] = NGE_ENTITY_FREE;
    clear_components(id);
    nge_entity_free_stack[nge_entity_free_top++] = id;
}

uint8_t nge_entity_free_count(void) NGE_BANKED
{
    return nge_entity_free_top;
}

uint8_t nge_entity_dying_count(void) NGE_BANKED
{
    return nge_entity_dying_top;
}

