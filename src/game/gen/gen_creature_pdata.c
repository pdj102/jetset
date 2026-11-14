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
#define MONSTER_QUICK  (MONSTER_SPEED * 4 / 5)   /* speed 8: the warg, the minotaur */

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
#define SHAPE_KOBOLD(bank)  { SPRITE_KOBOLD_IDLE, SPRITE_KOBOLD_IDLE_COUNT, SPRITE_KOBOLD_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_DOG(bank)     { SPRITE_DOG_IDLE, SPRITE_DOG_IDLE_COUNT, SPRITE_DOG_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_BUG(bank)     { SPRITE_BUG_IDLE, SPRITE_BUG_IDLE_COUNT, SPRITE_BUG_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_ZOMBIE(bank)  { SPRITE_ZOMBIE_IDLE, SPRITE_ZOMBIE_IDLE_COUNT, SPRITE_ZOMBIE_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_ROBE(bank)    { SPRITE_ROBE_IDLE, SPRITE_ROBE_IDLE_COUNT, SPRITE_ROBE_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
#define SHAPE_LIZARD(bank)  { SPRITE_LIZARD_IDLE, SPRITE_LIZARD_IDLE_COUNT, SPRITE_LIZARD_IDLE_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }
/* The will-o'-wisp: the VFX's orb, one still frame (docs/creature_extensions.md 6.2) */
#define SHAPE_ORB(bank)     { SPRITE_FX_ORB, SPRITE_FX_ORB_COUNT, SPRITE_FX_ORB_RATE, \
                              NGE_SPRITE_PALETTE(bank), LAYER_MONSTERS }

const gen_creature_kind_t gen_creatures[CR_COUNT] = {
    [CR_NONE] = { 0 },                                  /* none: rarity 0, never generated */
    [CR_PLAYER] = {                                     /* the Dev class (game/chargen): medium abilities */
        .name = "you",
        .render = { SPRITE_PLAYER_IDLE, SPRITE_PLAYER_IDLE_COUNT, SPRITE_PLAYER_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_PLAYER), LAYER_PLAYER },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = 10, .ability = { 12, 12, 12, 12, 12, 12 } },   /* Str Dex Con Int Wis Cha */
        .health = { .hp_max = 12, .ac = 10 },
        .mana = { .mp_max = 5 },
        .spells = { SP_MAGIC_BOLT },                    /* the starting spells: within level 1's slots (docs/spells.md, R38) */
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .innate = { EF_REGEN, 1 },
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
        .melee = { .to_hit = 0, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .melee_hit = { EF_POISON, 1 },                  /* a poisonous bite */
        .inventory = { .capacity = 0, .slots = 0 },     /* carries nothing */
        .xp = 5,                                        /* below 1 hit die */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 2, .rarity = 10 },
    },
    [CR_GOBLIN] = {
        .name = "goblin",
        .render = { SPRITE_GOBLIN_IDLE, SPRITE_GOBLIN_IDLE_COUNT, SPRITE_GOBLIN_IDLE_RATE,
                    NGE_SPRITE_PALETTE(BANK_GREEN), LAYER_MONSTERS },
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 14, 10, 10, 8, 8 } },
        .health = { .hp_max = 6, .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE), .ac = 13 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 5,                                        /* 1-1 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 6 },
    },
    [CR_BAT] = {
        .name = "bat",
        .render = SHAPE_BAT(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_FAST, .ability = { 4, 15, 8, 2, 12, 6 } },
        .health = { .hp_max = 3, .ac = 13 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 5,                                        /* below 1 hit die */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ERRATIC, 1 },                    /* erratic flight */
        .where = { .depth = 1, .depth_max = 3, .rarity = 8 },
    },
    [CR_GREEN_SNAKE] = {
        .name = "green snake",
        .render = SHAPE_SNAKE(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 6, 14, 10, 1, 10, 3 } },
        .health = { .hp_max = 5, .immune = DMG_BIT(DMG_POISON), .ac = 13 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .melee_hit = { EF_POISON, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 7 },
    },
    [CR_LICHEN] = {
        .name = "lichen",
        .render = SHAPE_LICHEN(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 2, 2, 10, 1, 6, 1 } },
        .health = { .hp_max = 4, .immune = DMG_BIT(DMG_POISON), .vuln = DMG_BIT(DMG_FIRE), .ac = 9 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 1, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a sticky touch */
        .melee_hit = { EF_HOLD, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ROOT, 1 },
        .where = { .depth = 1, .depth_max = 3, .rarity = 5 },
    },
    [CR_GREEN_SLIME] = {                                /* later: splits, corrodes gear */
        .name = "green slime",
        .render = SHAPE_SLIME(BANK_LIME),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 4, 4, 14, 1, 4, 1 } },
        .health = { .hp_max = 9, .immune = DMG_BIT(DMG_POISON) | DMG_BIT(DMG_ACID),
                    .resist = DMG_BIT(DMG_BLUNT) | DMG_BIT(DMG_PIERCE), .vuln = DMG_BIT(DMG_FIRE), .ac = 8 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_ACID, .range = 1 },    /* a burning touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 2, .depth_max = 4, .rarity = 6 },
    },
    [CR_SHRIEKER] = {
        .name = "shrieker",
        .render = SHAPE_SHRIEKER(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 1, 1, 8, 1, 8, 1 } },
        .health = { .hp_max = 5, .immune = DMG_BIT(DMG_POISON), .vuln = DMG_BIT(DMG_FIRE), .ac = 7 },
        .mana = { .mp_max = 4 },                        /* two shrieks */
        .spells = { SP_SHRIEK },                        /* wakes what sleeps around it */
        .inventory = { .capacity = 0, .slots = 0 },     /* no attack */
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ROOT, 1 },
        .where = { .depth = 2, .depth_max = 5, .rarity = 4 },
    },
    [CR_BANDIT] = {                                     /* later: steals and runs */
        .name = "bandit",
        .render = SHAPE_PLAYER(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 11, 13, 11, 10, 9, 8 } },
        .health = { .hp_max = 9, .ac = 12 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* a knife */
        .inventory = { .capacity = 6, .slots = SLOTS_HUMANOID },
        .xp = 20,                                       /* 2 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = BANDIT_STATE },
        .where = { .depth = 2, .depth_max = 4, .rarity = 5 },
    },
    [CR_ORC] = {
        .name = "orc",
        .render = SHAPE_GOBLIN(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 13, 10, 12, 8, 8, 7 } },
        .health = { .hp_max = 10, .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 4, .slots = SLOTS_FIGHTER },
        .xp = 20,                                       /* 2 hit dice */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 8 },
    },
    [CR_GIANT_SPIDER] = {                               /* later: webs */
        .name = "giant spider",
        .render = SHAPE_SPIDER(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 15, 10, 1, 10, 2 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 14 },
        .melee = { .to_hit = 3, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .melee_hit = { EF_POISON, 2 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 6 },
    },
    [CR_SKELETON] = {                                   /* mindless: never flees */
        .name = "skeleton",
        .render = SHAPE_SKELETON(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 12, 10, 3, 8, 4 } },
        .health = { .hp_max = 9, .immune = IMMUNE_UNDEAD, .resist = DMG_BIT(DMG_PIERCE), .vuln = DMG_BIT(DMG_BLUNT), .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 2, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 20,                                       /* 2 hit dice */
        .flags = CREATURE_UNDEAD | CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 7 },
    },
    [CR_GOBLIN_SHAMAN] = {
        .name = "goblin shaman",
        .render = SHAPE_GOBLIN(BANK_PURPLE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 12, 10, 13, 14, 10 } },
        .health = { .hp_max = 9, .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE), .ac = 12 },
        .mana = { .mp_max = 6 },
        .spells = { SP_MAGIC_BOLT, SP_CURE_WOUNDS, SP_ENFEEBLE, SP_MEND },   /* Mend: its hurt allies (M2.2) */
        .melee = { .to_hit = 1, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a stick */
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 3, .depth_max = 5, .rarity = 4 },
    },
    [CR_GOLD_SNAKE] = {
        .name = "gold snake",
        .render = SHAPE_SNAKE(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 16, 11, 1, 12, 4 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 14 },
        .melee = { .to_hit = 3, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .ranged = { .to_hit = 3, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_POISON, .range = 3 }, /* spit */
        .ranged_hit = { EF_BLIND, 1 },                  /* in the eyes */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 4, .depth_max = 6, .rarity = 5 },
    },
    [CR_FLOATING_EYE] = {
        .name = "floating eye",
        .render = SHAPE_EYE(BANK_BLUE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 1, 3, 10, 8, 14, 1 } },
        .health = { .hp_max = 13, .immune = DMG_BIT(DMG_POISON), .ac = 9 },
        .inventory = { .capacity = 0, .slots = 0 },     /* no attack */
        .xp = 75,                                       /* 3+ hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ROOT, 1 },
        .struck = { EF_PARALYSE, 2 },                   /* a paralysing gaze: don't hit it */
        .where = { .depth = 4, .depth_max = 6, .rarity = 3 },
    },
    [CR_MIMIC] = {                                      /* later: poses as treasure until touched */
        .name = "mimic",
        .render = SHAPE_COINS(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 14, 8, 14, 3, 10, 1 } },
        .health = { .hp_max = 18, .immune = DMG_BIT(DMG_ACID), .ac = 12 },
        .melee = { .to_hit = 4, .dice = 2, .sides = 4, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a slam */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 125,                                      /* 4 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ROOT, 1 },
        .where = { .depth = 4, .depth_max = 7, .rarity = 3 },
    },
    [CR_OGRE] = {
        .name = "ogre",
        .render = SHAPE_OGRE(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 17, 8, 15, 6, 8, 6 } },
        .health = { .hp_max = 23, .ac = 14 },
        .melee = { .to_hit = 5, .dice = 2, .sides = 6, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 3, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 175,                                      /* 5 hit dice */
        .flags = CREATURE_GIANT,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 5, .depth_max = 7, .rarity = 8 },
    },
    [CR_GHOST] = {                                      /* later: passes through walls */
        .name = "ghost",
        .render = SHAPE_GHOST(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* floats: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 2, 14, 10, 10, 12, 12 } },
        .health = { .hp_max = 20, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 15 },
        .melee = { .to_hit = 4, .dice = 1, .sides = 8, .to_damage = 0, .type = DMG_COLD, .range = 1 },    /* a chill touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 225,                                      /* 5 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 5, .depth_max = 7, .rarity = 5 },
    },
    [CR_WRAITH] = {
        .name = "wraith",
        .render = SHAPE_GHOST(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 15, 10, 12, 13, 14 } },
        .health = { .hp_max = 27, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 16 },
        .melee = { .to_hit = 6, .dice = 1, .sides = 10, .to_damage = 0, .type = DMG_COLD, .range = 1 },   /* a draining touch */
        .melee_hit = { EF_WEAKNESS, 2 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 8, .rarity = 5 },
    },
    [CR_TROLL] = {
        .name = "troll",
        .render = SHAPE_OGRE(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 20, 12, 18, 7, 9, 6 } },
        .health = { .hp_max = 31, .vuln = DMG_BIT(DMG_FIRE) | DMG_BIT(DMG_ACID), .ac = 15 },
        .melee = { .to_hit = 7, .dice = 2, .sides = 6, .to_damage = 2, .type = DMG_SLASH, .range = 1 },   /* claws */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 850,                                      /* 7 hit dice, special */
        .flags = CREATURE_GIANT,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_REGEN, 1 },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 6 },
    },
    [CR_DRAGON] = {
        .name = "dragon",
        .render = SHAPE_DRAGON(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 22, 10, 20, 14, 14, 16 } },
        .health = { .hp_max = 60, .immune = DMG_BIT(DMG_FIRE), .vuln = DMG_BIT(DMG_COLD), .ac = 18 },   /* its own breath can't hurt it (R9) */
        .mana = { .mp_max = 12 },                       /* three breaths */
        .spells = { SP_FIRE_BREATH },
        .melee = { .to_hit = 9, .dice = 3, .sides = 6, .to_damage = 3, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 1750,                                     /* 10 hit dice, special */
        .flags = CREATURE_DRAGON,
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
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .innate = { EF_REGEN, 1 },
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
        .spells = { SP_MAGIC_BOLT },                    /* the starting spells: within level 1's slots (docs/spells.md, R38) */
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .inventory = { .capacity = 20, .slots = SLOTS_HUMANOID },
        .xp = 0,
        .flags = CREATURE_PLAYER,
        .ai = { .brain = AI_BRAIN_NONE },               /* their turns wait for commands */
        .innate = { EF_REGEN, 1 },
        .seqs = {
            [GEN_SEQ_ATTACK] = { SPRITE_PLAYER_ATTACK, SPRITE_PLAYER_ATTACK_COUNT, SPRITE_PLAYER_ATTACK_RATE },
        },
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },          /* never at random */
    },
    [CR_GOBLIN_ARCHER] = {                              /* the goblin war band's (docs/creature_extensions.md 6.1) */
        .name = "goblin archer",
        .render = SHAPE_GOBLIN(BANK_ORANGE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 8, 14, 10, 10, 8, 8 } },
        .health = { .hp_max = 5, .resist = DMG_BIT(DMG_BLUNT), .vuln = DMG_BIT(DMG_FIRE), .ac = 13 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* a knife */
        .kit = { IT_SHORT_BOW, IT_ARROWS },
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO) },
        .xp = 10,                                       /* 1-1 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 2, .depth_max = 4, .rarity = 5 },
    },
    /* ---- docs/creature_extensions.md 6.1: depths 1 to 6 (plan step 6) ---- */
    [CR_KOBOLD] = {                                     /* weak alone, dangerous in threes (later: bands) */
        .name = "kobold",
        .render = SHAPE_KOBOLD(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 7, 13, 9, 8, 7, 6 } },
        .health = { .hp_max = 3, .ac = 12 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* a spear */
        .inventory = { .capacity = 2, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 5,                                        /* below 1 hit die */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 8 },
    },
    [CR_KOBOLD_SLINGER] = {
        .name = "kobold slinger",
        .render = SHAPE_KOBOLD(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 6, 14, 9, 8, 8, 6 } },
        .health = { .hp_max = 3, .ac = 12 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a cudgel */
        .kit = { IT_SLING, IT_STONES },
        .inventory = { .capacity = 3, .slots = SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO) },
        .xp = 6,                                        /* below 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 1, .depth_max = 3, .rarity = 5 },
    },
    [CR_JACKAL] = {                                     /* later: packs (bands) */
        .name = "jackal",
        .render = SHAPE_DOG(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_FAST, .ability = { 6, 15, 9, 2, 12, 5 } },
        .health = { .hp_max = 3, .ac = 12 },
        .melee = { .to_hit = 0, .dice = 1, .sides = 2, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 5,                                        /* below 1 hit die */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 6 },
    },
    [CR_STIRGE] = {
        .name = "stirge",
        .render = SHAPE_BAT(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 4, 16, 9, 2, 10, 4 } },
        .health = { .hp_max = 4, .ac = 13 },
        .melee = { .to_hit = 1, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* a beak that drinks */
        .melee_hit = { EF_BLEEDING, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 13,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 5 },
    },
    [CR_FIRE_BEETLE] = {
        .name = "fire beetle",
        .render = SHAPE_BUG(BANK_ORANGE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 10, 12, 1, 8, 2 } },
        .health = { .hp_max = 5, .immune = DMG_BIT(DMG_FIRE), .ac = 14 },   /* a hard shell */
        .melee = { .to_hit = 1, .dice = 1, .sides = 4, .to_damage = 1, .type = DMG_SLASH, .range = 1 },   /* mandibles */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 10,                                       /* 1 hit die */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 1, .depth_max = 3, .rarity = 5 },
    },
    [CR_ZOMBIE] = {
        .name = "zombie",
        .render = SHAPE_ZOMBIE(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 13, 6, 14, 3, 6, 3 } },
        .health = { .hp_max = 11, .immune = IMMUNE_UNDEAD, .ac = 8 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 8, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a fist */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 20,                                       /* 2 hit dice */
        .flags = CREATURE_UNDEAD | CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 2, .depth_max = 4, .rarity = 6 },
    },
    [CR_GHOUL] = {
        .name = "ghoul",
        .render = SHAPE_ZOMBIE(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 11, 12, 11, 6, 9, 4 } },
        .health = { .hp_max = 11, .immune = IMMUNE_UNDEAD, .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 3, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .melee_hit = { EF_PARALYSE, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 5 },
    },
    [CR_WOLF] = {                                       /* later: packs (bands) */
        .name = "wolf",
        .render = SHAPE_DOG(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 12, 14, 12, 2, 12, 6 } },
        .health = { .hp_max = 11, .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 20,                                       /* 2 hit dice */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 6 },
    },
    [CR_BOMBARDIER] = {
        .name = "bombardier beetle",
        .render = SHAPE_BUG(BANK_GOLD),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 10, 13, 1, 8, 2 } },
        .health = { .hp_max = 9, .immune = DMG_BIT(DMG_ACID), .ac = 15 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* mandibles */
        .ranged = { .to_hit = 2, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_ACID, .range = 3 },   /* a spray */
        .ranged_hit = { EF_CORROSION, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 3, .depth_max = 5, .rarity = 4 },
    },
    [CR_HOBGOBLIN] = {                                  /* later: leads a troop (bands) */
        .name = "hobgoblin",
        .render = SHAPE_GOBLIN(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 13, 12, 12, 10, 9, 9 } },
        .health = { .hp_max = 11, .vuln = DMG_BIT(DMG_FIRE), .ac = 11 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .kit = { IT_SHORT_SWORD, IT_SHIELD },
        .inventory = { .capacity = 4, .slots = SLOTS_FIGHTER },   /* drinks potions: Int 10 */
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 4, .depth_max = 6, .rarity = 3 },
    },
    [CR_HARPY] = {
        .name = "harpy",
        .render = SHAPE_BAT(BANK_FLESH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 14, 11, 7, 10, 14 } },
        .health = { .hp_max = 13, .ac = 12 },
        .mana = { .mp_max = 6 },                        /* three songs */
        .spells = { SP_CONFUSE },                       /* its song */
        .melee = { .to_hit = 3, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* talons */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 4, .depth_max = 6, .rarity = 4 },
    },
    [CR_SHADOW] = {
        .name = "shadow",
        .render = SHAPE_GHOST(BANK_SHADOW),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 6, 14, 10, 3, 10, 6 } },
        .health = { .hp_max = 13, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 13 },
        .melee = { .to_hit = 3, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_COLD, .range = 1 },    /* a chill touch */
        .melee_hit = { EF_DRAIN_STR, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 50,                                       /* 3 hit dice, special */
        .flags = CREATURE_UNDEAD | CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 4, .depth_max = 6, .rarity = 4 },
    },
    [CR_ORC_ARCHER] = {                                 /* later: an orc war party (bands) */
        .name = "orc archer",
        .render = SHAPE_GOBLIN(BANK_EARTH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 12, 12, 12, 8, 8, 7 } },
        .health = { .hp_max = 9, .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* a knife */
        .kit = { IT_CROSSBOW, IT_BOLTS },
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO) },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 4, .depth_max = 6, .rarity = 5 },
    },
    [CR_SKELETON_ARCHER] = {
        .name = "skeleton archer",
        .render = SHAPE_SKELETON(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 13, 10, 3, 8, 4 } },
        .health = { .hp_max = 9, .immune = IMMUNE_UNDEAD, .resist = DMG_BIT(DMG_PIERCE), .vuln = DMG_BIT(DMG_BLUNT), .ac = 13 },
        .melee = { .to_hit = 2, .dice = 1, .sides = 4, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .kit = { IT_SHORT_BOW, IT_ARROWS },
        .inventory = { .capacity = 2, .slots = SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO) },
        .xp = 25,                                       /* 2 hit dice, special */
        .flags = CREATURE_UNDEAD | CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 4, .depth_max = 6, .rarity = 4 },
    },
    /* ---- docs/creature_extensions.md 6.2: depths 5 to 10 (plan step 7) ---- */
    [CR_OWLBEAR] = {
        .name = "owlbear",
        .render = SHAPE_OGRE(BANK_FLESH),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 18, 12, 16, 2, 12, 7 } },
        .health = { .hp_max = 23, .ac = 13 },
        .melee = { .to_hit = 5, .dice = 2, .sides = 6, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .melee_hit = { EF_HOLD, 1 },                    /* a hug */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 225,                                      /* 5 hit dice, special */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 5, .depth_max = 7, .rarity = 5 },
    },
    [CR_WISP] = {
        .name = "will-o'-wisp",
        .render = SHAPE_ORB(BANK_BLUE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_FAST, .ability = { 2, 20, 10, 8, 14, 10 } },
        .health = { .hp_max = 22, .immune = DMG_BIT(DMG_POISON), .ac = 19 },   /* hard to hit */
        .mana = { .mp_max = 8 },
        .spells = { SP_MAGIC_BOLT, SP_BLINK },
        .melee = { .to_hit = 5, .dice = 2, .sides = 4, .to_damage = 0, .type = DMG_MAGIC, .range = 1 },   /* a shock */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 225,                                      /* 5 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_ERRATIC, 1 },                    /* it drifts */
        .where = { .depth = 5, .depth_max = 8, .rarity = 3 },
    },
    [CR_WARG] = {                                       /* later: packs (bands) */
        .name = "warg",
        .render = SHAPE_DOG(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_QUICK, .ability = { 15, 14, 14, 6, 12, 8 } },
        .health = { .hp_max = 18, .ac = 14 },
        .melee = { .to_hit = 4, .dice = 2, .sides = 4, .to_damage = 1, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 75,                                       /* 4 hit dice */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 8, .rarity = 5 },
    },
    [CR_WIGHT] = {
        .name = "wight",
        .render = SHAPE_ZOMBIE(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 13, 12, 12, 10, 12, 14 } },
        .health = { .hp_max = 18, .immune = IMMUNE_UNDEAD, .resist = RESIST_WEAPONS, .ac = 14 },
        .melee = { .to_hit = 4, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_COLD, .range = 1 },    /* a draining touch */
        .melee_hit = { EF_DRAIN_CON, 1 },
        .inventory = { .capacity = 0, .slots = 0 },     /* no sword: a weapon's hit would take its touch's place (R17) */
        .xp = 125,                                      /* 4 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 8, .rarity = 4 },
    },
    [CR_BASILISK] = {
        .name = "basilisk",
        .render = SHAPE_LIZARD(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 16, 8, 15, 2, 10, 6 } },
        .health = { .hp_max = 27, .ac = 15 },
        .melee = { .to_hit = 6, .dice = 1, .sides = 10, .to_damage = 0, .type = DMG_PIERCE, .range = 1 }, /* bite */
        .ranged = { .to_hit = 6, .dice = 1, .sides = 1, .to_damage = 0, .type = DMG_MAGIC, .range = 4 },  /* its gaze */
        .ranged_hit = { EF_PARALYSE, 1 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 8, .rarity = 3 },
    },
    [CR_WYVERN] = {
        .name = "wyvern",
        .render = SHAPE_DRAGON(BANK_GREEN),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 17, 12, 15, 5, 12, 8 } },
        .health = { .hp_max = 27, .ac = 14 },
        .melee = { .to_hit = 6, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* a sting */
        .melee_hit = { EF_POISON, 3 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = CREATURE_DRAGON,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 9, .rarity = 4 },
    },
    [CR_MUMMY] = {
        .name = "mummy",
        .render = SHAPE_ZOMBIE(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 17, 8, 16, 6, 10, 12 } },
        .health = { .hp_max = 32, .immune = IMMUNE_UNDEAD, .resist = DMG_BIT(DMG_BLUNT) | DMG_BIT(DMG_PIERCE),
                    .vuln = DMG_BIT(DMG_FIRE), .ac = 14 },
        .melee = { .to_hit = 7, .dice = 2, .sides = 6, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a rotting fist */
        .melee_hit = { EF_DRAIN_CON, 2 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 850,                                      /* 7 hit dice, special */
        .flags = CREATURE_UNDEAD | CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 4 },
    },
    [CR_HILL_GIANT] = {
        .name = "hill giant",
        .render = SHAPE_OGRE(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 21, 8, 19, 6, 9, 7 } },
        .health = { .hp_max = 36, .ac = 13 },
        .melee = { .to_hit = 8, .dice = 2, .sides = 8, .to_damage = 2, .type = DMG_BLUNT, .range = 1 },   /* fists */
        .ranged = { .to_hit = 6, .dice = 2, .sides = 6, .to_damage = 0, .type = DMG_BLUNT, .range = 5 },  /* a boulder */
        .inventory = { .capacity = 3, .slots = SLOT_BIT(SLOT_WEAPON) },   /* wields what it finds */
        .xp = 650,                                      /* 8 hit dice */
        .flags = CREATURE_GIANT,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 5 },
    },
    [CR_HELL_HOUND] = {                                 /* later: packs (bands) */
        .name = "hell hound",
        .render = SHAPE_DOG(BANK_ORANGE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 16, 13, 15, 6, 12, 8 } },
        .health = { .hp_max = 27, .immune = DMG_BIT(DMG_FIRE), .vuln = DMG_BIT(DMG_COLD), .ac = 15 },   /* R9 */
        .mana = { .mp_max = 6 },                        /* one breath, and the next half a breath away */
        .spells = { SP_FIRE_BREATH },
        .melee = { .to_hit = 6, .dice = 1, .sides = 6, .to_damage = 1, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = CREATURE_BEAST,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 4 },
    },
    [CR_BANSHEE] = {
        .name = "banshee",
        .render = SHAPE_GHOST(BANK_ICE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* floats: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 4, 14, 12, 12, 14, 16 } },
        .health = { .hp_max = 32, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .resist = RESIST_WEAPONS, .ac = 15 },
        .mana = { .mp_max = 8 },                        /* two wails */
        .spells = { SP_WAIL },
        .melee = { .to_hit = 7, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_COLD, .range = 1 },    /* a chill touch */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 850,                                      /* 7 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 8, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 3 },
    },
    [CR_MINOTAUR] = {
        .name = "minotaur",
        .render = SHAPE_OGRE(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_QUICK, .ability = { 20, 12, 18, 8, 12, 8 } },
        .health = { .hp_max = 40, .ac = 14 },
        .melee = { .to_hit = 9, .dice = 2, .sides = 8, .to_damage = 2, .type = DMG_PIERCE, .range = 1 },  /* horns */
        .kit = { IT_BATTLE_AXE },
        .inventory = { .capacity = 2, .slots = SLOT_BIT(SLOT_WEAPON) },
        .xp = 900,                                      /* 9 hit dice */
        .flags = CREATURE_GIANT,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 8, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 4 },
    },
    [CR_FROST_DRAKE] = {
        .name = "frost drake",
        .render = SHAPE_DRAGON(BANK_ICE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* flies: over holes and traps */
        .stats = { .speed = MONSTER_SPEED, .ability = { 19, 11, 17, 12, 13, 14 } },
        .health = { .hp_max = 40, .immune = DMG_BIT(DMG_COLD), .vuln = DMG_BIT(DMG_FIRE), .ac = 16 },   /* R9 */
        .mana = { .mp_max = 10 },                       /* one breath, and most of the next */
        .spells = { SP_FROST_BREATH },
        .melee = { .to_hit = 9, .dice = 2, .sides = 6, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 1600,                                     /* 9 hit dice, special */
        .flags = CREATURE_DRAGON,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 9, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 3 },
    },
    [CR_VAMPIRE] = {                                    /* later: life drain (M5.1) */
        .name = "vampire",
        .render = SHAPE_ROBE(BANK_RED),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 18, 16, 16, 14, 14, 18 } },
        .health = { .hp_max = 41, .immune = IMMUNE_UNDEAD, .resist = RESIST_WEAPONS, .vuln = DMG_BIT(DMG_FIRE), .ac = 16 },
        .mana = { .mp_max = 8 },
        .spells = { SP_HOLD, SP_BLINK },
        .melee = { .to_hit = 9, .dice = 1, .sides = 8, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* a bite */
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 1600,                                     /* 9 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .innate = { EF_REGEN, 2 },
        .where = { .depth = 9, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 2 },
    },
    [CR_NECROMANCER] = {                                /* later: holds court (bands) */
        .name = "necromancer",
        .render = SHAPE_ROBE(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 10, 12, 12, 17, 14, 12 } },
        .health = { .hp_max = 22, .ac = 11 },
        .mana = { .mp_max = 14 },                       /* a summoning and a bolt, or bolts and a hold */
        .spells = { SP_FROST_BOLT, SP_HOLD, SP_SUMMON_UNDEAD, SP_BLINK },
        .melee = { .to_hit = 5, .dice = 1, .sides = 6, .to_damage = 0, .type = DMG_BLUNT, .range = 1 },   /* a staff's blow */
        .kit = { IT_QUARTERSTAFF, IT_ROBE },
        .inventory = { .capacity = 4, .slots = SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_BODY) },   /* drinks potions: Int 17 */
        .xp = 500,                                      /* 6 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = 7, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 3 },
    },
    [CR_LICH] = {
        .name = "lich",
        .render = SHAPE_ROBE(BANK_BONE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 12, 14, 14, 20, 16, 16 } },
        .health = { .hp_max = 50, .immune = IMMUNE_UNDEAD | DMG_BIT(DMG_COLD), .ac = 17 },
        .mana = { .mp_max = 30 },
        .spells = { SP_POWER_BOLT, SP_FROST_BOLT, SP_HOLD, SP_SUMMON_UNDEAD },
        .melee = { .to_hit = 10, .dice = 1, .sides = 10, .to_damage = 0, .type = DMG_COLD, .range = 1 },  /* a cold touch */
        .melee_hit = { EF_PARALYSE, 1 },
        .inventory = { .capacity = 0, .slots = 0 },     /* nothing wielded: a weapon's hit would take its touch's place (R17) */
        .xp = 2300,                                     /* 11 hit dice, special */
        .flags = CREATURE_UNDEAD,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP, .traits = AI_TRAIT_SKIRMISH },
        .where = { .depth = LEVEL_DEPTH_BOTTOM, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 1 },
    },
    [CR_GAS_SPORE] = {
        .name = "gas spore",
        .render = SHAPE_EYE(BANK_IRON),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR },   /* floats: over holes and traps */
        .stats = { .speed = MONSTER_SLOW, .ability = { 1, 3, 10, 1, 8, 1 } },
        .health = { .hp_max = 4, .immune = DMG_BIT(DMG_POISON), .ac = 10 },
        .inventory = { .capacity = 0, .slots = 0 },     /* no attack: it drifts up to you */
        .xp = 25,                                       /* 1 hit die, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .death = { EF_BOLT_FIRE, 8 },                   /* bursts: kill it from afar */
        .where = { .depth = 2, .depth_max = 5, .rarity = 3 },
    },
    [CR_SALAMANDER] = {
        .name = "salamander",
        .render = SHAPE_LIZARD(BANK_ORANGE),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 16, 14, 15, 6, 10, 8 } },
        .health = { .hp_max = 32, .immune = DMG_BIT(DMG_FIRE), .vuln = DMG_BIT(DMG_COLD), .ac = 15 },
        .melee = { .to_hit = 7, .dice = 1, .sides = 8, .to_damage = 0, .type = DMG_PIERCE, .range = 1 },  /* bite */
        .melee_hit = { EF_BURNING, 2 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 850,                                      /* 7 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .struck = { EF_BURNING, 1 },                    /* hot to touch */
        .where = { .depth = 8, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 4 },
    },
    [CR_BLACK_PUDDING] = {                              /* later: splits (M5.3) */
        .name = "black pudding",
        .render = SHAPE_SLIME(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SLOW, .ability = { 16, 3, 18, 1, 4, 1 } },
        .health = { .hp_max = 45, .immune = DMG_BIT(DMG_ACID) | DMG_BIT(DMG_COLD) | DMG_BIT(DMG_POISON), .ac = 6 },
        .melee = { .to_hit = 9, .dice = 2, .sides = 8, .to_damage = 0, .type = DMG_ACID, .range = 1 },    /* a dissolving touch */
        .melee_hit = { EF_CORROSION, 2 },
        .inventory = { .capacity = 0, .slots = 0 },
        .xp = 1750,                                     /* 10 hit dice, special */
        .flags = CREATURE_MINDLESS,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .struck = { EF_CORROSION, 1 },                  /* acid on what strikes it */
        .where = { .depth = 8, .depth_max = LEVEL_DEPTH_BOTTOM, .rarity = 3 },
    },
    [CR_ORC_WARLORD] = {                                /* later: leads a war party (bands) */
        .name = "orc warlord",
        .render = SHAPE_GOBLIN(BANK_DARK),
        .block = { NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE },
        .stats = { .speed = MONSTER_SPEED, .ability = { 16, 11, 14, 10, 10, 13 } },
        .health = { .hp_max = 23, .vuln = DMG_BIT(DMG_FIRE), .ac = 12 },
        .mana = { .mp_max = 6 },                        /* a war cry, and most of the next */
        .spells = { SP_WAR_CRY },
        .melee = { .to_hit = 5, .dice = 1, .sides = 8, .to_damage = 0, .type = DMG_SLASH, .range = 1 },   /* claws */
        .kit = { IT_BATTLE_AXE, IT_CHAIN_MAIL },
        .inventory = { .capacity = 4, .slots = SLOTS_FIGHTER },   /* drinks potions: Int 10 */
        .xp = 225,                                      /* 5 hit dice, special */
        .flags = 0,
        .ai = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP },
        .where = { .depth = 6, .depth_max = 8, .rarity = 2 },
    },
};

/*
 * The bands (docs/creature_extensions.md M3): a leader, and up to 3 kinds of
 * member, each min to max. The goblin war band and the bone patrol stop
 * where their leaders' depths stop (goblin 1-3, skeleton 3-5)
 */
#define BAND(l, m0, n0, x0, m1, n1, x1, m2, n2, x2, d0, d1, r) \
    { .leader = (l), .member = { (m0), (m1), (m2) }, .min = { (n0), (n1), (n2) }, .max = { (x0), (x1), (x2) }, \
      .where = { .depth = (d0), .depth_max = (d1), .rarity = (r) } }

const gen_band_t gen_bands[GEN_BANDS] = {
    /*    leader            member, min, max ...                                                       depths  rarity */
    BAND(CR_KOBOLD,      CR_KOBOLD, 1, 2,        CR_KOBOLD_SLINGER, 0, 1,  CR_NONE, 0, 0,             1, 3, 6),    /* kobold pack */
    BAND(CR_JACKAL,      CR_JACKAL, 2, 2,        CR_NONE, 0, 0,            CR_NONE, 0, 0,             1, 3, 5),    /* jackal pack */
    BAND(CR_GOBLIN,      CR_GOBLIN, 1, 2,        CR_GOBLIN_ARCHER, 1, 1,   CR_GOBLIN_SHAMAN, 0, 1,    2, 3, 5),    /* goblin war band */
    BAND(CR_HOBGOBLIN,   CR_GOBLIN, 2, 2,        CR_GOBLIN_ARCHER, 1, 1,   CR_GOBLIN_SHAMAN, 1, 1,    4, 6, 3),    /* hobgoblin troop */
    BAND(CR_WOLF,        CR_WOLF, 1, 2,          CR_NONE, 0, 0,            CR_NONE, 0, 0,             3, 5, 5),    /* wolf pack */
    BAND(CR_SKELETON,    CR_SKELETON, 1, 1,      CR_SKELETON_ARCHER, 1, 1, CR_NONE, 0, 0,             3, 5, 4),    /* bone patrol */
    BAND(CR_GHOUL,       CR_ZOMBIE, 1, 2,        CR_NONE, 0, 0,            CR_NONE, 0, 0,             3, 5, 3),    /* ghoul pack */
    BAND(CR_ORC_WARLORD, CR_ORC, 2, 2,           CR_ORC_ARCHER, 1, 1,      CR_NONE, 0, 0,             6, 8, 4),    /* orc war party */
    BAND(CR_WARG,        CR_WARG, 1, 2,          CR_NONE, 0, 0,            CR_NONE, 0, 0,             6, 8, 4),    /* warg pack */
    BAND(CR_NECROMANCER, CR_ZOMBIE, 1, 1,        CR_SKELETON, 1, 1,        CR_WIGHT, 0, 1,            7, LEVEL_DEPTH_BOTTOM, 3),   /* necromancer's court */
    BAND(CR_HELL_HOUND,  CR_HELL_HOUND, 1, 1,    CR_NONE, 0, 0,            CR_NONE, 0, 0,             7, LEVEL_DEPTH_BOTTOM, 3),   /* hell hound pack */
    BAND(CR_LICH,        CR_MUMMY, 1, 1,         CR_WIGHT, 1, 1,           CR_NONE, 0, 0,             LEVEL_DEPTH_BOTTOM, LEVEL_DEPTH_BOTTOM, 1),   /* the lich's crypt */
};
