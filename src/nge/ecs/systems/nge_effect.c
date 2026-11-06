/*
 * Effect system: banked in page 58, with no data of its own: an active
 * effect is its entity, its owner, the effect tag and its timer, all in
 * slot 0. It calls the entity, owner and timer functions (banked) and
 * nothing of the game's.
 */
#ifdef __SDCC
#pragma codeseg PAGE_58
#pragma constseg PAGE_58
#endif

#include "ecs/systems/nge_effect.h"

#include "core/nge_util.h"
#include "ecs/systems/nge_timer.h"

nge_entity_id_t nge_effect_create(nge_entity_id_t target, uint8_t kind, uint8_t period, uint8_t cycles,
                                  uint8_t p0, uint8_t p1) NGE_BANKED
{
    nge_entity_id_t e;

    nge_util_assert(target < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(target));
    nge_util_assert(period != 0);

    e = nge_entity_create();
    if (e == NGE_ENTITY_INVALID) {
        return NGE_ENTITY_INVALID;
    }
    nge_owner_set(e, target);   /* it goes when its target goes */
    NGE_ENTITY_ADD_COMPONENT(e, NGE_COMPONENT_EFFECT);
    nge_timer_payload[0][e] = p0;
    nge_timer_payload[1][e] = p1;
    nge_timer_add(e, kind, period, cycles);
    return e;
}

void nge_effect_destroy(nge_entity_id_t effect) NGE_BANKED
{
    nge_util_assert(effect < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(effect));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT));
    nge_util_assert(NGE_OWNER_IS_ENTITY(NGE_OWNER_OF(effect)));

    /* Its timer stays: a dying entity's never fires, and release clears it */
    nge_entity_destroy(effect);
}
