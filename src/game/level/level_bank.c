/*
 * Setting up and building a level: banked in page 102 with its literals and
 * constants (the first banked game module, see "Banked code" in
 * docs/memory_map.md). It runs once per level. The map and the per-turn
 * checks are in common memory, in level.c.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include <stdlib.h>

#include "game/screen.h"
#include "hw/nge_mmu.h"
#include "io/nge_tm_stream.h"

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
            if ((y == y1 || y == y2) && (x == x1 || x == x2)) {
                put(x, y, LEVEL_CORNER, NGE_INK_WHITE);
            } else if (y == y1 || y == y2) {
                put(x, y, LEVEL_WALL_H, NGE_INK_WHITE);
            } else if (x == x1 || x == x2) {
                put(x, y, LEVEL_WALL_V, NGE_INK_WHITE);
            } else {
                /* Floor, with a variant pattern here and there */
                put(x, y, (rand() & 7) == 0 ? LEVEL_FLOOR2 : LEVEL_FLOOR, NGE_INK_YELLOW);
            }
        }
    }
}

static void door(uint8_t x, uint8_t y)
{
    put(x, y, LEVEL_DOOR, NGE_INK_RED);
}

/* Corridors: horizontal from x1 to x2 on row y, vertical from y1 to y2 in column x */
static void corridor_h(uint8_t x1, uint8_t x2, uint8_t y)
{
    for (; x1 <= x2; x1++) {
        put(x1, y, LEVEL_CORRIDOR, NGE_INK_CYAN);
    }
}

static void corridor_v(uint8_t x, uint8_t y1, uint8_t y2)
{
    for (; y1 <= y2; y1++) {
        put(x, y1, LEVEL_CORRIDOR, NGE_INK_CYAN);
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
            put(x, y, LEVEL_ROCK, 0);
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

/*
 * What each tile ID blocks. Everything is solid unless set otherwise, so a
 * tile that isn't listed (rock, a new wall pattern) can't be walked on.
 */
static void set_up_blocks(void)
{
    nge_map_clear_blocks(&level_map, NGE_BLOCK_SOLID);
    nge_map_set_block(&level_map, LEVEL_FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(&level_map, LEVEL_FLOOR2, NGE_BLOCK_NONE);
    nge_map_set_block(&level_map, LEVEL_DOOR, NGE_BLOCK_NONE);
    nge_map_set_block(&level_map, LEVEL_CORRIDOR, NGE_BLOCK_NONE);
}

void level_init(void) NGE_BANKED
{
    nge_mmu_map(LEVEL_MAP_SLOT, LEVEL_MAP_PAGE);
    nge_map_init(&level_map, (nge_tile_t *)NGE_MMU_SLOT_ADDR(LEVEL_MAP_SLOT),
                 LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);

    set_up_blocks();
    srand(0); /* reproducible test layout */
    build_test_layout();
    nge_map_clip_sprites(&level_map);
}
