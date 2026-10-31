/*
 * Helpers shared by the actions: in BANK_56 with them, not banked, so
 * only code in BANK_56 may call them (directly, with no bank switch).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks)
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t target = a->target;
    nge_entity_id_t item = a->item;
    uint8_t roll;
    uint8_t damage;
    uint16_t hook;

    /* Resolve: the d20 roll, and on a hit the damage */
    roll = (uint8_t)(1 + (unsigned int)rand() % 20);
    if (!attack_hits(attacker, use, target, roll)) {
        /* A miss is an attempt: the turn is used */
        nge_event_post(target, EV_MISSED, actor, roll, item);
        return act_fail(a, ACT_R_MISSED);
    }
    damage = attack_damage(attacker, use);

    /* Apply: the hit, then the damage (its events follow), then the hit's effect */
    nge_event_post(target, EV_HIT, actor, roll, item);
    a->kind = attack_type[use][attacker];
    a->amount = health_damage(target, actor, damage, a->kind);
    hook = gen_hook_of(hooks, GEN_HOOK_HIT);
    if (GEN_HOOK_EF(hook) != EF_NONE) {
        effect_apply(GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook), target, actor);   /* refused if the hit killed it */
    }
    return act_succeed(a);
}

uint8_t act_rooted(nge_entity_id_t id)
{
    return NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) && STATUS_HAS(id, STATUS_ROOTED);
}

uint8_t act_fail(act_t *a, uint8_t reason)
{
    nge_util_assert(reason != ACT_R_NONE && reason < ACT_R_COUNT);

    a->reason = reason;
    a->used = (act_reason_flags[reason] & ACT_RF_ATTEMPTED) != 0;
    return ACT_FAIL;
}

uint8_t act_succeed(act_t *a)
{
    a->reason = ACT_R_NONE;
    a->used = 1;
    return ACT_SUCCESS;
}

uint8_t act_checked(act_t *a)
{
    a->reason = ACT_R_NONE;
    a->used = 0;
    return ACT_SUCCESS;
}
