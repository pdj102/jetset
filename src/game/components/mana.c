/*
 * The mana component's functions: banked in page 110 with the other
 * component functions; its tables are in slot 0 (mana_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/mana.h"

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/events.h"

/* An entity in use with the mana component */
static void check_mana(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(MANA_IS(id));
}

void mana_add(nge_entity_id_t id, const mana_kind_t *k) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(MANA_KIND_HAS(k));

    mana_mp[id] = k->mp_max;
    mana_mp_max[id] = k->mp_max;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_MANA);
}

void mana_spend(nge_entity_id_t id, uint8_t amount) NGE_BANKED
{
    uint8_t mp;

    check_mana(id);
    mp = mana_mp[id];
    nge_util_assert(amount <= mp);   /* the caller checks MANA_HAS() */
    if (amount == 0) {
        return;
    }
    mana_mp[id] = (amount >= mp) ? 0 : (uint8_t)(mp - amount);   /* never below 0, asserts or not */
    nge_event_post(id, EV_MANA_SPENT, NGE_EVENT_NONE, amount, 0);
}

uint8_t mana_restore(nge_entity_id_t id, uint8_t amount) NGE_BANKED
{
    uint8_t mp;
    uint8_t room;

    check_mana(id);
    mp = mana_mp[id];
    room = (uint8_t)(mana_mp_max[id] - mp);
    if (amount > room) {
        amount = room;
    }
    if (amount != 0) {
        mana_mp[id] = (uint8_t)(mp + amount);
        nge_event_post(id, EV_MANA_RESTORED, NGE_EVENT_NONE, amount, 0);
    }
    return amount;
}
