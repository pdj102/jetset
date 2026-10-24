/*
 * The level's decorator: draws each cell with a look chosen by its terrain
 * and its neighbours' (docs/dungeon_decoration.md). Banked in page 102 with
 * the level's code; its rule tables (data/tile_rules_data.c, made from the
 * tileset's look names) are in page 103, mapped in slot 7 while it runs.
 *
 * A terrain tries its bases in order. A base's rules are its looks with a
 * code: a pattern on the 8 neighbours' signature, a bit set for each
 * neighbour that joins (by the base's join set). The first rule that
 * matches gives the look; if none in any base, the last base's fallback.
 * Then one of the look's variants, by weight, from a hash of the cell and
 * the level's decoration seed: the same every time the cell is drawn.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include "core/nge_util.h"

#define DECO_BASES_MAX  2   /* bases a terrain tries */

typedef struct deco_base {
    uint8_t       base;    /* LOOK_BASE_* */
    terrain_set_t joins;   /* the neighbours that join, for its codes */
} deco_base_t;

typedef struct deco_terrain {
    nge_block_t block;                  /* what its looks block */
    uint8_t     count;                  /* bases, tried in order */
    deco_base_t bases[DECO_BASES_MAX];  /* the last one has the fallback */
} deco_terrain_t;

/* For walls, the neighbours that are solid: a wall's code says where its open sides are */
#define SOLID_TERRAIN  (TERRAIN_BIT(TERRAIN_WALL) | TERRAIN_BIT(TERRAIN_ROCK))

/*
 * Each terrain's block value and bases. The game's half of the rules: the
 * tileset's names give the looks and their codes. A base named here must be
 * in assets/tile_names.txt, the last of each terrain "required".
 */
static const deco_terrain_t terrains[TERRAIN_COUNT] = {
    [TERRAIN_ROCK]     = { NGE_BLOCK_SOLID, 1, { { LOOK_BASE_ROCK, 0 } } },
    [TERRAIN_WALL]     = { NGE_BLOCK_SOLID, 1, { { LOOK_BASE_WALL, SOLID_TERRAIN } } },
    [TERRAIN_FLOOR]    = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_FLOOR, 0 } } },
    [TERRAIN_CORRIDOR] = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_CORRIDOR, 0 } } },
    [TERRAIN_DOORWAY]  = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_DOORWAY, 0 } } },
    [TERRAIN_HOLE]     = { NGE_BLOCK_NO_FLOOR, 1, { { LOOK_BASE_HOLE, TERRAIN_BIT(TERRAIN_HOLE) } } },
};

/* The 8 neighbours, (dx, dy), in the signature's bit order: N, NE, E, SE, S, SW, W, NW */
static const int8_t around_offsets[16] = { 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, -1, 0, -1, -1 };

/*
 * The tables are walked by pointer, not indexed: an index into a table in a
 * banked page makes zsdcc split its 24-bit address into bytes, which
 * z80asm warns about ("integer range"), though the code is right.
 */

/* A cell's terrain; rock off the map (a coordinate below 0 wraps past the edge) */
static uint8_t terrain_at(uint8_t x, uint8_t y)
{
    return (x < LEVEL_WIDTH && y < LEVEL_HEIGHT) ? level_terrain[y][x] : TERRAIN_ROCK;
}

/* A cell's hash with the level's seed: picks its variants */
static uint8_t hash(uint8_t x, uint8_t y)
{
    uint8_t h = (uint8_t)((uint8_t)(x * 151u) + (uint8_t)(y * 59u)) ^ level_deco_seed;

    h ^= (uint8_t)(h >> 3);
    h = (uint8_t)(h * 109u);
    h ^= (uint8_t)(h >> 4);
    return h;
}

/* One of looks first to first + count - 1, by their weights and h */
static void pick(uint8_t first, uint8_t count, uint8_t h, nge_tile_t *tile)
{
    const tile_look_t *look = tile_looks;
    uint16_t total = 0;
    uint16_t at;
    uint8_t i;

    nge_util_assert(count > 0);
    look += first;
    for (i = 0; i < count; i++) {
        total += look[i].weight;
    }
    at = (uint16_t)((unsigned int)h % total);
    for (i = 0; at >= look[i].weight; i++) {
        at -= look[i].weight;
    }
    tile->tile_id = look[i].tile_id;
    tile->tile_attr = look[i].attr;
}

void level_decorate(uint8_t x, uint8_t y)
{
    uint8_t around[8];
    const int8_t *offset = around_offsets;
    const deco_terrain_t *t = terrains;
    const tile_base_t *base;
    const tile_rule_t *rule;
    terrain_set_t joins;
    nge_tile_t tile;
    uint8_t signature;
    uint8_t bit;
    uint8_t b;
    uint8_t r;
    uint8_t i;
    uint8_t first;
    uint8_t rules;
    uint8_t care;
    uint8_t want;

    nge_util_assert(x < LEVEL_WIDTH && y < LEVEL_HEIGHT);
    for (i = 0; i < 8; i++, offset += 2) {
        around[i] = terrain_at((uint8_t)(x + offset[0]), (uint8_t)(y + offset[1]));
    }
    t += level_terrain[y][x];
    for (b = 0; b < t->count; b++) {
        joins = t->bases[b].joins;
        base = tile_bases;
        base += t->bases[b].base;
        signature = 0;
        for (i = 0, bit = 1; i < 8; i++, bit = (uint8_t)(bit << 1)) {
            if (joins & TERRAIN_BIT(around[i])) {
                signature |= bit;
            }
        }
        /* The fields in locals before the comparisons (T-ZSDCC-1) */
        first = base->first;
        rules = base->count;
        rule = tile_rules;
        rule += first;
        for (r = 0; r < rules; r++, rule++) {
            care = rule->care;
            want = rule->want;
            if ((uint8_t)(signature & care) == want) {
                pick(rule->first, rule->count, hash(x, y), &tile);
                nge_map_set(x, y, &tile);
                return;
            }
        }
    }
    base = tile_bases;
    base += t->bases[t->count - 1].base;
    pick(base->fallback, base->fallback_count, hash(x, y), &tile);
    nge_map_set(x, y, &tile);
}

void level_decorate_all(void)
{
    uint8_t x;
    uint8_t y;

    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            level_decorate(x, y);
        }
    }
}

void level_decorate_around(uint8_t x, uint8_t y)
{
    uint8_t cx;
    uint8_t cy;
    uint8_t i;
    uint8_t j;

    for (j = 0; j < 3; j++) {
        cy = (uint8_t)(y + j - 1);
        for (i = 0; i < 3; i++) {
            cx = (uint8_t)(x + i - 1);
            if (cx < LEVEL_WIDTH && cy < LEVEL_HEIGHT) {
                level_decorate(cx, cy);
            }
        }
    }
}

/* Looks first to first + count - 1 block what their terrain blocks */
static void block_looks(uint8_t first, uint8_t count, nge_block_t block)
{
    const tile_look_t *look = tile_looks;
    uint8_t i;
    uint8_t tile_id;

    look += first;
    for (i = 0; i < count; i++, look++) {
        tile_id = look->tile_id;   /* read before the banked call maps page 44 over it */
        nge_map_set_block(tile_id, block);
    }
}

void level_deco_blocks(void)
{
    const deco_terrain_t *t = terrains;
    const tile_base_t *base;
    const tile_rule_t *rule;
    nge_block_t block;
    uint8_t terrain;
    uint8_t bases;
    uint8_t b;
    uint8_t r;
    uint8_t first;
    uint8_t rules;
    uint8_t look_first;
    uint8_t look_count;

    for (terrain = 0; terrain < TERRAIN_COUNT; terrain++, t++) {
        block = t->block;
        bases = t->count;
        for (b = 0; b < bases; b++) {
            base = tile_bases;
            base += t->bases[b].base;
            first = base->first;
            rules = base->count;
            look_first = base->fallback;
            look_count = base->fallback_count;
            rule = tile_rules;
            rule += first;
            for (r = 0; r < rules; r++, rule++) {
                block_looks(rule->first, rule->count, block);
            }
            block_looks(look_first, look_count, block);
        }
    }
}
