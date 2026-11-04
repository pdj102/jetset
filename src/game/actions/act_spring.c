/*
 * A trap going off: banked in BANK_56 with the actions, as it strikes with
 * act_strike(). Not an action (no actor chose it): a reaction to a creature
 * moving onto a trap (feature_on_event()), and a failed disarm (later).
 * See docs/features.md 4.8.
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/attack.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

void act_spring(nge_entity_id_t trap, nge_entity_id_t victim) NGE_BANKED
{
    act_t a = { 0 };
    uint16_t hook;

    nge_util_assert(trap < NGE_ENTITY_MAX && FEATURE_IS(trap));
    nge_util_assert(victim < NGE_ENTITY_MAX);

    if (!NGE_ENTITY_IS_ALIVE(trap) || !NGE_ENTITY_IS_ALIVE(victim) ||
        !NGE_ENTITY_HAS_COMPONENT(victim, COMPONENT_HEALTH) || health_hp[victim] == 0) {
        return;
    }
    /* 1, 2. It goes off, and if it was hidden it is found: drawn, and known from now on */
    nge_event_post(trap, EV_TRIGGERED, victim, 0, 0);
    if (FEATURE_IS_HIDDEN(trap)) {
        feature_find(trap);
        nge_event_post(trap, EV_FOUND, NGE_EVENT_NONE, 0, 0);
    }
    /* 3. With an attack, a roll against the victim's AC, and its hit hook on a hit that leaves it alive */
    if (ATTACK_HAS(trap, ATTACK_MELEE)) {
        a.actor = trap;
        a.target = victim;
        a.item = NGE_ENTITY_INVALID;
        act_strike(&a, trap, ATTACK_MELEE, trap, 0, 0);
        return;
    }
    /* 4. Without one, it always hits: its hit hook at once (any saving throw is the effect's) */
    hook = gen_hook_of(trap, GEN_HOOK_HIT);
    if (GEN_HOOK_EF(hook) != EF_NONE) {
        effect_apply(GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook), victim, trap);
    }
}
