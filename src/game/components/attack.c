/*
 * The attack component's functions: banked in page 110 with the other
 * component functions; its tables are in slot 0 (attack_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/attack.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "game/components/health.h"

static void check_entity(nge_entity_id_t id, uint8_t kind)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    (void)kind;
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(kind < ATTACK_KINDS);
}

void attack_add(nge_entity_id_t id, uint8_t kind, const attack_base_t *b) NGE_BANKED
{
    check_entity(id, kind);
    nge_util_assert(b->dice != 0 && b->sides != 0 && b->type < DMG_COUNT);
    nge_util_assert(kind != ATTACK_MELEE || b->range == 1);

    attack_to_hit[kind][id] = b->to_hit;
    attack_dice[kind][id] = b->dice;
    attack_sides[kind][id] = b->sides;
    attack_bonus[kind][id] = b->bonus;
    attack_type[kind][id] = b->type;
    attack_range[kind][id] = b->range;
    nge_entity_components[id] |= ATTACK_COMPONENT(kind);
}

void attack_remove(nge_entity_id_t id, uint8_t kind) NGE_BANKED
{
    check_entity(id, kind);

    nge_entity_components[id] &= (nge_component_mask_t)~ATTACK_COMPONENT(kind);
}

uint8_t attack_hits(nge_entity_id_t id, uint8_t kind, nge_entity_id_t target, uint8_t roll) NGE_BANKED
{
    check_entity(id, kind);
    nge_util_assert(roll >= 1 && roll <= 20);
    nge_util_assert(NGE_ENTITY_HAS_ANY_COMPONENTS(target, COMPONENT_HEALTH));

    if (roll == ATTACK_ROLL_HIT) {
        return 1;
    }
    if (roll == ATTACK_ROLL_MISS) {
        return 0;
    }
    return (int16_t)roll + attack_to_hit[kind][id] >= (int16_t)health_ac[target];
}

uint8_t attack_damage(nge_entity_id_t id, uint8_t kind) NGE_BANKED
{
    int16_t damage;
    uint8_t i;

    check_entity(id, kind);

    damage = attack_bonus[kind][id];
    for (i = 0; i < attack_dice[kind][id]; i++) {
        damage += 1 + (int16_t)((unsigned int)rand() % attack_sides[kind][id]);
    }
    if (damage < 1) {
        return 1;
    }
    return (damage > 255) ? 255 : (uint8_t)damage;
}
