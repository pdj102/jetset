#include "game/level.h"

#include "game/screen.h"
#include "hw/nge_mmu.h"
#include "io/nge_tm_stream.h"

nge_map_t level_map;

/* Map cells: an ASCII font tile and its text colour (palette bank, attribute bits 7-4) */
#define ROCK      0      /* tile 0: transparent */
#define WALL_H    '-'
#define WALL_V    '|'
#define FLOOR     '.'
#define DOOR      '+'
#define CORRIDOR  '#'

static void put(uint8_t x, uint8_t y, uint8_t id, uint8_t ink)
{
    nge_tile_t *t = NGE_MAP_ROW(&level_map, y) + x;

    t->tile_id = id;
    t->tile_attr = (uint8_t)(ink << 4);
}

/* A room: walls on the edges of (x1, y1)-(x2, y2), floor inside */
static void room(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uint8_t x;
    uint8_t y;

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (y == y1 || y == y2) {
                put(x, y, WALL_H, NGE_INK_WHITE);
            } else if (x == x1 || x == x2) {
                put(x, y, WALL_V, NGE_INK_WHITE);
            } else {
                put(x, y, FLOOR, NGE_INK_YELLOW);
            }
        }
    }
}

static void door(uint8_t x, uint8_t y)
{
    put(x, y, DOOR, NGE_INK_RED);
}

/* Corridors: horizontal from x1 to x2 on row y, vertical from y1 to y2 in column x */
static void corridor_h(uint8_t x1, uint8_t x2, uint8_t y)
{
    for (; x1 <= x2; x1++) {
        put(x1, y, CORRIDOR, NGE_INK_CYAN);
    }
}

static void corridor_v(uint8_t x, uint8_t y1, uint8_t y2)
{
    for (; y1 <= y2; y1++) {
        put(x, y1, CORRIDOR, NGE_INK_CYAN);
    }
}

/*
 * The test layout: three rooms joined by corridors.
 *   A (3,2)-(16,10)    door east (16,6), door south (8,10)
 *   B (30,4)-(48,14)   door west (30,8)
 *   C (10,20)-(24,30)  door north (15,20)
 */
static void build_test_layout(void)
{
    uint8_t x;
    uint8_t y;

    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            put(x, y, ROCK, 0);
        }
    }

    room(3, 2, 16, 10);
    room(30, 4, 48, 14);
    room(10, 20, 24, 30);

    /* A east to B west */
    door(16, 6);
    corridor_h(17, 23, 6);
    corridor_v(23, 6, 8);
    corridor_h(23, 29, 8);
    door(30, 8);

    /* A south to C north */
    door(8, 10);
    corridor_v(8, 11, 15);
    corridor_h(8, 15, 15);
    corridor_v(15, 15, 19);
    door(15, 20);
}

void level_init(void)
{
    nge_mmu_map(LEVEL_MAP_SLOT, LEVEL_MAP_PAGE);
    nge_map_init(&level_map, (nge_tile_t *)NGE_MMU_SLOT_ADDR(LEVEL_MAP_SLOT),
                 LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);
    build_test_layout();
    nge_map_clip_sprites(&level_map);
}

uint8_t level_is_open(uint8_t x, uint8_t y)
{
    uint8_t id = NGE_MAP_TILE(&level_map, x, y).tile_id;

    return id == FLOOR || id == DOOR || id == CORRIDOR;
}
