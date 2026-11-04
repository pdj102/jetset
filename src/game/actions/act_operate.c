/*
 * The operate action: banked in BANK_56 with the other actions (see act.h
 * for the pattern its steps follow, and docs/features.md 4.6).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "event/nge_event.h"
#include "game/components/feature.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

uint8_t act_operate(act_t *a, uint8_t mode) NGE_BANKED
{
    gen_feature_info_t info;
    nge_entity_id_t target;
    uint16_t hook;
    uint8_t ef;
    uint8_t reason;

    /* 1, 2. Context and validation: a found feature next to the actor that can be operated, not spent */
    reason = act_find_feature(a, FCAN_OPERATE, ACT_R_CANT_OPERATE);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    target = a->target;
    if (FEATURE_IS_SPENT(target)) {
        return act_fail(a, ACT_R_SPENT);
    }
    hook = gen_hook_of(target, GEN_HOOK_USE);
    ef = GEN_HOOK_EF(hook);
    a->effect = ef;
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll (the effect may have a saving throw of its own) */

    /* 4. Apply: the event first, as act_use()'s, so the effect's messages follow it; then spent */
    nge_event_post(target, EV_OPERATED, a->actor, ef, 0);
    effect_apply(ef, GEN_HOOK_POWER(hook), a->actor, target);
    gen_feature_info(kind_index[target], &info);
    if (info.can & FCAN_ONCE) {
        feature_spend(target);
    }
    return act_succeed(a);
}
