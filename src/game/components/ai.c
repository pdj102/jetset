/*
 * The AI component's functions: banked in page 110 with the other
 * component functions. It has no tables yet.
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/ai.h"

#include "core/nge_util.h"

static void check_entity(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

void ai_add(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id);

    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_AI);
}

void ai_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id);

    NGE_ENTITY_REMOVE_COMPONENT(id, COMPONENT_AI);
}
