/*
 * The inventory component's functions: banked in page 110 with the shared
 * generator helpers; its tables are in slot 0 (inventory_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/inventory.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "game/components/item.h"

static void check_entity(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

/* An alive item owned by holder in slot: SLOT_NONE for stowed ones */
static uint8_t held_in(nge_entity_id_t id, nge_entity_id_t holder, uint8_t slot)
{
    return NGE_ENTITY_IS_ALIVE(id) && ITEM_IS(id) && NGE_OWNER_OF(id) == holder && item_slot[id] == slot;
}

void inv_add(nge_entity_id_t id, uint8_t capacity, uint16_t slots) NGE_BANKED
{
    check_entity(id);
    nge_util_assert(capacity != 0);

    inv_capacity[id] = capacity;
    inv_slots[id] = slots;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_INVENTORY);
}

uint8_t inv_count(nge_entity_id_t holder) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t n = 0;

    check_entity(holder);
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (held_in(id, holder, SLOT_NONE)) {
            n++;
        }
    }
    return n;
}

nge_entity_id_t inv_next(nge_entity_id_t holder, nge_entity_id_t after) NGE_BANKED
{
    nge_entity_id_t id = (after == NGE_ENTITY_INVALID) ? 0 : (nge_entity_id_t)(after + 1);

    check_entity(holder);
    for (; id < NGE_ENTITY_MAX; id++) {
        if (held_in(id, holder, SLOT_NONE)) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

nge_entity_id_t inv_equipped(nge_entity_id_t holder, uint8_t slot) NGE_BANKED
{
    nge_entity_id_t id;

    check_entity(holder);
    nge_util_assert(slot != SLOT_NONE && slot < SLOT_COUNT);
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (held_in(id, holder, slot)) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

void inv_stow(nge_entity_id_t item, nge_entity_id_t holder) NGE_BANKED
{
    check_entity(item);
    nge_util_assert(ITEM_IS(item));

    nge_owner_set(item, holder);
    nge_location_remove(item);
    item_slot[item] = SLOT_NONE;
}

void inv_place(nge_entity_id_t item, nge_owner_t area, uint8_t x, uint8_t y) NGE_BANKED
{
    check_entity(item);
    nge_util_assert(ITEM_IS(item));

    nge_owner_set(item, area);
    nge_location_set(item, x, y);
    item_slot[item] = SLOT_NONE;
}

uint8_t inv_drop_all(nge_entity_id_t holder) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t n = 0;
    uint8_t i;

    check_entity(holder);
    if (!NGE_ENTITY_HAS_COMPONENT(holder, NGE_COMPONENT_LOCATION)) {
        return 0;
    }
    NGE_OWNER_FOR_EACH_OWNED(i, id, holder) {
        if (ITEM_IS(id)) {
            inv_place(id, NGE_OWNER_OF(holder), NGE_LOCATION_X(holder), NGE_LOCATION_Y(holder));
            n++;
        }
    }
    return n;
}
