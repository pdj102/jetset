/* nge-dataseg: PAGE_109 */
/*
 * The item kinds: one record per kind (gen_kinds.h), constant data in page
 * 109 with their names, read by the item generator in page 108. See
 * gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_109
#endif

#include "game/gen/gen_internal.h"

#include "data/sprite_data.h"
#include "game/actors.h"
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

const gen_item_kind_t gen_items[IT_COUNT] = {
    [IT_POTION] = {
        .name = "healing potion",
        .render = { SPRITE_POTION, 1, 0, NGE_SPRITE_PALETTE(BANK_RED), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = { EF_HEALING, 6 } },
        .where = { .depth = 1, .rarity = 10 },
    },
    [IT_SWORD] = {
        .name = "sword",
        .render = { SPRITE_SWORD, 1, 0, NGE_SPRITE_PALETTE(BANK_IRON), LAYER_OBJECTS },
        .subclass = ITEM_WEAPON,
        .fits = SLOT_BIT(SLOT_WEAPON),
        .melee = { .to_hit = 1, .dice = 1, .sides = 8, .bonus = 0, .type = DMG_SLASH, .range = 1 },
        .where = { .depth = 1, .rarity = 5 },
    },
    [IT_RING] = {
        .name = "ring of health",
        .render = { SPRITE_RING, 1, 0, NGE_SPRITE_PALETTE(BANK_GOLD), LAYER_OBJECTS },
        .subclass = ITEM_RING,
        .fits = SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R),
        .hooks = { [GEN_HOOK_WORN] = { EF_RING_HEALTH, 2 } },
        .where = { .depth = 1, .rarity = 3 },
    },
    /* Until they have sprites of their own: the potion recoloured, the rat upside down, the potion on its side or mirrored */
    [IT_ANTIDOTE] = {
        .name = "antidote",
        .render = { SPRITE_POTION, 1, 0, NGE_SPRITE_PALETTE(BANK_GREEN), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = { EF_CURE, 1 } },
        .where = { .depth = 1, .rarity = 4 },
    },
    [IT_FOOD] = {
        .name = "dead rat",
        .render = { SPRITE_RAT_DEAD, 1, 0, NGE_SPRITE_PALETTE(BANK_EARTH), LAYER_OBJECTS },
        .subclass = ITEM_FOOD,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = { EF_POISON, 1 } },
        .where = { .depth = 1, .rarity = 4 },
    },
    [IT_ELIXIR] = {
        .name = "elixir",
        .render = { SPRITE_POTION, 1, 0, NGE_SPRITE_PALETTE(BANK_GOLD), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = { EF_VITALITY, 1 } },
        .where = { .depth = 1, .rarity = 1 },
    },
    [IT_STRENGTH] = {
        .name = "potion of strength",
        .render = { SPRITE_POTION, 1, 0, NGE_SPRITE_PALETTE(BANK_ORANGE), LAYER_OBJECTS },
        .subclass = ITEM_POTION,
        .fits = 0,
        .hooks = { [GEN_HOOK_USE] = { EF_STRENGTH, 1 } },
        .where = { .depth = 1, .rarity = 1 },
    },
    /* The quest item (docs/winning.md): the ring's sprite until it has its own. Rarity 0: never at random */
    [IT_QUEST] = {
        .name = "crown",
        .render = { SPRITE_RING, 1, 0, NGE_SPRITE_PALETTE(BANK_PURPLE), LAYER_OBJECTS },
        .subclass = ITEM_QUEST,
        .fits = 0,
        .where = { .depth = LEVEL_DEPTH_BOTTOM, .rarity = 0 },
    },
};
