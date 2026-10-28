/*
 * The item component's functions: banked in page 110 with the shared
 * generator helpers; its table is in slot 0 (item_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/item.h"

#include "core/nge_util.h"

void item_add(nge_entity_id_t id) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    item_slot[id] = SLOT_NONE;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_ITEM);
}
