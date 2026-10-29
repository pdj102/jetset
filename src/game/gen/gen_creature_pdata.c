/* nge-dataseg: PAGE_107 */
/*
 * The creature kinds: one record per kind (gen_kinds.h), constant data in
 * page 107 with their names, read by the creature generator in page 106.
 * See gen_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_107
#endif

#include "game/gen/gen_internal.h"

#include "data/sprite_data.h"
#include "game/actors.h"
#include "game/dev.h"
#include "sprite/nge_sprite.h"

/* Development: the monsters act half as often, to watch the AI (game/dev.h) */
#if GAME_DEV_KIT
#define MONSTER_SPEED  20
#else
#define MONSTER_SPEED  10
#endif

const gen_creature_kind_t gen_creatures[CR_COUNT] = {
    [CR_PLAYER] = {
        .name = "you",
        .render = { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_PLAYER), LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10, .ability = { 16, 14, 14, 10, 10, 10 } },   /* Str Dex Con Int Wis Cha */
        .health = { .hp_max = 12, .ac = 10 },
        .mana = { .mp_max = 5 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .hooks = { [GEN_HOOK_INNATE] = EF_REGEN },
        .seqs = {
            [GEN_SEQ_ATTACK] = { SPRITE_PLAYER_ATTACK, SPRITE_PLAYER_ATTACK_COUNT, SPRITE_PLAYER_ATTACK_RATE },
        },
        .where = { .depth = 0, .rarity = 0 },          /* never at random */
    },
    [CR_RAT] = {
        .name = "rat",
        .render = { SPRITE_RAT_IDLE, SPRITE_RAT_IDLE_COUNT, SPRITE_RAT_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_EARTH), LAYER_MONSTERS },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 2, 11, 9, 2, 10, 4 } },
        .health = { .hp_max = 3, .immune = DMG_BIT(DMG_POISON), .ac = 12 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 3, .bonus = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },     /* carries nothing */
        .xp = 5,                                        /* below 1 hit die */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = EF_POISON },        /* a poisonous bite */
        .where = { .depth = 1, .rarity = 10 },
    },
    [CR_GOBLIN] = {
        .name = "goblin",
        .render = { SPRITE_GOBLIN_IDLE, SPRITE_GOBLIN_IDLE_COUNT, SPRITE_GOBLIN_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_GREEN), LAYER_MONSTERS },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 14, 10, 10, 8, 8 } },
        .health = { .hp_max = 6, .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE), .ac = 13 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 4, .bonus = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 5,                                        /* 1-1 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .rarity = 6 },
    },
};
