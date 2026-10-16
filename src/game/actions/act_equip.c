/*
 * The equip and unequip actions: banked in page 112 with the other actions
 * (see act.h for the pattern their steps follow).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/events.h"
#include "game/gen/gen.h"

/* The actor has the item: alive, an item, owned by it */
static uint8_t carried(nge_entity_id_t actor, nge_entity_id_t item)
{
    return NGE_ENTITY_IS_ALIVE(item) && ITEM_IS(item) && NGE_OWNER_OF(item) == actor;
}

uint8_t act_equip(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    uint8_t want = a->kind;
    nge_entity_id_t taken = NGE_ENTITY_INVALID;
    uint16_t slots;
    uint8_t slot;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);
    nge_util_assert(want < SLOT_COUNT);

    /* 2. Validate: carried, stowed, fits a slot the actor has, one that is free */
    if (!INV_HAS(actor)) {
        return act_fail(a, ACT_R_NO_INVENTORY);
    }
    if (!carried(actor, item)) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_ALREADY_EQUIPPED);
    }
    nge_util_assert(KIND_IS(item, CLASS_ITEM));
    slots = gen_item_fits(kind_index[item]);
    if (slots == 0) {
        return act_fail(a, ACT_R_CANT_EQUIP);
    }
    slots &= inv_slots[actor];
    if (want != SLOT_NONE) {
        slots &= SLOT_BIT(want);
    }
    if (slots == 0) {
        return act_fail(a, ACT_R_NO_SLOT);
    }
    /* The first slot it fits that is free */
    for (slot = 1; slot < SLOT_COUNT; slot++) {
        if (slots & SLOT_BIT(slot)) {
            taken = inv_equipped(actor, slot);
            if (taken == NGE_ENTITY_INVALID) {
                break;
            }
        }
    }
    if (slot == SLOT_COUNT) {
        a->target = taken;   /* the item in the (last) slot it fits */
        return act_fail(a, ACT_R_SLOT_TAKEN);
    }
    a->kind = slot;
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    item_slot[item] = slot;
    nge_event_post(item, EV_EQUIPPED, actor, slot, 0);
    return act_succeed(a);
}

uint8_t act_unequip(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);

    /* 2. Validate: carried, equipped, room to stow it */
    if (!carried(actor, item)) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] == SLOT_NONE) {
        return act_fail(a, ACT_R_NOT_EQUIPPED);
    }
    if (inv_count(actor) >= inv_capacity[actor]) {
        return act_fail(a, ACT_R_FULL);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    item_slot[item] = SLOT_NONE;
    nge_event_post(item, EV_UNEQUIPPED, actor, 0, 0);
    return act_succeed(a);
}
