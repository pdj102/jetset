/* nge-dataseg: PAGE_123 */
/*
 * The feature kinds: one record per kind (gen_kinds.h), constant data in
 * page 123 with their names, read by the feature generator in page 122.
 * See gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_123
#endif

#include "game/gen/gen_internal.h"

#include "data/tile_data.h"

const gen_feature_kind_t gen_features[FT_COUNT] = {
    [FT_DOOR] = {
        .name = "door",
        .info = {
            .look = { { LOOK_DOOR_SHUT, LOOK_DOOR_SHUT_BANK }, { LOOK_DOOR_OPEN, LOOK_DOOR_OPEN_BANK } },
            .can = FCAN_OPEN,
            .block_shut = NGE_BLOCK_SOLID,   /* walkers and creatures; not ghosts */
        },
        .state = 0,                          /* closed (the level generator opens some) */
        .where = { .depth = 0, .rarity = 0 },   /* placed in doorways, never at random */
    },
    [FT_STAIRS_UP] = {
        .name = "staircase up",
        .info = {
            .look = { { LOOK_STAIRS_UP, LOOK_STAIRS_UP_BANK }, { LOOK_STAIRS_UP, LOOK_STAIRS_UP_BANK } },
            .can = FCAN_ASCEND,
            .block_shut = NGE_BLOCK_NONE,    /* stood on to climb: never in the way */
        },
        .state = 0,
        .where = { .depth = 0, .rarity = 0 },   /* placed by the level generator, never at random */
    },
    [FT_STAIRS_DOWN] = {
        .name = "staircase down",
        .info = {
            .look = { { LOOK_STAIRS_DOWN, LOOK_STAIRS_DOWN_BANK }, { LOOK_STAIRS_DOWN, LOOK_STAIRS_DOWN_BANK } },
            .can = FCAN_DESCEND,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = 0,
        .where = { .depth = 0, .rarity = 0 },
    },
};
