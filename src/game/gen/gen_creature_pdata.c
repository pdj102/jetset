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
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

/* Development: the monsters act half as often, to watch the AI (game/dev.h) */
#if GAME_DEV_KIT
#define MONSTER_SPEED  20
#define BANDIT_STATE   AI_STATE_IDLE   /* awake, so depth 1's bandit arms itself (level_gen.c) */
#else
#define MONSTER_SPEED  10
#define BANDIT_STATE   AI_STATE_ASLEEP
#endif
#define MONSTER_FAST   (MONSTER_SPEED / 2)
#define MONSTER_SLOW   (MONSTER_SPEED * 2)

/* What undead and spirits share: no poison in them; a spirit shrugs off weapons */
#define IMMUNE_UNDEAD   DMG_BIT(DMG_POISON)
#define RESIST_WEAPONS  (DMG_BIT(DMG_BLUNT) | DMG_BIT(DMG_SLASH) | DMG_BIT(DMG_PIERCE))

/* What a humanoid can wield and wear */
#define SLOTS_FIGHTER  (SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_SHIELD) | SLOT_BIT(SLOT_HEAD) | SLOT_BIT(SLOT_BODY))

/* A kind's sprite part from its shape, in a bank */
#define SHAPE_RAT(bank)     { SPRITE_RAT_IDLE, SPRITE_RAT_IDLE_COUNT, SPRITE_RAT_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_GOBLIN(bank)  { SPRITE_GOBLIN_IDLE, SPRITE_GOBLIN_IDLE_COUNT, SPRITE_GOBLIN_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_PLAYER(bank)  { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_COINS(bank)   { SPRITE_COINS, SPRITE_COINS_COUNT, SPRITE_COINS_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_BAT(bank)     { SPRITE_BAT_IDLE, SPRITE_BAT_IDLE_COUNT, SPRITE_BAT_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_SNAKE(bank)   { SPRITE_SNAKE_IDLE, SPRITE_SNAKE_IDLE_COUNT, SPRITE_SNAKE_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_LICHEN(bank)  { SPRITE_LICHEN_IDLE, SPRITE_LICHEN_IDLE_COUNT, SPRITE_LICHEN_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_SHRIEKER(bank) { SPRITE_SHRIEKER_IDLE, SPRITE_SHRIEKER_IDLE_COUNT, SPRITE_SHRIEKER_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_SLIME(bank)   { SPRITE_SLIME_IDLE, SPRITE_SLIME_IDLE_COUNT, SPRITE_SLIME_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_SPIDER(bank)  { SPRITE_SPIDER_IDLE, SPRITE_SPIDER_IDLE_COUNT, SPRITE_SPIDER_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_EYE(bank)     { SPRITE_EYE_IDLE, SPRITE_EYE_IDLE_COUNT, SPRITE_EYE_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_SKELETON(bank) { SPRITE_SKELETON_IDLE, SPRITE_SKELETON_IDLE_COUNT, SPRITE_SKELETON_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_GHOST(bank)   { SPRITE_GHOST_IDLE, SPRITE_GHOST_IDLE_COUNT, SPRITE_GHOST_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_OGRE(bank)    { SPRITE_OGRE_IDLE, SPRITE_OGRE_IDLE_COUNT, SPRITE_OGRE_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_DRAGON(bank)  { SPRITE_DRAGON_IDLE, SPRITE_DRAGON_IDLE_COUNT, SPRITE_DRAGON_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }

const gen_creature_kind_t gen_creatures[CR_COUNT] = {
    [CR_PLAYER] = {                                     /* the Dev class (game/chargen): medium abilities */
        .name = "you",
        .render = { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_PLAYER), LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10, .ability = { 12, 12, 12, 12, 12, 12 } },   /* Str Dex Con Int Wis Cha */
        .health = { .hp_max = 12, .ac = 10 },
        .mana = { .mp_max = 5 },
        .spells = { SP_MAGIC_BOLT },                    /* every spell learned at level 1: level 1 is never gained */
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .hooks = { [GEN_HOOK_INNATE] = { EF_REGEN, 1 } },
        .seqs = {
            [GEN_SEQ_ATTACK] = { SPRITE_PLAYER_ATTACK, SPRITE_PLAYER_ATTACK_COUNT, SPRITE_PLAYER_ATTACK_RATE },
        },
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },          /* never at random */
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
        .hooks = { [GEN_HOOK_HIT] = { EF_POISON, 1 } },        /* a poisonous bite */
        .where = { .depth = 1, .depth_max = 4, .rarity = 10 },
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
        .where = { .depth = 1, .depth_max = 6, .rarity = 6 },
    },
    [CR_BAT] = {                                        /* later: erratic flight */
        .name = "bat",
        .render = SHAPE_BAT(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_FAST, .ability = { 4, 15, 8, 2, 12, 6 } },
        .health = { .hp_max = 3, .ac = 13 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .bonus = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 5,                                        /* below 1 hit die */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 4, .rarity = 8 },
    },
    [CR_GREEN_SNAKE] = {
        .name = "green snake",
        .render = SHAPE_SNAKE(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 6, 14, 10, 1, 10, 3 } },
        .health = { .hp_max = 5, .immune = DMG_BIT(DMG_POISON), .ac = 13 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 3, .bonus = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = { EF_POISON, 1 } },
        .where = { .depth = 1, .depth_max = 5, .rarity = 7 },
    },
    [CR_LICHEN] = {
        .name = "lichen",
        .render = SHAPE_LICHEN(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 2, 2, 10, 1, 6, 1 } },
        .health = { .hp_max = 4, .immune = DMG_BIT(DMG_POISON), .vuln = DMG_BIT(DMG_FIRE), .ac = 9 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 1, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* a sticky touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = { EF_HOLD, 1 }, [GEN_HOOK_INNATE] = { EF_ROOT, 1 } },
        .where = { .depth = 1, .depth_max = 5, .rarity = 5 },
    },
    [CR_GREEN_SLIME] = {                                /* later: splits, corrodes gear */
        .name = "green slime",
        .render = SHAPE_SLIME(BANK_LIME),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 4, 4, 14, 1, 4, 1 } },
        .health = { .hp_max = 9, .immune = DMG_BIT(DMG_POISON) | DMG_BIT(DMG_ACID),
                    .resist = DMG_BIT(DMG_BLUNT) | DMG_BIT(DMG_PIERCE), .vuln = DMG_BIT(DMG_FIRE), .ac = 8 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 4, .bonus = 0, .type = DMG_ACID, .range = 1 },    /* a burning touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 2, .depth_max = 6, .rarity = 6 },
    },
    [CR_SHRIEKER] = {                                   /* later: its shriek wakes the level */
        .name = "shrieker",
        .render = SHAPE_SHRIEKER(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 1, 1, 8, 1, 8, 1 } },
        .health = { .hp_max = 5, .immune = DMG_BIT(DMG_POISON), .vuln = DMG_BIT(DMG_FIRE), .ac = 7 },
        .inventory = { .capacity = 0, .slots = 0 },     /* no attack */
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_INNATE] = { EF_ROOT, 1 } },
        .where = { .depth = 2, .depth_max = 6, .rarity = 4 },
    },
    [CR_BANDIT] = {                                     /* later: steals and runs */
        .name = "bandit",
        .render = SHAPE_PLAYER(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 11, 13, 11, 10, 9, 8 } },
        .health = { .hp_max = 9, .ac = 12 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 4, .bonus = 0, .type = DMG_SLASH, .range = 1 },   /* a knife */
        .inventory = { .capacity = 6, .slots = SLOTS_HUMANOID },
        .xp = 20,                                       /* 2 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = BANDIT_STATE },
        .where = { .depth = 2, .depth_max = 6, .rarity = 5 },
    },
    [CR_ORC] = {
        .name = "orc",
        .render = SHAPE_GOBLIN(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 13, 10, 12, 8, 8, 7 } },
        .health = { .hp_max = 10, .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .bonus = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 4, .slots = SLOTS_FIGHTER },
        .xp = 20,                                       /* 2 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 7, .rarity = 8 },
    },
    [CR_GIANT_SPIDER] = {                               /* later: webs */
        .name = "giant spider",
        .render = SHAPE_SPIDER(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 15, 10, 1, 10, 2 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 14 },
        .melee = { .to_hit = 3, .dice = 1, .sides = 6, .bonus = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = { EF_POISON, 2 } },
        .where = { .depth = 3, .depth_max = 7, .rarity = 6 },
    },
    [CR_SKELETON] = {                                   /* later: never flees */
        .name = "skeleton",
        .render = SHAPE_SKELETON(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 12, 10, 3, 8, 4 } },
        .health = { .hp_max = 9, .immune = IMMUNE_UNDEAD, .resist = DMG_BIT(DMG_PIERCE), .vuln = DMG_BIT(DMG_BLUNT), .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .bonus = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 2, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 20,                                       /* 2 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 7, .rarity = 7 },
    },
    [CR_GOBLIN_SHAMAN] = {
        .name = "goblin shaman",
        .render = SHAPE_GOBLIN(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 12, 10, 13, 14, 10 } },
        .health = { .hp_max = 9, .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE), .ac = 12 },
        .mana = { .mp_max = 6 },
        .spells = { SP_MAGIC_BOLT, SP_CURE_WOUNDS, SP_ENFEEBLE },
        .melee = { .to_hit = 1, .dice = 1, .sides = 4, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* a stick */
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 7, .rarity = 4 },
    },
    [CR_GOLD_SNAKE] = {                                 /* later: spits at range */
        .name = "gold snake",
        .render = SHAPE_SNAKE(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 16, 11, 1, 12, 4 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 14 },
        .melee = { .to_hit = 3, .dice = 1, .sides = 6, .bonus = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = { EF_BLIND, 1 } },
        .where = { .depth = 4, .depth_max = 8, .rarity = 5 },
    },
    [CR_FLOATING_EYE] = {                               /* later: a paralysing gaze */
        .name = "floating eye",
        .render = SHAPE_EYE(BANK_BLUE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 1, 3, 10, 8, 14, 1 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 9 },
        .inventory = { .capacity = 0, .slots = 0 },     /* no attack */
        .xp = 75,                                       /* 3+ hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_INNATE] = { EF_ROOT, 1 } },
        .where = { .depth = 4, .depth_max = 8, .rarity = 3 },
    },
    [CR_MIMIC] = {                                      /* later: poses as treasure until touched */
        .name = "mimic",
        .render = SHAPE_COINS(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 14, 8, 14, 3, 10, 1 } },
        .health = { .hp_max = 18, .immune = DMG_BIT(DMG_ACID), .ac = 12 },
        .melee = { .to_hit = 4, .dice = 2, .sides = 4, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* a slam */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 125,                                      /* 4 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_INNATE] = { EF_ROOT, 1 } },
        .where = { .depth = 4, .depth_max = 9, .rarity = 3 },
    },
    [CR_OGRE] = {
        .name = "ogre",
        .render = SHAPE_OGRE(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 17, 8, 15, 6, 8, 6 } },
        .health = { .hp_max = 23, .ac = 14 },
        .melee = { .to_hit = 5, .dice = 2, .sides = 6, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 3, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 175,                                      /* 5 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 5, .depth_max = 8, .rarity = 8 },
    },
    [CR_GHOST] = {                                      /* later: passes through walls */
        .name = "ghost",
        .render = SHAPE_GHOST(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* floats: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 2, 14, 10, 10, 12, 12 } },
        .health = { .hp_max = 20, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 15 },
        .melee = { .to_hit = 4, .dice = 1, .sides = 8, .bonus = 0, .type = DMG_COLD, .range = 1 },    /* a chill touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 225,                                      /* 5 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 5, .depth_max = 9, .rarity = 5 },
    },
    [CR_WRAITH] = {
        .name = "wraith",
        .render = SHAPE_GHOST(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 15, 10, 12, 13, 14 } },
        .health = { .hp_max = 27, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 16 },
        .melee = { .to_hit = 6, .dice = 1, .sides = 10, .bonus = 0, .type = DMG_COLD, .range = 1 },   /* a draining touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_HIT] = { EF_WEAKNESS, 2 } },
        .where = { .depth = 6, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 5 },
    },
    [CR_TROLL] = {
        .name = "troll",
        .render = SHAPE_OGRE(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 20, 12, 18, 7, 9, 6 } },
        .health = { .hp_max = 31, .vuln = DMG_BIT(DMG_FIRE) | DMG_BIT(DMG_ACID), .ac = 15 },
        .melee = { .to_hit = 7, .dice = 2, .sides = 6, .bonus = 2, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 850,                                      /* 7 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .hooks = { [GEN_HOOK_INNATE] = { EF_REGEN, 1 } },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 6 },
    },
    [CR_DRAGON] = {                                     /* later: fire breath */
        .name = "dragon",
        .render = SHAPE_DRAGON(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 22, 10, 20, 14, 14, 16 } },
        .health = { .hp_max = 60, .immune = DMG_BIT(DMG_FIRE), .vuln = DMG_BIT(DMG_COLD), .ac = 18 },
        .melee = { .to_hit = 9, .dice = 3, .sides = 6, .bonus = 3, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 1750,                                     /* 10 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 9, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 2 },
    },
    [CR_FIGHTER] = {                                    /* the Fighter class: no mana, no spells */
        .name = "you",
        .render = { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_PLAYER), LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10, .ability = { 16, 14, 14, 9, 9, 10 } },   /* Str Dex Con Int Wis Cha */
        .health = { .hp_max = 12, .ac = 10 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .hooks = { [GEN_HOOK_INNATE] = { EF_REGEN, 1 } },
        .seqs = {
            [GEN_SEQ_ATTACK] = { SPRITE_PLAYER_ATTACK, SPRITE_PLAYER_ATTACK_COUNT, SPRITE_PLAYER_ATTACK_RATE },
        },
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },          /* never at random */
    },
    [CR_MAGE] = {                                       /* the Mage class: frail, with mana */
        .name = "you",
        .render = { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_PLAYER), LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10, .ability = { 9, 10, 12, 16, 14, 10 } },   /* Str Dex Con Int Wis Cha */
        .health = { .hp_max = 8, .ac = 10 },
        .mana = { .mp_max = 8 },
        .spells = { SP_MAGIC_BOLT },                    /* every spell learned at level 1: level 1 is never gained */
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .bonus = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .hooks = { [GEN_HOOK_INNATE] = { EF_REGEN, 1 } },
        .seqs = {
            [GEN_SEQ_ATTACK] = { SPRITE_PLAYER_ATTACK, SPRITE_PLAYER_ATTACK_COUNT, SPRITE_PLAYER_ATTACK_RATE },
        },
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },          /* never at random */
    },
};
