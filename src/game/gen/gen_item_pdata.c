/* nge-dataseg: PAGE_109 */
/*
 * The base items: one record per kind (gen_kinds.h), constant data in page
 * 109 with their names, read by the item generator in page 108. See
 * gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_109
#endif

#include "game/gen/gen_internal.h"

#include "data/sprite_data.h"
#include "game/actors.h"

const gen_item_base_t gen_items[IT_COUNT] = {
    [IT_POTION] = {
        .name = "potion",
        .render = { DEMO_SPRITE_POTION, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .where = { .depth = 1, .rarity = 10 },
    },
    [IT_SWORD] = {
        .name = "sword",
        .render = { DEMO_SPRITE_SWORD, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_WEAPON,
        .fits = SLOT_BIT(SLOT_WEAPON),
        .where = { .depth = 1, .rarity = 5 },
    },
    [IT_RING] = {
        .name = "ring",
        .render = { DEMO_SPRITE_RING, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_RING,
        .fits = SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R),
        .where = { .depth = 1, .rarity = 3 },
    },
};
