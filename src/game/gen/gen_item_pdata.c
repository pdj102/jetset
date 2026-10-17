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
#include "sprite/nge_sprite.h"

const gen_item_base_t gen_items[IT_COUNT] = {
    [IT_POTION] = {
        .name = "healing potion",
        .render = { DEMO_SPRITE_POTION, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = EF_HEALING },
        .where = { .depth = 1, .rarity = 10 },
    },
    [IT_SWORD] = {
        .name = "sword",
        .render = { DEMO_SPRITE_SWORD, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_WEAPON,
        .fits = SLOT_BIT(SLOT_WEAPON),
        .melee = { .to_hit = 1, .dice = 1, .sides = 8, .bonus = 0, .type = DMG_SLASH, .range = 1 },
        .where = { .depth = 1, .rarity = 5 },
    },
    [IT_RING] = {
        .name = "ring of health",
        .render = { DEMO_SPRITE_RING, 1, 0, 0, LAYER_OBJECTS },
        .subclass = ITEM_RING,
        .fits = SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R),
        .hooks = { [GEN_HOOK_WORN] = EF_RING_HEALTH },
        .where = { .depth = 1, .rarity = 3 },
    },
    /* Until they have sprites of their own: the potion recoloured, the rat upside down, the potion on its side or mirrored */
    [IT_ANTIDOTE] = {
        .name = "antidote",
        .render = { DEMO_SPRITE_POTION, 1, 0, NGE_SPRITE_PALETTE(1), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = EF_CURE },
        .where = { .depth = 1, .rarity = 4 },
    },
    [IT_FOOD] = {
        .name = "dead rat",
        .render = { DEMO_SPRITE_RAT, 1, 0, NGE_SPRITE_MIRROR_Y, LAYER_OBJECTS },
        .subclass = ITEM_FOOD,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = EF_POISON },
        .where = { .depth = 1, .rarity = 4 },
    },
    [IT_ELIXIR] = {
        .name = "elixir",
        .render = { DEMO_SPRITE_POTION, 1, 0, NGE_SPRITE_ROTATE | NGE_SPRITE_PALETTE(1), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = EF_VITALITY },
        .where = { .depth = 1, .rarity = 1 },
    },
    [IT_STRENGTH] = {
        .name = "potion of strength",
        .render = { DEMO_SPRITE_POTION, 1, 0, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(1), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = EF_STRENGTH },
        .where = { .depth = 1, .rarity = 1 },
    },
};
