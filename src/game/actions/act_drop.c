/*
 * The drop action: banked in page 112 with the other actions (see act.h
 * for the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/events.h"

uint8_t act_drop(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(item < NGE_ENTITY_MAX);

    /* 2. Validate: the actor has the item, stowed */
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_EQUIPPED);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    inv_place(item, NGE_OWNER_OF(actor), NGE_LOCATION_X(actor), NGE_LOCATION_Y(actor));
    nge_event_post(item, EV_DROPPED, actor, 0, 0);
    return act_succeed(a);
}
