/*
 * The attack component's functions: banked in page 110 with the other
 * component functions; its tables are page data in page 111
 * (attack_pdata.c), in slot 7 while this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/attack_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/stats.h"

static void check_entity(nge_entity_id_t id, uint8_t use)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    (void)use;
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(use < ATTACK_USE_COUNT);
}

void attack_add(nge_entity_id_t id, uint8_t use, const attack_kind_t *k) NGE_BANKED
{
    check_entity(id, use);
    nge_util_assert(k->dice != 0 && k->sides != 0 && k->type < DMG_COUNT);
    nge_util_assert(use == ATTACK_MELEE ? k->range == 1 : k->range >= 2);

    attack_to_hit[use][id] = k->to_hit;
    attack_dice[use][id] = k->dice;
    attack_sides[use][id] = k->sides;
    attack_to_damage[use][id] = k->to_damage;
    attack_type[use][id] = k->type;
    attack_range[use][id] = k->range;
    NGE_ENTITY_ADD_COMPONENT(id, ATTACK_COMPONENT(use));
}

void attack_remove(nge_entity_id_t id, uint8_t use) NGE_BANKED
{
    check_entity(id, use);

    NGE_ENTITY_REMOVE_COMPONENT(id, ATTACK_COMPONENT(use));
}

void attack_get(nge_entity_id_t id, uint8_t use, attack_kind_t *out) NGE_BANKED
{
    uint16_t i;

    check_entity(id, use);
    nge_util_assert(ATTACK_HAS(id, use));

    /* One offset into each [use][entity] table, read as bytes: [use][id] six times costs zsdcc about 250 bytes */
    i = (uint16_t)(use == ATTACK_MELEE ? 0 : NGE_ENTITY_MAX) + id;
    out->to_hit = (int8_t)((const uint8_t *)attack_to_hit)[i];
    out->dice = ((const uint8_t *)attack_dice)[i];
    out->sides = ((const uint8_t *)attack_sides)[i];
    out->to_damage = (int8_t)((const uint8_t *)attack_to_damage)[i];
    out->type = ((const uint8_t *)attack_type)[i];
    out->range = ((const uint8_t *)attack_range)[i];
}

uint8_t attack_hits(nge_entity_id_t id, uint8_t use, nge_entity_id_t target, uint8_t roll, int8_t to_hit) NGE_BANKED
{
    check_entity(id, use);
    nge_util_assert(roll >= 1 && roll <= 20);
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(target, COMPONENT_HEALTH));

    if (roll == ATTACK_ROLL_HIT) {
        return 1;
    }
    if (roll == ATTACK_ROLL_MISS) {
        return 0;
    }
    return (int16_t)roll + attack_to_hit[use][id] + to_hit >= (int16_t)health_ac[target];
}

uint8_t attack_damage(nge_entity_id_t id, uint8_t use, int8_t to_damage) NGE_BANKED
{
    int16_t damage;
    uint8_t i;

    check_entity(id, use);

    damage = (int16_t)attack_to_damage[use][id] + to_damage;
    for (i = 0; i < attack_dice[use][id]; i++) {
        damage += 1 + (int16_t)((unsigned int)rand() % attack_sides[use][id]);
    }
    if (damage < 1) {
        return 1;
    }
    return (damage > 255) ? 255 : (uint8_t)damage;
}

int8_t attack_actor_mod(nge_entity_id_t actor, uint8_t use) NGE_BANKED
{
    nge_util_assert(actor < NGE_ENTITY_MAX);
    nge_util_assert(use < ATTACK_USE_COUNT);

    if (use != ATTACK_MELEE || !NGE_ENTITY_HAS_COMPONENT(actor, COMPONENT_STATS)) {
        return 0;
    }
    return STATS_MOD(stats_ability[ABIL_STR][actor]);
}

nge_entity_id_t attack_weapon(nge_entity_id_t id) NGE_BANKED
{
    nge_entity_id_t weapon;

    nge_util_assert(id < NGE_ENTITY_MAX);

    if (!INV_HAS(id)) {
        return NGE_ENTITY_INVALID;
    }
    weapon = inv_equipped(id, SLOT_WEAPON);
    if (weapon != NGE_ENTITY_INVALID && !ATTACK_HAS(weapon, ATTACK_MELEE)) {
        return NGE_ENTITY_INVALID;   /* wielded, but not a melee weapon */
    }
    return weapon;
}
