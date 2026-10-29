/*
 * game/level: the decorator (level_deco.c), with the tables made from the
 * real tileset's names (data/tile_rules_data.c): wall pieces by their place,
 * the more specific rule first, every terrain drawn and blocking what it
 * should, variants by weight, the same look every time, and redrawing around
 * a cell.
 */
#include "test.h"

#include <string.h>

#include "map/nge_map.h"

#include "data/tile_data.h"

#include "game/level/level_internal.h"

/* The screen's map view (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

#define MARK  0xEE   /* a tile no look has */

static const nge_block_t terrain_blocks[TERRAIN_COUNT] = {
    [TERRAIN_ROCK] = NGE_BLOCK_SOLID, [TERRAIN_WALL] = NGE_BLOCK_SOLID,
    [TERRAIN_FLOOR] = NGE_BLOCK_NONE, [TERRAIN_CORRIDOR] = NGE_BLOCK_NONE,
    [TERRAIN_DOORWAY] = NGE_BLOCK_NONE, [TERRAIN_HOLE] = NGE_BLOCK_NO_FLOOR,
};

static void fill(uint8_t t)
{
    memset(level_terrain, t, sizeof(level_terrain));
}

/* A room's walls on (x1, y1)-(x2, y2) and its floor inside */
static void room(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uint8_t x;
    uint8_t y;

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            level_terrain[y][x] = (x == x1 || x == x2 || y == y1 || y == y2) ? TERRAIN_WALL : TERRAIN_FLOOR;
        }
    }
}

static uint8_t look(uint8_t x, uint8_t y)
{
    return NGE_MAP_TILE(x, y).tile_id;
}

int main(void)
{
    uint8_t t;
    uint8_t u;
    uint8_t x;
    uint8_t y;
    int ok;
    int variant;
    int floors;
    uint8_t r;
    uint8_t i;

    test_init();
    level_init();   /* the map, and the block table from the terrains' looks */
    level_deco_seed = 0;

    /* ---- a room: each wall piece by its place (assets/Tilesheet.aseprite's names) ---- */
    fill(TERRAIN_ROCK);
    room(2, 2, 8, 6);
    level_terrain[2][5] = TERRAIN_DOORWAY;   /* a doorway in the top wall */
    level_decorate_all();
    CHECK(look(2, 2) == LOOK_WALL_XWWX && look(8, 2) == LOOK_WALL_XXWW);   /* top corners */
    CHECK(look(2, 6) == LOOK_WALL_WWXX && look(8, 6) == LOOK_WALL_WXXW);   /* bottom corners */
    CHECK(look(3, 2) == LOOK_WALL_XWFW && look(4, 6) == LOOK_WALL_FWXW);   /* top, bottom */
    CHECK(look(2, 4) == LOOK_WALL_WFWX && look(8, 4) == LOOK_WALL_WXWF);   /* left, right */
    CHECK(look(4, 2) == LOOK_WALL_XDFW && look(6, 2) == LOOK_WALL_XWFD);   /* beside the doorway: its ends */
    CHECK(look(5, 2) == LOOK_DOORWAY && look(0, 0) == LOOK_ROCK && look(5, 4) != LOOK_ROCK);

    /* Off the map is rock: a room in the top-left corner has the same corner */
    fill(TERRAIN_ROCK);
    room(0, 0, 6, 5);
    level_decorate(0, 0);
    CHECK(look(0, 0) == LOOK_WALL_XWWX);

    /* A wall alone in the open: a pillar */
    fill(TERRAIN_FLOOR);
    level_terrain[10][10] = TERRAIN_WALL;
    level_decorate(10, 10);
    CHECK(look(10, 10) == LOOK_WALL_FFFF);

    /* Walls all round: wall_wwww, though wall_wwwx and the corners match too (D12) */
    fill(TERRAIN_WALL);
    level_decorate(10, 10);
    CHECK(look(10, 10) == LOOK_WALL_WWWW);

    /* ---- every terrain gets a look, among every neighbour, and blocks what it should (R4, R8) ---- */
    ok = 1;
    for (t = 0; t < TERRAIN_COUNT; t++) {
        for (u = 0; u < TERRAIN_COUNT; u++) {
            fill(u);
            level_terrain[10][10] = t;
            level_decorate(10, 10);
            ok = ok && nge_map_block(10, 10) == terrain_blocks[t];
        }
    }
    CHECK(ok);
    CHECK(test_halt_count == 0);   /* every cell matched a rule: every terrain has its default */

    /* ---- every terrain's base ends with its default, and every look blocks its terrain's value (R5, R21) ---- */
    ok = 1;
    for (t = 0; t < TERRAIN_COUNT; t++) {
        const tile_base_t *base = &tile_bases[t];
        const tile_rule_t *rule;

        ok = ok && base->count > 0;
        for (r = 0; r < base->count; r++) {
            rule = &tile_rules[base->first + r];
            for (i = 0; i < rule->count; i++) {
                ok = ok && nge_map_blocks[tile_looks[rule->first + i].tile_id] == terrain_blocks[t];
            }
        }
        rule = &tile_rules[base->first + base->count - 1];
        for (i = 0; i < 8; i++) {
            ok = ok && rule->want[i] == TILE_ANY;
        }
    }
    CHECK(ok);

    /* ---- variants by weight: floor_2 and floor_3 (weights 1, 2) among 9 in all, over a level of floor ---- */
    fill(TERRAIN_FLOOR);
    level_decorate_all();
    floors = 0;
    variant = 0;
    for (y = 1; y < LEVEL_HEIGHT - 1; y++) {
        for (x = 1; x < LEVEL_WIDTH - 1; x++) {
            floors++;
            variant += look(x, y) != LOOK_FLOOR;   /* floor_2, floor_3: LOOK_FLOOR is variant 1 */
        }
    }
    CHECK(variant * 100 > floors * 23 && variant * 100 < floors * 43);   /* about 33% */

    /* ---- the same look every time; another seed, another pattern (R10, D4) ---- */
    t = look(17, 9);
    level_decorate(17, 9);
    CHECK(look(17, 9) == t);
    ok = 0;
    level_deco_seed = 77;
    for (x = 1; x < 30; x++) {
        u = look(x, 9);
        level_decorate(x, 9);
        ok = ok || look(x, 9) != u;
    }
    CHECK(ok);

    /* ---- around a cell: it and its 8 neighbours, no more (R11) ---- */
    for (y = 3; y <= 7; y++) {
        for (x = 3; x <= 7; x++) {
            NGE_MAP_TILE(x, y).tile_id = MARK;
        }
    }
    level_decorate_around(5, 5);
    ok = 1;
    for (y = 3; y <= 7; y++) {
        for (x = 3; x <= 7; x++) {
            int inside = x >= 4 && x <= 6 && y >= 4 && y <= 6;

            ok = ok && (look(x, y) == MARK) == !inside;
        }
    }
    CHECK(ok);
    level_decorate_around(0, 0);   /* at the map's corner: only what is on the map */
    CHECK(test_halt_count == 0);

    return test_summary("deco");
}
