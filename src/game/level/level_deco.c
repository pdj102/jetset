/*
 * The level's decorator: draws each cell with a look chosen by its terrain
 * and its neighbours' (docs/dungeon_decoration.md). Banked in page 102 with
 * the level's code; its rule tables (data/tile_rules_data.c, made from the
 * tileset's look names) are in page 103, mapped in slot 7 while it runs.
 *
 * A terrain has a base e.g. wall_, floor_,.
 * Each terrain has a set of rules, most specific first, that selects the look  
 * Each rule is a pattern based on the terrain and its 8 neighbours' terrains.
 * The first rule that matches gives the look; the last is the terrain's default look, which
 * matches any neighbours.
 * Then one of the look's variants is selected, by weight, from a 
 * hash of the cell and the level's decoration seed: the same every time the
 * cell is drawn.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include "core/nge_util.h"

#define DECO_AROUND  8   /* a cell's neighbours */

/*
 * Each terrain's block value: what its looks block. 
 */
static const nge_block_t terrain_blocks[TERRAIN_COUNT] = {
    [TERRAIN_ROCK]     = NGE_BLOCK_SOLID,
    [TERRAIN_WALL]     = NGE_BLOCK_SOLID,
    [TERRAIN_FLOOR]    = NGE_BLOCK_NONE,
    [TERRAIN_CORRIDOR] = NGE_BLOCK_NONE,
    [TERRAIN_DOORWAY]  = NGE_BLOCK_NONE,
    [TERRAIN_HOLE]     = NGE_BLOCK_NO_FLOOR,
};

/*
 * The tables have a base a terrain, in assets/tile_names.txt's order, and
 * the codes' letters name terrains by that order: it must be TERRAIN_*'s.
 */
typedef char deco_terrains_check[(TILE_TERRAINS == TERRAIN_COUNT &&
                                  TILE_TERRAIN_ROCK == TERRAIN_ROCK &&
                                  TILE_TERRAIN_WALL == TERRAIN_WALL &&
                                  TILE_TERRAIN_FLOOR == TERRAIN_FLOOR &&
                                  TILE_TERRAIN_CORRIDOR == TERRAIN_CORRIDOR &&
                                  TILE_TERRAIN_DOORWAY == TERRAIN_DOORWAY &&
                                  TILE_TERRAIN_HOLE == TERRAIN_HOLE) ? 1 : -1];

/* The 8 neighbours, (dx, dy), in the signature's bit order: N, NE, E, SE, S, SW, W, NW */
static const int8_t around_offsets[2 * DECO_AROUND] = { 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, -1, 0, -1, -1 };

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
    uint8_t around[DECO_AROUND];
    const int8_t *offset = around_offsets;
    const tile_base_t *base = tile_bases;
    const tile_rule_t *rule = tile_rules;
    const uint8_t *want;
    nge_tile_t tile;
    uint8_t first;
    uint8_t rules;
    uint8_t r;
    uint8_t i;
    uint8_t w;

    nge_util_assert(x < LEVEL_WIDTH && y < LEVEL_HEIGHT);
    typedef int check1[sizeof(((tile_rule_t *)0)->want) == DECO_AROUND ? 1 : -1];

    for (i = 0; i < DECO_AROUND; i++, offset += 2) {
        around[i] = terrain_at((uint8_t)(x + offset[0]), (uint8_t)(y + offset[1]));
    }
    base += level_terrain[y][x];
    /* The fields in locals before the comparisons (T-ZSDCC-1) */
    first = base->first;
    rules = base->count;
    rule += first;
    for (r = 0; r < rules; r++, rule++) {
        want = rule->want;
        for (i = 0; i < DECO_AROUND; i++) {
            w = want[i];
            if (w != TILE_ANY && w != around[i]) {
                break;
            }
        }
        if (i == DECO_AROUND) {
            pick(rule->first, rule->count, hash(x, y), &tile);
            nge_map_set(x, y, &tile);
            return;
        }
    }
    nge_util_assert(0);   /* every terrain's base ends with its default, which matches */
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
        tile_id = look->tile_id;   /* read before the banked call maps page 45 over it */
        nge_map_set_block(tile_id, block);
    }
}

void level_deco_blocks(void)
{
    const tile_base_t *base = tile_bases;
    const nge_block_t *blocks = terrain_blocks;
    const tile_rule_t *rule;
    nge_block_t block;
    uint8_t terrain;
    uint8_t r;
    uint8_t rules;

    for (terrain = 0; terrain < TERRAIN_COUNT; terrain++, base++, blocks++) {
        block = *blocks;
        rule = tile_rules;
        rule += base->first;
        rules = base->count;
        for (r = 0; r < rules; r++, rule++) {
            block_looks(rule->first, rule->count, block);
        }
    }
}
