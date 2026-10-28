/*
 * The pick up action: banked in page 112 with the other actions (see act.h
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

uint8_t act_pickup(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->target;
    nge_owner_t area;
    uint8_t x;
    uint8_t y;

    /* 1. Context: the actor's tile, and the item on it */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(item == NGE_ENTITY_INVALID || item < NGE_ENTITY_MAX);

    area = NGE_OWNER_OF(actor);
    x = NGE_LOCATION_X(actor);
    y = NGE_LOCATION_Y(actor);

    /* 2. Validate: something to hold it in, an item on the tile, room for it */
    if (!INV_HAS(actor)) {
        return act_fail(a, ACT_R_NO_INVENTORY);
    }
    if (item == NGE_ENTITY_INVALID) {
        item = nge_location_find(area, x, y, COMPONENT_ITEM, 0);
        if (item == NGE_ENTITY_INVALID) {
            return act_fail(a, ACT_R_NOTHING_HERE);
        }
    } else {
        if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item)) {
            return act_fail(a, ACT_R_NOT_ITEM);
        }
        if (!NGE_ENTITY_HAS_COMPONENT(item, NGE_COMPONENT_LOCATION) || NGE_OWNER_OF(item) != area ||
            NGE_LOCATION_X(item) != x || NGE_LOCATION_Y(item) != y) {
            return act_fail(a, ACT_R_NOTHING_HERE);
        }
    }
    a->target = item;
    if (inv_count(actor) >= inv_capacity[actor]) {
        return act_fail(a, ACT_R_FULL);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    inv_stow(item, actor);
    nge_event_post(item, EV_PICKED_UP, actor, 0, 0);
    return act_succeed(a);
}
