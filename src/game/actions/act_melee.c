/*
 * The melee action: banked in page 112 with the other actions (see act.h
 * for the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/events.h"

uint8_t act_melee(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t target = a->target;
    nge_entity_id_t weapon = NGE_ENTITY_INVALID;
    nge_entity_id_t attacker;   /* whose attack component: the weapon's or the actor's */
    nge_owner_t area;
    uint8_t x;
    uint8_t y;
    uint8_t roll;
    uint8_t damage;
    uint8_t ef;

    /* 1. Context: the attack used, and the target */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENTS(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(target == NGE_ENTITY_INVALID || target < NGE_ENTITY_MAX);

    area = NGE_OWNER_OF(actor);
    x = NGE_LOCATION_X(actor);
    y = NGE_LOCATION_Y(actor);
    if (INV_HAS(actor)) {
        weapon = inv_equipped(actor, SLOT_WEAPON);
        if (weapon != NGE_ENTITY_INVALID && !ATTACK_HAS(weapon, ATTACK_MELEE)) {
            weapon = NGE_ENTITY_INVALID;   /* wielded, but not a melee weapon */
        }
    }
    attacker = (weapon != NGE_ENTITY_INVALID) ? weapon : actor;
    if (target == NGE_ENTITY_INVALID) {
        nge_util_assert(a->dx >= -1 && a->dx <= 1 && a->dy >= -1 && a->dy <= 1 && (a->dx | a->dy) != 0);
        target = nge_location_find(area, (uint8_t)(x + a->dx), (uint8_t)(y + a->dy), COMPONENT_HEALTH, 0);
    }
    a->target = target;
    a->item = weapon;

    /* 2. Validate: an attack, a target that can be damaged, within reach */
    if (!ATTACK_HAS(attacker, ATTACK_MELEE)) {
        return act_fail(a, ACT_R_NO_ATTACK);
    }
    if (target == NGE_ENTITY_INVALID || target == actor) {
        return act_fail(a, ACT_R_NO_TARGET);
    }
    if (!NGE_ENTITY_IS_ALIVE(target) || !NGE_ENTITY_HAS_ANY_COMPONENTS(target, COMPONENT_HEALTH) ||
        health_hp[target] == 0) {
        return act_fail(a, ACT_R_CANT_DAMAGE);
    }
    if (!NGE_ENTITY_HAS_COMPONENTS(target, NGE_COMPONENT_LOCATION) || NGE_OWNER_OF(target) != area) {
        return act_fail(a, ACT_R_OUT_OF_RANGE);
    }
    roll = act_distance(x, y, NGE_LOCATION_X(target), NGE_LOCATION_Y(target));
    if (roll == 0 || roll > attack_range[ATTACK_MELEE][attacker]) {
        return act_fail(a, ACT_R_OUT_OF_RANGE);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: the d20 roll, and on a hit the damage */
    roll = (uint8_t)(1 + (unsigned int)rand() % 20);
    if (!attack_hits(attacker, ATTACK_MELEE, target, roll)) {
        /* 4. Apply: a miss is an attempt: the turn is used */
        nge_event_post(target, EV_MISSED, actor, roll, weapon);
        return act_fail(a, ACT_R_MISSED);
    }
    damage = attack_damage(attacker, ATTACK_MELEE);

    /* 4. Apply: the hit, then the damage (its events follow), then the hit's effect */
    nge_event_post(target, EV_HIT, actor, roll, weapon);
    a->kind = attack_type[ATTACK_MELEE][attacker];
    a->amount = health_damage(target, actor, damage, a->kind);
    ef = gen_hook(attacker, GEN_HOOK_HIT);
    if (ef != EF_NONE) {
        effect_apply(ef, target, actor);   /* refused if the hit killed it */
    }
    return act_succeed(a);
}
