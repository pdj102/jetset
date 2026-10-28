/*
 * The kind component: banked in page 110 with the shared generator helpers
 * (it runs when an entity is created); its tables are in slot 0
 * (kind_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/kind.h"

#include "core/nge_util.h"

void kind_add(nge_entity_id_t id, uint8_t c, uint8_t i) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    kind_class[id] = c;
    kind_index[id] = i;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_KIND);
}
