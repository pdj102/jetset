/*
 * The aim action: banked in BANK_56 with the other actions (see act.h for
 * the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

uint8_t act_aim(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    nge_entity_id_t target = actor;
    uint16_t hook;
    uint8_t ef;
    uint8_t wand;
    uint8_t how;
    uint8_t reason;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(item < NGE_ENTITY_MAX);

    /* 2. Validate: the actor has the item, stowed, with an aim hook and a charge, and a target */
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_EQUIPPED);
    }
    hook = gen_hook_of(item, GEN_HOOK_AIM);
    ef = GEN_HOOK_EF(hook);
    if (ef == EF_NONE) {
        return act_fail(a, ACT_R_CANT_AIM);
    }
    wand = gen_item_subclass(kind_index[item]) == ITEM_WAND;
    if (wand && item_count[item] == 0) {
        return act_fail(a, ACT_R_NO_CHARGES);
    }
    a->kind = ef;
    if (a->x == NGE_LOCATION_X(actor) && a->y == NGE_LOCATION_Y(actor)) {
        a->target = actor;   /* only aim does this: a staff of healing can heal its holder */
    } else {
        how = (gen_item_flags(kind_index[item]) & ITEMF_DIRECT) ? ACT_LINE_DIRECT : ACT_LINE_BOLT;
        reason = act_line(a, ACT_AIM_RANGE, how);
        if (reason != ACT_R_NONE) {
            return act_fail(a, reason);
        }
        target = a->target;
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll: the effect's own saving throw decides what it does */

    /* 4. Apply: a charge used, or a parchment used up, then the effect, with the actor as its source */
    nge_event_post(target, EV_AIMED, actor, item, 0);
    if (wand) {
        item_count[item]--;
    } else {
        nge_entity_destroy(item);
    }
    effect_apply(ef, GEN_HOOK_POWER(hook), target, actor);
    return act_succeed(a);
}
