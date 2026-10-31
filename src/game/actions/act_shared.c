/*
 * Helpers shared by the actions: in BANK_56 with them, not banked, so
 * only code in BANK_56 may call them (directly, with no bank switch).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include <stddef.h>
#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "map/nge_map.h"

/* What a line of fire passes: everything solid or outside the map stops it, and pits don't */
#define LINE_PASS  ((nge_block_t)~(NGE_BLOCK_SOLID | NGE_BLOCK_EDGE))

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

uint8_t act_line(act_t *a, uint8_t range)
{
    nge_block_walk_t w;
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t id;
    nge_owner_t area = NGE_OWNER_OF(actor);
    uint8_t ax = NGE_LOCATION_X(actor);
    uint8_t ay = NGE_LOCATION_Y(actor);
    uint8_t tx = a->x;
    uint8_t ty = a->y;
    uint8_t x;
    uint8_t y;

    a->target = NGE_ENTITY_INVALID;
    if (!nge_map_within(ax, ay, tx, ty, range)) {
        return ACT_R_OUT_OF_RANGE;
    }
    if (ax == tx && ay == ty) {
        return ACT_R_NO_TARGET;
    }
    nge_block_walk_start(&w, ax, ay, tx, ty);
    while (nge_block_walk_next(&w)) {
        x = w.x;
        y = w.y;
        if (!nge_block_can_enter(area, x, y, LINE_PASS, NGE_ENTITY_INVALID, NULL)) {
            return ACT_R_NO_LINE;
        }
        NGE_LOCATION_FOR_EACH_AT(id, area, x, y, COMPONENT_HEALTH) {
            if (health_hp[id] != 0) {
                a->target = id;
                return ACT_R_NONE;
            }
        }
    }
    return ACT_R_NO_TARGET;
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
