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
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },   /* placed in doorways, never at random */
    },
    [FT_STAIRS_UP] = {
        .name = "staircase up",
        .info = {
            .look = { { LOOK_STAIRS_UP, LOOK_STAIRS_UP_BANK }, { LOOK_STAIRS_UP, LOOK_STAIRS_UP_BANK } },
            .can = FCAN_ASCEND,
            .block_shut = NGE_BLOCK_NONE,    /* stood on to climb: never in the way */
        },
        .state = 0,
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },   /* placed by the level generator, never at random */
    },
    [FT_STAIRS_DOWN] = {
        .name = "staircase down",
        .info = {
            .look = { { LOOK_STAIRS_DOWN, LOOK_STAIRS_DOWN_BANK }, { LOOK_STAIRS_DOWN, LOOK_STAIRS_DOWN_BANK } },
            .can = FCAN_DESCEND,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = 0,
        .where = { .depth = 0, .depth_max = 0, .rarity = 0 },
    },
    /* Furniture (docs/features.md 5.2): blocks walking, placed only where it can't cut a path */
    [FT_BOULDER] = {
        .name = "boulder",
        .info = {
            .look = { { LOOK_BOULDER, LOOK_BOULDER_BANK }, { LOOK_BOULDER, LOOK_BOULDER_BANK } },
            .block_shut = NGE_BLOCK_SOLID,
        },
        /* Broken by blows: blunt weapons pay */
        .health = { .hp_max = 12, .immune = DMG_BIT(DMG_POISON),
                    .resist = DMG_BIT(DMG_SLASH) | DMG_BIT(DMG_PIERCE), .ac = 1 },
        .where = { .depth = 1, .depth_max = 10, .rarity = 4 },
    },
    [FT_STATUE] = {
        .name = "statue",
        .info = {
            .look = { { LOOK_STATUE, LOOK_STATUE_BANK }, { LOOK_STATUE, LOOK_STATUE_BANK } },
            .block_shut = NGE_BLOCK_SOLID,   /* cover: it blocks sight too */
        },
        .where = { .depth = 2, .depth_max = 10, .rarity = 2 },
    },
    [FT_CRACKED_STATUE] = {
        .name = "cracked statue",
        .info = {
            .look = { { LOOK_STATUE, LOOK_STATUE_BANK }, { LOOK_STATUE, LOOK_STATUE_BANK } },
            .can = FCAN_CONTAINER,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .holds = 1,   /* found by breaking it: it doesn't open */
        .health = { .hp_max = 8, .immune = DMG_BIT(DMG_POISON),
                    .resist = DMG_BIT(DMG_SLASH) | DMG_BIT(DMG_PIERCE), .ac = 1 },
        .where = { .depth = 3, .depth_max = 10, .rarity = 1 },
    },
    /* Operated once (docs/features.md 5.2): the use hook applies to the operator, then it is spent */
    [FT_SHRINE_HEALING] = {
        .name = "healing shrine",
        .info = {
            .look = { { LOOK_SHRINE, LOOK_SHRINE_BANK }, { LOOK_SHRINE_SPENT, LOOK_SHRINE_SPENT_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_PRAY,
        .hooks = { [GEN_HOOK_USE] = { EF_HEALING, 15 } },
        .where = { .depth = 1, .depth_max = 10, .rarity = 3 },
    },
    [FT_SHRINE_CLEANSING] = {
        .name = "cleansing shrine",
        .info = {
            .look = { { LOOK_SHRINE, LOOK_SHRINE_BANK }, { LOOK_SHRINE_SPENT, LOOK_SHRINE_SPENT_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_PRAY,
        .hooks = { [GEN_HOOK_USE] = { EF_CURE_ALL, 1 } },
        .where = { .depth = 2, .depth_max = 10, .rarity = 2 },
    },
    [FT_ALTAR_MIGHT] = {
        .name = "altar of might",
        .info = {
            .look = { { LOOK_ALTAR, LOOK_ALTAR_BANK }, { LOOK_ALTAR_SPENT, LOOK_ALTAR_SPENT_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_TOUCH,
        .hooks = { [GEN_HOOK_USE] = { EF_MIGHT, 2 } },
        .where = { .depth = 3, .depth_max = 10, .rarity = 2 },
    },
    [FT_ALTAR_ANCIENT] = {
        .name = "ancient altar",
        .info = {
            .look = { { LOOK_ALTAR, LOOK_ALTAR_BANK }, { LOOK_ALTAR_SPENT, LOOK_ALTAR_SPENT_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_TOUCH,
        .hooks = { [GEN_HOOK_USE] = { EF_VITALITY, 2 } },
        .where = { .depth = 5, .depth_max = 10, .rarity = 1 },
    },
    [FT_FOUNTAIN_RENEW] = {
        .name = "fountain",
        /* The three fountains: one name and one look, a gamble (docs/features.md D9) */
        .info = {
            .look = { { LOOK_FOUNTAIN, LOOK_FOUNTAIN_BANK }, { LOOK_FOUNTAIN_DRY, LOOK_FOUNTAIN_DRY_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_DRINK,
        .hooks = { [GEN_HOOK_USE] = { EF_RENEW, 1 } },
        .where = { .depth = 1, .depth_max = 10, .rarity = 2 },
    },
    [FT_FOUNTAIN_MANA] = {
        .name = "fountain",
        .info = {
            .look = { { LOOK_FOUNTAIN, LOOK_FOUNTAIN_BANK }, { LOOK_FOUNTAIN_DRY, LOOK_FOUNTAIN_DRY_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_DRINK,
        .hooks = { [GEN_HOOK_USE] = { EF_MANA, 4 } },
        .where = { .depth = 1, .depth_max = 10, .rarity = 2 },
    },
    [FT_FOUNTAIN_FOUL] = {
        .name = "fountain",
        .info = {
            .look = { { LOOK_FOUNTAIN, LOOK_FOUNTAIN_BANK }, { LOOK_FOUNTAIN_DRY, LOOK_FOUNTAIN_DRY_BANK } },
            .can = FCAN_OPERATE | FCAN_ONCE,
            .block_shut = NGE_BLOCK_SOLID,
        },
        .verb = FVERB_DRINK,
        .hooks = { [GEN_HOOK_USE] = { EF_POISON, 1 } },
        .where = { .depth = 1, .depth_max = 10, .rarity = 2 },
    },
    /* A container that opens (docs/features.md 4.9): a door that holds items, which spill on its tile */
    [FT_CHEST] = {
        .name = "chest",
        .info = {
            .look = { { LOOK_CHEST_SHUT, LOOK_CHEST_SHUT_BANK }, { LOOK_CHEST_OPEN, LOOK_CHEST_OPEN_BANK } },
            .can = FCAN_OPEN | FCAN_CONTAINER,
            .block_shut = NGE_BLOCK_SOLID,   /* open, it blocks nothing: its items are picked up from its tile */
        },
        .holds = 2,
        .where = { .depth = 1, .depth_max = 10, .rarity = 6 },
    },
    /* Traps (docs/features.md 5.3): fire at a walker that moves onto them; most start hidden */
    [FT_DART_TRAP] = {
        .name = "dart trap",
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_STEP,
        .dc = 12,
        .attack = { .to_hit = 4, .dice = 1, .sides = 4, .type = DMG_PIERCE, .range = 1 },
        .hooks = { [GEN_HOOK_HIT] = { EF_POISON, 1 } },
        .where = { .depth = 1, .depth_max = 6, .rarity = 4 },
    },
    [FT_PIT] = {
        .name = "pit",   /* stuck in it: a hold */
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_FALL,
        .dc = 10,
        .attack = { .to_hit = 8, .dice = 1, .sides = 6, .type = DMG_BLUNT, .range = 1 },
        .hooks = { [GEN_HOOK_HIT] = { EF_HOLD, 1 } },
        .where = { .depth = 1, .depth_max = 10, .rarity = 4 },
    },
    [FT_FIRE_RUNE] = {
        .name = "fire rune",   /* no attack: it always goes off */
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_STEP,
        .dc = 14,
        .hooks = { [GEN_HOOK_HIT] = { EF_BOLT_FIRE, 6 } },
        .where = { .depth = 4, .depth_max = 10, .rarity = 3 },
    },
    [FT_WEB] = {
        .name = "spider web",   /* seen, not disarmed: cut or burnt (3 hp) */
        .info = {
            .look = { { LOOK_WEB, LOOK_WEB_BANK }, { LOOK_WEB, LOOK_WEB_BANK } },
            .can = FCAN_TRAP,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = 0,
        .verb = FVERB_STEP,
        .dc = 0,
        .health = { .hp_max = 3, .immune = DMG_BIT(DMG_POISON), .vuln = DMG_BIT(DMG_FIRE), .ac = 1 },
        .hooks = { [GEN_HOOK_HIT] = { EF_HOLD, 1 } },
        .where = { .depth = 3, .depth_max = 7, .rarity = 3 },
    },
    /* Pending (D15): complete, but rarity 0 until sleep, teleport and summoning are built */
    [FT_SLEEP_GAS] = {
        .name = "sleeping gas trap",
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_STEP,
        .dc = 13,
        .hooks = { [GEN_HOOK_HIT] = { EF_SLEEP, 1 } },
        .where = { .depth = 3, .depth_max = 10, .rarity = 0 },
    },
    [FT_TELEPORT_RUNE] = {
        .name = "teleport rune",
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_STEP,
        .dc = 13,
        .hooks = { [GEN_HOOK_HIT] = { EF_TELEPORT, 1 } },
        .where = { .depth = 3, .depth_max = 10, .rarity = 0 },
    },
    [FT_SUMMON_RUNE] = {
        .name = "summoning rune",
        .info = {
            .look = { { LOOK_TRAP, LOOK_TRAP_BANK }, { LOOK_TRAP, LOOK_TRAP_BANK } },
            .can = FCAN_TRAP | FCAN_DISARM,
            .block_shut = NGE_BLOCK_NONE,
        },
        .state = FEAT_HIDDEN,
        .verb = FVERB_STEP,
        .dc = 15,
        .hooks = { [GEN_HOOK_HIT] = { EF_SUMMON, 1 } },
        .where = { .depth = 5, .depth_max = 10, .rarity = 0 },
    },
};
