/*
 * The equip, exchange and unequip actions: banked in BANK_56 with the other actions
 * (see act.h for the pattern their steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

/* The actor has the item: alive, an item, owned by it */
static uint8_t carried(nge_entity_id_t actor, nge_entity_id_t item)
{
    return NGE_ENTITY_IS_ALIVE(item) && ITEM_IS(item) && NGE_OWNER_OF(item) == actor;
}

/*
 * The checks equip and exchange share: the actor has the item stowed, it fits a
 * slot the actor has, and the one asked for (want, or SLOT_NONE for any). Returns
 * the mask of those slots, or 0 with a->reason set by act_fail() (refused)
 */
static uint16_t fitting_slots(act_t *a, nge_entity_id_t actor, nge_entity_id_t item, uint8_t want)
{
    uint16_t slots;

    if (!INV_HAS(actor)) {
        act_fail(a, ACT_R_NO_INVENTORY);
        return 0;
    }
    if (!carried(actor, item)) {
        act_fail(a, ACT_R_NOT_CARRIED);
        return 0;
    }
    if (item_slot[item] != SLOT_NONE) {
        act_fail(a, ACT_R_ALREADY_EQUIPPED);
        return 0;
    }
    nge_util_assert(KIND_IS(item, CLASS_ITEM));
    slots = gen_item_fits(kind_index[item]);
    if (slots == 0) {
        act_fail(a, ACT_R_CANT_EQUIP);
        return 0;
    }
    slots &= inv_slots[actor];
    if (want != SLOT_NONE) {
        slots &= SLOT_BIT(want);
    }
    if (slots == 0) {
        act_fail(a, ACT_R_NO_SLOT);
    }
    return slots;
}

uint8_t act_equip(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    uint8_t want = a->slot;
    nge_entity_id_t taken = NGE_ENTITY_INVALID;
    uint16_t slots;
    uint8_t slot;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);
    nge_util_assert(want < SLOT_COUNT);

    /* 2. Validate: carried, stowed, fits a slot the actor has, one that is free, hands for it */
    slots = fitting_slots(a, actor, item, want);
    if (slots == 0) {
        return ACT_FAIL;
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
    taken = inv_hands_taken(actor, item, slot);
    if (taken != NGE_ENTITY_INVALID) {
        a->target = taken;
        return act_fail(a, ACT_R_SLOT_TAKEN);
    }
    a->slot = slot;
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event: its worn effect now counts */
    item_slot[item] = slot;
    effects_recompute(actor);
    nge_event_post(item, EV_EQUIPPED, actor, slot, 0);
    return act_succeed(a);
}

uint8_t act_exchange(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    uint8_t want = a->slot;
    nge_entity_id_t off[2];   /* what comes off: the item in the slot, then what is in the way of the hands */
    nge_entity_id_t id;
    uint16_t slots;
    uint8_t slot;
    uint8_t first = SLOT_NONE;
    uint8_t n = 0;
    uint8_t i;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);
    nge_util_assert(want < SLOT_COUNT);

    /* 2. Validate: as equip, then what comes off can, and has room if two do */
    slots = fitting_slots(a, actor, item, want);
    if (slots == 0) {
        return ACT_FAIL;
    }
    /* The first slot it fits that is taken; the first it fits if none is (docs/actions.md R43, R44) */
    for (slot = 1; slot < SLOT_COUNT; slot++) {
        if (slots & SLOT_BIT(slot)) {
            if (first == SLOT_NONE) {
                first = slot;
            }
            id = inv_equipped(actor, slot);
            if (id != NGE_ENTITY_INVALID) {
                off[n++] = id;
                break;
            }
        }
    }
    if (slot == SLOT_COUNT) {
        slot = first;
    }
    /* What is in the way of the hands: the shield for a two-hander, a two-hander for a shield (R45, R46) */
    id = inv_hands_taken(actor, item, slot);
    if (id != NGE_ENTITY_INVALID) {
        off[n++] = id;
    }
    a->target = NGE_ENTITY_INVALID;
    for (i = 0; i < n; i++) {
        id = off[i];
        a->target = id;
        if (inv_cursed(id)) {
            return act_fail(a, ACT_R_CURSED);
        }
    }
    /* One for one needs no room (R41); two for one needs a free place (R45): target, the second */
    if (n == 2 && inv_count(actor) >= inv_capacity[actor]) {
        return act_fail(a, ACT_R_FULL);
    }
    a->slot = slot;
    if (n != 0) {
        a->target = off[0];
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the events, off before on: the worn effects are recomputed once */
    for (i = 0; i < n; i++) {
        item_slot[off[i]] = SLOT_NONE;
    }
    item_slot[item] = slot;
    effects_recompute(actor);
    for (i = 0; i < n; i++) {
        nge_event_post(off[i], EV_UNEQUIPPED, actor, 0, 0);
    }
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

    /* 2. Validate: carried, equipped, not cursed, room to stow it */
    if (!carried(actor, item)) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] == SLOT_NONE) {
        return act_fail(a, ACT_R_NOT_EQUIPPED);
    }
    if (inv_cursed(item)) {
        return act_fail(a, ACT_R_CURSED);
    }
    if (inv_count(actor) >= inv_capacity[actor]) {
        return act_fail(a, ACT_R_FULL);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event: its worn effect no longer counts */
    item_slot[item] = SLOT_NONE;
    effects_recompute(actor);
    nge_event_post(item, EV_UNEQUIPPED, actor, 0, 0);
    return act_succeed(a);
}
