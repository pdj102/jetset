/* nge-dataseg: PAGE_107 */
/*
 * The base creatures: one record per kind (gen_kinds.h), constant data in
 * page 107 with their names, read by the creature generator in page 106.
 * See gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_107
#endif

#include "game/gen/gen_internal.h"

#include "data/sprite_data.h"
#include "game/actors.h"
#include "sprite/nge_sprite.h"

#define PLAYER_FRAME_RATE  25   /* frames per animation step: two steps a second */

const gen_creature_base_t gen_creatures[CR_COUNT] = {
    [CR_PLAYER] = {
        .name = "you",
        .render = { DEMO_SPRITE_PLAYER, 2, PLAYER_FRAME_RATE, 0, LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10 },
        .health = { .hp_dice = 0, .hp_sides = 0, .hp_bonus = 12 },
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .flags = CREATURE_PLAYER,
        .where = { .depth = 0, .rarity = 0 },          /* never at random */
    },
    [CR_RAT] = {
        .name = "rat",
        .render = { DEMO_SPRITE_RAT, 1, 0, 0, LAYER_MONSTERS },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10 },
        .health = { .hp_dice = 1, .hp_sides = 4, .hp_bonus = 0, .immune = DMG_BIT(DMG_POISON) },
        .inventory = { .capacity = 0, .slots = 0 },     /* carries nothing */
        .flags = 0,
        .where = { .depth = 1, .rarity = 10 },
    },
    [CR_GOBLIN] = {
        .name = "goblin",
        /* The player's figure in the recoloured palette */
        .render = { DEMO_SPRITE_PLAYER, 2, PLAYER_FRAME_RATE, NGE_SPRITE_PALETTE(1), LAYER_MONSTERS },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10 },
        .health = { .hp_dice = 2, .hp_sides = 4, .hp_bonus = 1,
                    .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE) },
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) },
        .flags = 0,
        .where = { .depth = 1, .rarity = 6 },
    },
};
