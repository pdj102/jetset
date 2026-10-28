/*
 * The stats component's functions: banked in page 110 with the shared
 * generator helpers; its tables are in slot 0 (stats_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/stats.h"

#include "core/nge_util.h"
#include "game/components/attrs.h"

static void check_entity(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

void stats_add(nge_entity_id_t id, const stats_kind_t *k) NGE_BANKED
{
    uint8_t a;
    uint8_t score;

    check_entity(id);
    nge_util_assert(k->speed != 0);

    stats_speed[id] = k->speed;
    attr_base[ATTR_SPEED][id] = k->speed;
    for (a = 0; a < ABIL_COUNT; a++) {
        score = k->ability[a];
        nge_util_assert(score >= STATS_ABILITY_MIN && score <= STATS_ABILITY_MAX);
        stats_ability[a][id] = score;
        attr_base[ATTR_ABILITY(a)][id] = score;
    }
    nge_entity_components[id] |= COMPONENT_STATS;
}

void stats_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id);

    nge_entity_components[id] &= (nge_component_mask_t)~COMPONENT_STATS;
}
