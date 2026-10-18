/* nge-dataseg: PAGE_123 */
/*
 * The base features: one record per kind (gen_kinds.h), constant data in
 * page 123 with their names, read by the feature generator in page 122.
 * See gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_123
#endif

#include "game/gen/gen_internal.h"

#include "game/level/level.h"
#include "io/nge_tm_stream.h"

const gen_feature_base_t gen_features[FT_COUNT] = {
    [FT_DOOR] = {
        .name = "door",
        .info = {
            .look = { { LEVEL_TILE_DOOR_SHUT, NGE_INK_RED }, { LEVEL_TILE_DOOR_OPEN, NGE_INK_RED } },
            .can = FCAN_OPEN,
            .block_shut = NGE_BLOCK_SOLID,   /* walkers and creatures; not ghosts */
        },
        .state = 0,                          /* closed (the level generator opens some) */
        .where = { .depth = 0, .rarity = 0 },   /* placed in doorways, never at random */
    },
    [FT_STAIRS_UP] = {
        .name = "staircase up",
        .info = {
            .look = { { LEVEL_TILE_STAIRS_UP, NGE_INK_WHITE }, { LEVEL_TILE_STAIRS_UP, NGE_INK_WHITE } },
            .can = FCAN_ASCEND,
            .block_shut = NGE_BLOCK_NONE,    /* stood on to climb: never in the way */
        },
        .state = 0,
        .where = { .depth = 0, .rarity = 0 },   /* placed by the level generator, never at random */
    },
    [FT_STAIRS_DOWN] = {
        .name = "staircase down",
        .info = {
            .look = { { LEVEL_TILE_STAIRS_DOWN, NGE_INK_WHITE }, { LEVEL_TILE_STAIRS_DOWN, NGE_INK_WHITE } },
            .can = FCAN_DESCEND,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = 0,
        .where = { .depth = 0, .rarity = 0 },
    },
};
