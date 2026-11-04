/*
 * A trap going off: banked in BANK_56 with the actions, as it strikes with
 * act_strike(). Not an action (no actor chose it): a reaction to a creature
 * moving onto a trap (feature_on_event()), and a disarm failed badly
 * (act_disarm()).
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
#include "game/components/kind.h"
#include "game/components/pc.h"
#include "game/gen/gen.h"
#include "game/level/level.h"

void act_spring(nge_entity_id_t trap, nge_entity_id_t victim) NGE_BANKED
{
    gen_feature_info_t info;
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
    } else {
        /* 4. Without one, it always hits: its hit hook at once (any saving throw is the effect's) */
        hook = gen_hook_of(trap, GEN_HOOK_HIT);
        if (GEN_HOOK_EF(hook) != EF_NONE) {
            effect_apply(GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook), victim, trap);
        }
    }
    /*
     * 5. A trap that leads down drops what it left alive, and not held, to
     * the next depth, anywhere on it: the player at the end of the frame
     * (a request, G-FLOW-1), anyone else now. Not from the bottom level
     */
    gen_feature_info(kind_index[trap], &info);
    if ((info.can & FCAN_DESCEND) && NGE_ENTITY_IS_ALIVE(victim) && health_hp[victim] != 0 && !act_rooted(victim) &&
        level_depth < LEVEL_DEPTH_BOTTOM) {
        if (PC_IS(victim)) {
            level_request((uint8_t)(level_depth + 1), LEVEL_ARRIVE_ANY);
        } else {
            level_send(victim, (uint8_t)(level_depth + 1));
        }
    }
}
