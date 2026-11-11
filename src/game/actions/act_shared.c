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
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "map/nge_map.h"

/* What a line of fire passes: everything solid or outside the map stops it, and pits don't */
#define LINE_PASS  ((nge_block_t)~(NGE_BLOCK_SOLID | NGE_BLOCK_EDGE))

uint8_t act_strike(act_t *a, nge_entity_id_t attacker, uint8_t use, nge_entity_id_t hooks, int8_t to_hit,
                   int8_t to_damage)
{
    attack_kind_t k;
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t target = a->target;
    nge_entity_id_t item = a->item;
    uint8_t roll;
    uint8_t damage;
    uint16_t hook;

    /* Resolve: the d20 roll, and on a hit the damage */
    roll = (uint8_t)(1 + (unsigned int)rand() % 20);
    if (!attack_hits(attacker, use, target, roll, to_hit)) {
        /* A miss is an attempt: the turn is used */
        nge_event_post(target, EV_MISSED, actor, roll, item);
        return act_fail(a, ACT_R_MISSED);
    }
    damage = attack_damage(attacker, use, to_damage);

    /* Apply: the hit, then the damage (its events follow), then the hit's effect */
    nge_event_post(target, EV_HIT, actor, roll, item);
    attack_get(attacker, use, &k);
    a->damage = k.type;
    a->amount = health_damage(target, actor, damage, a->damage);
    hook = gen_hook_of(hooks, GEN_HOOK_HIT);
    if (GEN_HOOK_EF(hook) != EF_NONE) {
        effect_apply(GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook), target, actor);   /* refused if the hit killed it */
    }
    return act_succeed(a);
}

uint8_t act_line(act_t *a, uint8_t range, uint8_t how)
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

    nge_util_assert(how < ACT_LINE_COUNT);

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
        if (how == ACT_LINE_DIRECT && (x != tx || y != ty)) {
            continue;   /* a direct line passes over creatures on the way */
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

uint8_t act_find_feature(act_t *a, uint8_t can, uint8_t cant)
{
    gen_feature_info_t info;
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t target = a->target;
    nge_owner_t area;
    uint8_t x;
    uint8_t y;

    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(target == NGE_ENTITY_INVALID || target < NGE_ENTITY_MAX);

    area = NGE_OWNER_OF(actor);
    x = NGE_LOCATION_X(actor);
    y = NGE_LOCATION_Y(actor);
    if (target == NGE_ENTITY_INVALID) {
        nge_util_assert(a->dx >= -1 && a->dx <= 1 && a->dy >= -1 && a->dy <= 1);
        target = nge_location_find(area, (uint8_t)(x + a->dx), (uint8_t)(y + a->dy), COMPONENT_FEATURE, 0);
        a->target = target;
        if (target == NGE_ENTITY_INVALID) {
            return ACT_R_NOTHING_THERE;
        }
    }
    if (!NGE_ENTITY_IS_ALIVE(target) || !FEATURE_IS(target)) {
        return cant;
    }
    if (FEATURE_IS_HIDDEN(target)) {
        return ACT_R_NOTHING_THERE;
    }
    gen_feature_info(kind_index[target], &info);
    if (!(info.can & can)) {
        return cant;
    }
    if (!NGE_ENTITY_HAS_COMPONENT(target, NGE_COMPONENT_LOCATION) || NGE_OWNER_OF(target) != area ||
        !nge_map_within(x, y, NGE_LOCATION_X(target), NGE_LOCATION_Y(target), 1)) {
        return ACT_R_OUT_OF_RANGE;
    }
    return ACT_R_NONE;
}

uint8_t act_rooted(nge_entity_id_t id)
{
    return act_status(id, STATUS_ROOTED);
}

uint8_t act_status(nge_entity_id_t id, uint8_t s)
{
    return NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) && STATUS_HAS(id, s);
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
