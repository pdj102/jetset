/*
 * The use action: banked in page 112 with the other actions (see act.h for
 * the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/item.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

uint8_t act_use(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    uint16_t hook;
    uint8_t ef;

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);

    /* 2. Validate: the actor has the item, stowed, and it does something when used */
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_EQUIPPED);
    }
    hook = gen_hook_of(item, GEN_HOOK_USE);
    ef = GEN_HOOK_EF(hook);
    if (ef == EF_NONE) {
        return act_fail(a, ACT_R_CANT_USE);
    }
    a->kind = ef;
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply: used up, then its effect on the user (whose events follow) */
    nge_event_post(item, EV_USED, actor, 0, 0);
    nge_entity_destroy(item);
    effect_apply(ef, GEN_HOOK_POWER(hook), actor, item);
    return act_succeed(a);
}
