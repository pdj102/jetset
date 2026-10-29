/* nge_block.c, nge_block_line.c: the block component, entering tiles (map and entities), and lines. */
#include "test.h"

#include <stdlib.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#define AREA0  NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)

#define FLOOR  '.'
#define WALL   '#'
#define WALL2  '|'   /* another wall pattern */
#define PIT    'O'

#define PASS_PLAYER  NGE_BLOCK_NONE
#define PASS_BIRD    NGE_BLOCK_NO_FLOOR
#define PASS_GHOST   (NGE_BLOCK_SOLID | NGE_BLOCK_NO_FLOOR)
#define PASS_CURSOR  ((nge_block_t)~NGE_BLOCK_EDGE)

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };   /* a view of 10 x 10 tiles */

static void tile(uint8_t x, uint8_t y, uint8_t id)
{
    NGE_MAP_TILE(x, y).tile_id = id;
}

/* An alive entity in area at (x, y) with the block component */
static nge_entity_id_t put(nge_owner_t area, uint8_t x, uint8_t y, nge_block_t block, nge_block_t pass)
{
    nge_entity_id_t id = nge_entity_create();

    nge_owner_set(id, area);
    nge_location_set(id, x, y);
    nge_block_add(id, block, pass);
    return id;
}

/* The reference: combine every blocker's flags with the tile's, plainly */
static uint8_t can_enter_slow(nge_owner_t area, uint8_t x, uint8_t y, nge_block_t pass, nge_entity_id_t ignore)
{
    nge_block_t block = nge_map_block(x, y);
    int id;

    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (id != ignore && NGE_ENTITY_IS_ALIVE(id) &&
            NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_BLOCK) &&
            nge_owner_of[id] == area && nge_location_x[id] == x && nge_location_y[id] == y) {
            block |= nge_block_of[id];
        }
    }
    return (block & (nge_block_t)~pass) == 0;
}

int main(void)
{
    nge_entity_id_t orc;
    nge_entity_id_t ring;
    nge_entity_id_t player;
    nge_entity_id_t blocker;
    nge_entity_id_t id;
    int x;
    int y;
    int step;
    int ok;

    test_init();

    CHECK(NGE_COMPONENT_BLOCK == NGE_COMPONENT(2));

    /* A 10 x 10 map: floor, a wall in two patterns, a pit */
    nge_map_init(10, 10, &window);
    nge_map_clear_blocks(NGE_BLOCK_SOLID);             /* unknown tiles are solid */
    nge_map_set_block(FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(PIT, NGE_BLOCK_NO_FLOOR);
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            tile((uint8_t)x, (uint8_t)y, FLOOR);
        }
    }
    tile(1, 1, WALL);
    tile(2, 1, WALL2);
    tile(3, 1, PIT);
    tile(4, 1, 0x7F);                                        /* a tile ID nobody set up */

    nge_entity_init();

    /* ---- the map alone: each pass mask against each tile ---- */
    CHECK(nge_block_can_enter(AREA0, 0, 0, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 1);
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 2, 1, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 3, 1, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 4, 1, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 3, 1, PASS_BIRD, NGE_ENTITY_INVALID, NULL) == 1);
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_BIRD, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_GHOST, NGE_ENTITY_INVALID, NULL) == 1);
    CHECK(nge_block_can_enter(AREA0, 3, 1, PASS_GHOST, NGE_ENTITY_INVALID, NULL) == 1);
    /* The edge blocks everyone, even the cursor */
    CHECK(nge_block_can_enter(AREA0, 10, 0, PASS_CURSOR, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 0, 10, PASS_CURSOR, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 255, 255, 0xFF & PASS_CURSOR, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_CURSOR, NGE_ENTITY_INVALID, NULL) == 1);

    /* ---- entities: a creature blocks, an item doesn't ---- */
    orc = put(AREA0, 5, 5, NGE_BLOCK_OCCUPIED, PASS_PLAYER);
    ring = put(AREA0, 6, 5, NGE_BLOCK_NONE, NGE_BLOCK_NONE);
    blocker = 0;
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_PLAYER, NGE_ENTITY_INVALID, &blocker) == 0 && blocker == orc);
    CHECK(nge_block_can_enter(AREA0, 6, 5, PASS_PLAYER, NGE_ENTITY_INVALID, &blocker) == 1 &&
          blocker == NGE_ENTITY_INVALID);
    CHECK(nge_block_find(AREA0, 5, 5, PASS_PLAYER, NGE_ENTITY_INVALID) == orc);
    CHECK(nge_block_find(AREA0, 6, 5, PASS_PLAYER, NGE_ENTITY_INVALID) == NGE_ENTITY_INVALID);
    /* A ghost passes walls but not creatures; the cursor passes creatures */
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_GHOST, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_CURSOR, NGE_ENTITY_INVALID, NULL) == 1);
    /* The map blocks first: no blocker reported for a wall */
    id = put(AREA0, 1, 1, NGE_BLOCK_OCCUPIED, PASS_GHOST);   /* a ghost in the wall */
    blocker = 0;
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_PLAYER, NGE_ENTITY_INVALID, &blocker) == 0 &&
          blocker == NGE_ENTITY_INVALID);
    blocker = 0;
    CHECK(nge_block_can_enter(AREA0, 1, 1, PASS_GHOST, NGE_ENTITY_INVALID, &blocker) == 0 && blocker == id);

    /* ---- ignore: the mover doesn't block itself ---- */
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_PLAYER, orc, NULL) == 1);
    player = put(AREA0, 5, 5, NGE_BLOCK_OCCUPIED, PASS_PLAYER);   /* sharing the orc's tile */
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_PLAYER, orc, &blocker) == 0 && blocker == player);

    /* ---- what doesn't count: other areas, no component, dying ---- */
    put(AREA1, 7, 7, NGE_BLOCK_OCCUPIED, PASS_PLAYER);
    CHECK(nge_block_can_enter(AREA0, 7, 7, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 1);
    nge_block_remove(orc);
    nge_entity_destroy(player);
    CHECK(nge_block_can_enter(AREA0, 5, 5, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 1);
    nge_location_remove(ring);
    nge_block_add(ring, NGE_BLOCK_OCCUPIED, 0);              /* blocks, but has no location */
    CHECK(nge_block_can_enter(AREA0, 6, 5, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 1);

    /* Game flags work like nge's */
    nge_entity_init();
    put(AREA0, 8, 8, NGE_BLOCK_USER, 0);                     /* e.g. water */
    CHECK(nge_block_can_enter(AREA0, 8, 8, PASS_PLAYER, NGE_ENTITY_INVALID, NULL) == 0);
    CHECK(nge_block_can_enter(AREA0, 8, 8, NGE_BLOCK_USER, NGE_ENTITY_INVALID, NULL) == 1);

    /* ---- randomised: the same answers as combining every flag plainly ---- */
    srand(31);
    nge_entity_init();
    ok = 1;
    for (step = 0; step < 4000 && ok; step++) {
        uint8_t tx = (uint8_t)(rand() % 12);                  /* sometimes outside the map */
        uint8_t ty = (uint8_t)(rand() % 3);
        nge_owner_t area = (rand() & 3) ? AREA0 : AREA1;
        nge_block_t pass = (nge_block_t)(rand() & 0x1F);
        nge_entity_id_t ignore = (nge_entity_id_t)((rand() % 4) ? NGE_ENTITY_INVALID : rand() % NGE_ENTITY_MAX);
        uint8_t can;

        switch (rand() % 5) {
        case 0:
            if (nge_entity_free_count() > 0) {
                put(area, (uint8_t)(rand() % 10), (uint8_t)(rand() % 3),
                    (nge_block_t)(rand() & 0x1E), (nge_block_t)(rand() & 0x1F));
            }
            break;
        case 1:
            id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
            if (NGE_ENTITY_IS_ALIVE(id)) {
                nge_location_set(id, (uint8_t)(rand() % 10), (uint8_t)(rand() % 3));
            }
            break;
        case 2:
            id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
            if (NGE_ENTITY_IS_ALIVE(id)) {
                nge_block_remove(id);
            }
            break;
        default:
            break;
        }
        can = nge_block_can_enter(area, tx, ty, pass, ignore, &blocker);
        ok = can == can_enter_slow(area, tx, ty, pass, ignore) &&
             (can || blocker == NGE_ENTITY_INVALID ||
              (nge_block_of[blocker] & (nge_block_t)~pass) != 0);
    }
    CHECK(ok);

    /* ---- the line test: the tiles strictly between the ends ---- */
    nge_entity_init();
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            tile((uint8_t)x, (uint8_t)y, FLOOR);
        }
    }
    CHECK(nge_block_line(AREA0, 0, 0, 9, 0, NGE_BLOCK_SOLID) == 1);   /* horizontal */
    CHECK(nge_block_line(AREA0, 3, 9, 3, 0, NGE_BLOCK_SOLID) == 1);   /* vertical, upwards */
    CHECK(nge_block_line(AREA0, 9, 9, 0, 0, NGE_BLOCK_SOLID) == 1);   /* diagonal */
    CHECK(nge_block_line(AREA0, 0, 0, 2, 9, NGE_BLOCK_SOLID) == 1);   /* steep */
    CHECK(nge_block_line(AREA0, 4, 4, 4, 4, NGE_BLOCK_SOLID) == 1);   /* the same tile */
    /* A wall between blocks, from either end; a wall at either end doesn't */
    tile(5, 0, WALL);
    CHECK(nge_block_line(AREA0, 0, 0, 9, 0, NGE_BLOCK_SOLID) == 0);
    CHECK(nge_block_line(AREA0, 9, 0, 0, 0, NGE_BLOCK_SOLID) == 0);
    CHECK(nge_block_line(AREA0, 5, 0, 9, 0, NGE_BLOCK_SOLID) == 1);
    CHECK(nge_block_line(AREA0, 0, 0, 5, 0, NGE_BLOCK_SOLID) == 1);
    CHECK(nge_block_line(AREA0, 4, 1, 6, 1, NGE_BLOCK_SOLID) == 1);   /* passes below it */
    /* Next-door tiles: nothing between, whatever they are */
    CHECK(nge_block_line(AREA0, 4, 0, 5, 0, NGE_BLOCK_SOLID) == 1);
    CHECK(nge_block_line(AREA0, 4, 1, 5, 0, NGE_BLOCK_SOLID) == 1);
    /* Only the flags asked for: a pit blocks a line asked about NO_FLOOR, not SOLID */
    tile(5, 2, PIT);
    CHECK(nge_block_line(AREA0, 0, 2, 9, 2, NGE_BLOCK_SOLID) == 1);
    CHECK(nge_block_line(AREA0, 0, 2, 9, 2, NGE_BLOCK_NO_FLOOR) == 0);
    /* Entities: one with the flag blocks (a closed door), one without (a creature) doesn't */
    id = put(AREA0, 4, 6, NGE_BLOCK_OCCUPIED, PASS_PLAYER);
    CHECK(nge_block_line(AREA0, 0, 6, 9, 6, NGE_BLOCK_SOLID) == 1);
    CHECK(nge_block_line(AREA0, 0, 6, 9, 6, NGE_BLOCK_SOLID | NGE_BLOCK_OCCUPIED) == 0);
    orc = put(AREA0, 6, 6, NGE_BLOCK_SOLID, PASS_PLAYER);
    CHECK(nge_block_line(AREA0, 0, 6, 9, 6, NGE_BLOCK_SOLID) == 0);
    CHECK(nge_block_line(AREA0, 6, 6, 9, 6, NGE_BLOCK_SOLID) == 1);   /* at the end: not tested */
    nge_block_add(orc, NGE_BLOCK_NONE, PASS_PLAYER);                  /* the door opened */
    CHECK(nge_block_line(AREA0, 0, 6, 9, 6, NGE_BLOCK_SOLID) == 1);
    put(AREA1, 6, 7, NGE_BLOCK_SOLID, PASS_PLAYER);                   /* another area's */
    CHECK(nge_block_line(AREA0, 0, 7, 9, 7, NGE_BLOCK_SOLID) == 1);
    /* The edge always blocks: a line through it, even asking for nothing */
    CHECK(nge_block_line(AREA0, 8, 0, 11, 0, NGE_BLOCK_NONE) == 0);
    CHECK(nge_block_line(AREA0, 8, 0, 10, 0, NGE_BLOCK_NONE) == 1);   /* only the end outside */

    /* ---- asserts ---- */
    nge_entity_init();
    TEST_EXPECT_HALT(nge_block_add(90, 0, 0));               /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_block_remove(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);

    return test_summary("block");
}
