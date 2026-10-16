/*
 * The stats component's functions: banked in page 110 with the shared
 * generator helpers; its tables are in slot 0 (stats_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/stats.h"

#include <stdlib.h>

#include "core/nge_util.h"

static void check_entity(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

void stats_add(nge_entity_id_t id, const stats_base_t *b) NGE_BANKED
{
    uint16_t hp = b->hp_bonus;
    uint8_t i;

    check_entity(id);
    nge_util_assert(b->hp_dice == 0 || b->hp_sides != 0);
    nge_util_assert(b->speed != 0);

    for (i = 0; i < b->hp_dice; i++) {
        hp += 1 + (unsigned int)rand() % b->hp_sides;
    }
    if (hp > 255) {
        hp = 255;
    }
    stats_hp[id] = (uint8_t)hp;
    stats_hp_max[id] = (uint8_t)hp;
    stats_speed[id] = b->speed;
    nge_entity_components[id] |= COMPONENT_STATS;
}

void stats_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id);

    nge_entity_components[id] &= (nge_component_mask_t)~COMPONENT_STATS;
}
