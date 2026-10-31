/*
 * The pick up action: banked in BANK_56 with the other actions (see act.h
 * for the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/events.h"
#include "game/gen/gen.h"

#define STACK_MAX  255   /* the most an item_count[] holds */

/*
 * The stack item can merge into: actor's first of the same kind with room,
 * stowed or equipped, or NGE_ENTITY_INVALID. Only ammo: on anything else a
 * count isn't a quantity (a wand's charges, a lantern's fuel)
 */
static nge_entity_id_t find_stack(nge_entity_id_t actor, nge_entity_id_t item)
{
    nge_entity_id_t id;
    uint8_t kind = kind_index[item];

    if (gen_item_subclass(kind) == ITEM_AMMO) {
        for (id = 0; id < NGE_ENTITY_MAX; id++) {
            if (NGE_ENTITY_IS_ALIVE(id) && ITEM_IS(id) && NGE_OWNER_OF(id) == actor && kind_index[id] == kind &&
                item_count[id] < STACK_MAX) {
                return id;
            }
        }
    }
    return NGE_ENTITY_INVALID;
}

uint8_t act_pickup(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->target;
    nge_entity_id_t stack;
    nge_owner_t area;
    uint8_t x;
    uint8_t y;
    uint8_t room;

    /* 1. Context: the actor's tile, and the item on it */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(item == NGE_ENTITY_INVALID || item < NGE_ENTITY_MAX);

    area = NGE_OWNER_OF(actor);
    x = NGE_LOCATION_X(actor);
    y = NGE_LOCATION_Y(actor);

    /* 2. Validate: something to hold it in, an item on the tile, a stack or room for it */
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
    stack = find_stack(actor, item);
    if (stack == NGE_ENTITY_INVALID && inv_count(actor) >= inv_capacity[actor]) {
        return act_fail(a, ACT_R_FULL);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply: into the stack, what fits of it, or stowed. Then the event */
    if (stack == NGE_ENTITY_INVALID) {
        inv_stow(item, actor);
    } else {
        room = (uint8_t)(STACK_MAX - item_count[stack]);
        if (item_count[item] <= room) {
            item_count[stack] = (uint8_t)(item_count[stack] + item_count[item]);
            nge_entity_destroy(item);
        } else {
            item_count[stack] = STACK_MAX;
            item_count[item] = (uint8_t)(item_count[item] - room);   /* the rest stays where it is */
        }
    }
    nge_event_post(item, EV_PICKED_UP, actor, 0, 0);
    return act_succeed(a);
}
