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
#include "game/level/level.h"
#include "game/spells/spells.h"
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

/*
 * The tiles of an area, ring by ring from the centre: nge_map_within()'s
 * circle (dx*dx + dy*dy <= r*r + r) as offsets, the first AREA_TILES_1
 * within 1, all AREA_TILES_2 within 2. A table, not the test on every tile
 * of the square at each ring: half the code. test_act checks it against
 * nge_map_within()
 */
#define AREA_TILES_1  9
#define AREA_TILES_2  21

/* The table holds the radii act.h names */
typedef char act_area_radius_check[(ACT_BALL_RADIUS == 1 && ACT_BURST_RADIUS == 2) ? 1 : -1];

static const int8_t area_dx[AREA_TILES_2] = {
    0,
    -1, 0, 1, -1, 1, -1, 0, 1,
    -1, 0, 1, -2, 2, -2, 2, -2, 2, -1, 0, 1,
};
static const int8_t area_dy[AREA_TILES_2] = {
    0,
    -1, -1, -1, 0, 0, 1, 1, 1,
    -2, -2, -2, -1, -1, 0, 0, 1, 1, 2, 2, 2,
};

/*
 * The creatures of an area: within radius of (s->cx, s->cy), ring by ring
 * from it, each in a clear line from the centre (act_line()'s test, by
 * nge_block_line(): the centre was tested by act_line(), or is the actor's,
 * and a tile with a creature on it is never solid), into s->ids[] up to
 * ACT_AREA_MAX, the actor only if with_actor. Sets a->target to the first
 * that isn't the actor. Returns ACT_R_NONE, or ACT_R_NO_TARGET with none but
 * the actor. See docs/items_aimed.md 4.3
 */
static uint8_t act_area(act_t *a, act_shape_t *s, uint8_t radius, uint8_t with_actor)
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t first = NGE_ENTITY_INVALID;
    nge_entity_id_t id;
    nge_owner_t area = NGE_OWNER_OF(actor);
    uint8_t cx = s->cx;   /* in locals (T-ZSDCC-1) */
    uint8_t cy = s->cy;
    uint8_t tiles = (radius == ACT_BALL_RADIUS) ? AREA_TILES_1 : AREA_TILES_2;
    uint8_t count = 0;
    uint8_t i;
    uint8_t x;
    uint8_t y;

    for (i = 0; i < tiles; i++) {
        x = (uint8_t)(cx + area_dx[i]);   /* off the map's left or top edge: 254 or 255, off the map */
        y = (uint8_t)(cy + area_dy[i]);
        if (x >= nge_map.width || y >= nge_map.height || !nge_block_line(area, cx, cy, x, y, NGE_BLOCK_SOLID)) {
            continue;
        }
        NGE_LOCATION_FOR_EACH_AT(id, area, x, y, COMPONENT_HEALTH) {
            if (count < ACT_AREA_MAX && NGE_ENTITY_IS_ALIVE(id) && health_hp[id] != 0 && (id != actor || with_actor)) {
                s->ids[count] = id;
                count++;
                if (id != actor && first == NGE_ENTITY_INVALID) {
                    first = id;
                }
            }
        }
    }
    s->count = count;
    a->target = first;   /* for the AI's check */
    return (first == NGE_ENTITY_INVALID) ? ACT_R_NO_TARGET : ACT_R_NONE;
}

uint8_t act_shape_find(act_t *a, act_shape_t *s, uint8_t range)
{
    nge_entity_id_t actor = a->actor;
    uint8_t shape = s->shape;
    uint8_t tx = a->x;   /* in locals (T-ZSDCC-1) */
    uint8_t ty = a->y;
    uint8_t reason;

    nge_util_assert(shape < SPELL_SHAPE_COUNT);

    s->count = 0;
    if (shape == SPELL_SELF) {
        a->target = actor;
        return ACT_R_NONE;
    }
    if (shape == SPELL_BURST) {
        /* Around the actor, not the actor: no tile is read */
        s->cx = NGE_LOCATION_X(actor);
        s->cy = NGE_LOCATION_Y(actor);
        return act_area(a, s, ACT_BURST_RADIUS, 0);
    }
    if (shape == SPELL_TILE || shape == SPELL_BALL) {
        /* A direct line reaches the tile with a creature on it or with none, but not the actor's own */
        reason = act_line(a, range, ACT_LINE_DIRECT);
        if (reason != ACT_R_NONE &&
            (reason != ACT_R_NO_TARGET || (tx == NGE_LOCATION_X(actor) && ty == NGE_LOCATION_Y(actor)))) {
            return reason;   /* out of range, no line, or its own tile */
        }
        s->cx = tx;
        s->cy = ty;
        if (shape == SPELL_BALL) {
            return act_area(a, s, ACT_BALL_RADIUS, 1);   /* it catches the actor (D4) */
        }
        /* A tile: free, or a creature is in the way; the effect is the actor's */
        if (reason == ACT_R_NONE) {
            return ACT_R_BLOCKED_ENTITY;   /* a->target: the creature */
        }
        if (!level_can_enter(actor, tx, ty, &a->target)) {
            return (a->target != NGE_ENTITY_INVALID) ? ACT_R_BLOCKED_ENTITY : ACT_R_BLOCKED;
        }
        a->target = actor;
        return ACT_R_NONE;
    }

    /* Self is done; bolt and direct. The actor's own tile is ACT_R_NO_TARGET: a check there asks whether it can act at all */
    return act_line(a, range, (shape == SPELL_DIRECT) ? ACT_LINE_DIRECT : ACT_LINE_BOLT);
}

void act_shape_apply(act_t *a, const act_shape_t *s, uint8_t ef, int8_t power)
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t id;
    uint8_t shape = s->shape;   /* in locals (T-ZSDCC-1); the loop's bound too */
    uint8_t count = s->count;
    uint8_t i;

    /* The shape is valid: act_shape_find() asserted it */
    if (shape == SPELL_TILE) {
        effect_apply_at(ef, power, actor, actor, s->cx, s->cy);
    } else if (SPELL_IS_AREA(shape)) {
        /* The area, then each creature found, if an effect before hasn't killed it (D13) */
        nge_event_post(NGE_EVENT_NONE, EV_AREA, actor, s->cx, s->cy);
        for (i = 0; i < count; i++) {
            id = s->ids[i];
            if (NGE_ENTITY_IS_ALIVE(id)) {
                effect_apply(ef, power, id, actor);
            }
        }
    } else {
        effect_apply(ef, power, a->target, actor);   /* self, bolt, direct */
    }
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
