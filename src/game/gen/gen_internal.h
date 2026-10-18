#ifndef GEN_INTERNAL_H
#define GEN_INTERNAL_H

/*
 * Shared by the generator module's files; not for use outside it:
 * - gen.c (common memory): gen_name(), which finds the class's page;
 * - gen_shared_bank.c (page 110): helpers used by every class's generator;
 * - gen_<class>_bank.c (even page): a class's generator;
 * - gen_<class>_pdata.c (odd page): its table of base records.
 *
 * A class's table is in its odd page, in slot 7 only while its own code
 * runs. Calling any other page (the shared helpers, the components, nge)
 * maps that page over slots 6 and 7, so a class first copies the record it
 * needs onto the stack, in common memory, and passes the copy.
 *
 * Base records are made of parts that match the components they set: a
 * component's base type (stats_base_t) is defined with the component and
 * passed to its _add() function, so the record and the component can't
 * drift apart. Records hold no pointers to code: behaviour is chosen by
 * numbers (flags, kinds) that the game's code switches on.
 */

#include "game/gen/gen.h"

#include "ecs/components/nge_block.h"
#include "game/components/attack.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"

/* The sprite part: nge_render_add() and nge_render_set_anim() arguments */
typedef struct gen_render {
    uint8_t pattern;      /* first pattern */
    uint8_t anim_count;   /* patterns in the animation, 1: still */
    uint8_t anim_rate;    /* frames per pattern */
    uint8_t flags;        /* NGE_SPRITE_MIRROR_*, NGE_SPRITE_PALETTE(n)... */
    uint8_t layer;        /* LAYER_* (game/actors.h) */
} gen_render_t;

/* The block part: nge_block_add() arguments */
typedef struct gen_block {
    nge_block_t block;
    nge_block_t pass;
} gen_block_t;

/* The inventory part: inv_add() arguments; capacity 0 for none (no component) */
typedef struct gen_inventory {
    uint8_t  capacity;
    uint16_t slots;   /* SLOT_BIT() mask */
} gen_inventory_t;

/* Where a kind appears: from depth, chosen at random with weight rarity (0: never) */
typedef struct gen_where {
    uint8_t depth;
    uint8_t rarity;
} gen_where_t;

/* Creature flags */
#define CREATURE_PLAYER  0x01   /* the player: its turns wait for commands */

typedef struct gen_creature_base {
    const char  *name;    /* in the table's page */
    gen_render_t render;
    gen_block_t  block;
    stats_base_t stats;
    health_base_t health;
    attack_base_t melee;      /* its natural melee attack (fists, a bite); dice 0 for none */
    gen_inventory_t inventory;
    uint8_t      flags;   /* CREATURE_* */
    uint8_t      hooks[GEN_HOOK_COUNT];   /* EF_* by GEN_HOOK_*: hit, innate */
    gen_where_t  where;
} gen_creature_base_t;

typedef struct gen_item_base {
    const char  *name;    /* in the table's page */
    gen_render_t render;
    uint8_t      subclass;   /* ITEM_POTION, ITEM_WEAPON... (gen_kinds.h) */
    uint16_t     fits;       /* the slots it can be equipped in (SLOT_BIT() mask), 0 for none */
    health_base_t health;    /* no hit points: it can't be damaged (no component) */
    attack_base_t melee;     /* as a melee weapon; dice 0 for none */
    uint8_t      hooks[GEN_HOOK_COUNT];   /* EF_* by GEN_HOOK_*: use, hit, worn */
    gen_where_t  where;
} gen_item_base_t;

typedef struct gen_feature_base {
    const char        *name;   /* in the table's page */
    gen_feature_info_t info;   /* its looks, what it can do, what it blocks when closed */
    uint8_t            state;  /* FEAT_* it starts in */
    uint8_t            hooks[GEN_HOOK_COUNT];   /* EF_* by GEN_HOOK_* (none yet) */
    gen_where_t        where;  /* for random features, later (gen_feature_random()) */
} gen_feature_base_t;

/* The tables, in the classes' odd pages: read only by their own page's code */
extern const gen_creature_base_t gen_creatures[CR_COUNT];   /* page 107 */
extern const gen_item_base_t     gen_items[IT_COUNT];       /* page 109 */
extern const gen_feature_base_t  gen_features[FT_COUNT];    /* page 123 */

/* ---- Shared helpers, banked in page 110: pointer arguments in common memory ---- */

/* Create an entity owned by owner with the kind component: NGE_ENTITY_INVALID if none is free */
nge_entity_id_t gen_begin(uint8_t c, uint8_t kind, nge_owner_t owner) NGE_BANKED;

/* Add the sprite component from a sprite part */
void gen_render(nge_entity_id_t id, const gen_render_t *r) NGE_BANKED;

/*
 * A random kind from 0 to count-1 appearing at depth, weighted by rarity:
 * where[kind] is each kind's where part, copied from the class's table
 * @return The kind, or 0xFF if none appears there
 */
uint8_t gen_pick(const gen_where_t *where, uint8_t count, uint8_t depth) NGE_BANKED;

/* ---- Each class's name copy, banked with its table (called by gen_name()) ---- */

void gen_creature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED;
void gen_item_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED;
void gen_feature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED;

/* ---- Each class's hooks, banked with its table (called by gen_hook()) ---- */

uint8_t gen_creature_hook(uint8_t kind, uint8_t hook) NGE_BANKED;
uint8_t gen_item_hook(uint8_t kind, uint8_t hook) NGE_BANKED;
uint8_t gen_feature_hook(uint8_t kind, uint8_t hook) NGE_BANKED;

/*
 * Copy a name into buf: at most size-1 characters and a NUL. A macro, so
 * each class's page has its own copy, reading its own table
 */
#define GEN_COPY_NAME(name, buf, size)                       \
    do {                                                     \
        const char *gen_s_ = (name);                         \
        char *gen_d_ = (buf);                                \
        uint8_t gen_n_ = (size);                             \
        nge_util_assert(gen_n_ >= 1);                        \
        while (--gen_n_ > 0 && *gen_s_ != '\0') {            \
            *gen_d_++ = *gen_s_++;                           \
        }                                                    \
        *gen_d_ = '\0';                                      \
    } while (0)

#endif
