/* game/level: the level generator (level_gen.c), for many depths. */
#include "test.h"

#include <stdio.h>
#include <string.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "map/nge_map.h"

#include "game/items.h"
#include "game/level/level_internal.h"
#include "game/screen.h"

#define DEPTHS  200

/* The screen's map view (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 26 };

static nge_entity_id_t player;

static uint8_t tile(uint8_t x, uint8_t y)
{
    return NGE_MAP_TILE(x, y).tile_id;
}

static int is_floor(uint8_t x, uint8_t y)
{
    return tile(x, y) == LEVEL_FLOOR || tile(x, y) == LEVEL_FLOOR2;
}

/* Release the entities destroyed by the last level_generate(), as the game does each frame */
static void release_dying(void)
{
    nge_entity_id_t id;

    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        nge_entity_release(id);
    }
}

/* Located alive entities in the level's area on (x, y) */
static int entities_at(uint8_t x, uint8_t y)
{
    nge_entity_id_t id;
    int n = 0;

    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, 0) {
        n++;
    }
    return n;
}

/* ---- Flood fill over open tiles from the player ---- */

static uint8_t reached[LEVEL_HEIGHT][LEVEL_WIDTH];
static uint8_t queue_x[LEVEL_WIDTH * LEVEL_HEIGHT];
static uint8_t queue_y[LEVEL_WIDTH * LEVEL_HEIGHT];

static int is_open(int x, int y)
{
    return x >= 0 && y >= 0 && x < LEVEL_WIDTH && y < LEVEL_HEIGHT &&
           nge_map_block((uint8_t)x, (uint8_t)y) == NGE_BLOCK_NONE;
}

static void flood(uint8_t x, uint8_t y)
{
    static const int dx[4] = { 1, -1, 0, 0 };
    static const int dy[4] = { 0, 0, 1, -1 };
    int head = 0;
    int tail = 0;
    int d;

    memset(reached, 0, sizeof(reached));
    reached[y][x] = 1;
    queue_x[tail] = x;
    queue_y[tail++] = y;
    while (head < tail) {
        int cx = queue_x[head];
        int cy = queue_y[head++];

        for (d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            if (is_open(nx, ny) && !reached[ny][nx]) {
                reached[ny][nx] = 1;
                queue_x[tail] = (uint8_t)nx;
                queue_y[tail++] = (uint8_t)ny;
            }
        }
    }
}

/* ---- One level's checks: 1 if all hold, else 0 with a note ---- */

static int fail(int depth, const char *what)
{
    printf("  depth %d: %s\n", depth, what);
    return 0;
}

static int check_level(int depth)
{
    int cell;
    int rooms = 0;
    int links = 0;
    int items = 0;
    int x;
    int y;
    uint8_t i;
    nge_entity_id_t id;

    /* Rooms: 5-10, each in its cell with a tile to spare, walls and floor drawn */
    for (cell = 0; cell < LEVEL_CELLS; cell++) {
        const level_room_t *r = &level_rooms[cell];
        int cx = (cell % LEVEL_GRID_W) * LEVEL_CELL_W;
        int cy = (cell / LEVEL_GRID_W) * LEVEL_CELL_H;

        if (r->x1 < cx + 1 || r->x2 > cx + LEVEL_CELL_W - 2 ||
            r->y1 < cy + 1 || r->y2 > cy + LEVEL_CELL_H - 2) {
            return fail(depth, "a cell's room or junction touches its cell's edge");
        }
        links += (level_links[cell] & LEVEL_LINK_RIGHT) != 0;
        links += (level_links[cell] & LEVEL_LINK_DOWN) != 0;
        if ((level_links[cell] & LEVEL_LINK_RIGHT) && cell % LEVEL_GRID_W == LEVEL_GRID_W - 1) {
            return fail(depth, "a link off the right of the grid");
        }
        if ((level_links[cell] & LEVEL_LINK_DOWN) && cell / LEVEL_GRID_W == LEVEL_GRID_H - 1) {
            return fail(depth, "a link off the bottom of the grid");
        }
        if (r->junction) {
            if (r->x1 != r->x2 || r->y1 != r->y2 || tile(r->x1, r->y1) != LEVEL_CORRIDOR) {
                return fail(depth, "a junction isn't a corridor tile");
            }
            continue;
        }
        rooms++;
        if (r->x2 - r->x1 < 4 || r->y2 - r->y1 < 3) {
            return fail(depth, "a room smaller than 5x4");
        }
        for (y = r->y1; y <= r->y2; y++) {
            for (x = r->x1; x <= r->x2; x++) {
                int edge_x = x == r->x1 || x == r->x2;
                int edge_y = y == r->y1 || y == r->y2;
                uint8_t t = tile((uint8_t)x, (uint8_t)y);

                if (edge_x && edge_y) {
                    if (t != LEVEL_CORNER) {
                        return fail(depth, "a corner isn't a corner");
                    }
                } else if (edge_y) {
                    if (t != LEVEL_WALL_H && t != LEVEL_DOOR) {
                        return fail(depth, "a top or bottom wall broken");
                    }
                } else if (edge_x) {
                    if (t != LEVEL_WALL_V && t != LEVEL_DOOR) {
                        return fail(depth, "a side wall broken");
                    }
                } else if (!is_floor((uint8_t)x, (uint8_t)y)) {
                    return fail(depth, "a room's floor overwritten");
                }
            }
        }
    }
    if (rooms != level_room_count || rooms < LEVEL_ROOMS_MIN || rooms > LEVEL_ROOMS_MAX) {
        return fail(depth, "room count");
    }
    for (i = 0; i < level_room_count; i++) {
        if (level_rooms[level_room_cells[i]].junction) {
            return fail(depth, "the room list holds a junction");
        }
    }
    /* A spanning tree has LEVEL_CELLS - 1 links, and up to 2 loops are added */
    if (links < LEVEL_CELLS - 1 || links > LEVEL_CELLS + 1) {
        return fail(depth, "link count");
    }

    /* The map's edges are rock */
    for (x = 0; x < LEVEL_WIDTH; x++) {
        if (tile((uint8_t)x, 0) != LEVEL_ROCK || tile((uint8_t)x, LEVEL_HEIGHT - 1) != LEVEL_ROCK) {
            return fail(depth, "the top or bottom edge isn't rock");
        }
    }
    for (y = 0; y < LEVEL_HEIGHT; y++) {
        if (tile(0, (uint8_t)y) != LEVEL_ROCK || tile(LEVEL_WIDTH - 1, (uint8_t)y) != LEVEL_ROCK) {
            return fail(depth, "the left or right edge isn't rock");
        }
    }

    /* The player: alive, in the area, alone on floor */
    if (!NGE_ENTITY_IS_ALIVE(player) || NGE_OWNER_OF(player) != LEVEL_AREA ||
        !NGE_ENTITY_HAS_COMPONENTS(player, NGE_COMPONENT_LOCATION)) {
        return fail(depth, "the player isn't placed in the area");
    }
    if (!is_floor(NGE_LOCATION_X(player), NGE_LOCATION_Y(player)) ||
        entities_at(NGE_LOCATION_X(player), NGE_LOCATION_Y(player)) != 1) {
        return fail(depth, "the player isn't alone on floor");
    }

    /* Every open tile can be reached from the player: rooms, doors, corridors, junctions */
    flood(NGE_LOCATION_X(player), NGE_LOCATION_Y(player));
    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            if (is_open(x, y) && !reached[y][x]) {
                return fail(depth, "an open tile can't be reached from the player");
            }
        }
    }

    /* Items: 4-10, each a known kind, alone on floor in a room (so reachable) */
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id == player) {
            continue;
        }
        if (!NGE_ENTITY_HAS_COMPONENTS(id, COMPONENT_ITEM | NGE_COMPONENT_LOCATION | NGE_COMPONENT_SPRITE)) {
            return fail(depth, "an entity in the area isn't a placed, drawn item");
        }
        if (item_kind[id] >= ITEM_KINDS) {
            return fail(depth, "an unknown item kind");
        }
        if (!is_floor(NGE_LOCATION_X(id), NGE_LOCATION_Y(id)) ||
            entities_at(NGE_LOCATION_X(id), NGE_LOCATION_Y(id)) != 1) {
            return fail(depth, "an item isn't alone on floor");
        }
        items++;
    }
    if (items < LEVEL_ITEMS_MIN || items > LEVEL_ITEMS_MAX) {
        return fail(depth, "item count");
    }
    return 1;
}

/* ---- A level's fingerprint, to compare two generations ---- */

static nge_tile_t snapshot[LEVEL_HEIGHT][LEVEL_WIDTH];
static uint8_t snapshot_items[LEVEL_HEIGHT][LEVEL_WIDTH];   /* item kind + 1, 0 for none */

static void take(nge_tile_t tiles[LEVEL_HEIGHT][LEVEL_WIDTH], uint8_t items[LEVEL_HEIGHT][LEVEL_WIDTH])
{
    nge_entity_id_t id;
    uint8_t i;
    int y;

    for (y = 0; y < LEVEL_HEIGHT; y++) {
        memcpy(tiles[y], &NGE_MAP_TILE(0, y), sizeof(nge_tile_t) * LEVEL_WIDTH);
    }
    memset(items, 0, LEVEL_HEIGHT * LEVEL_WIDTH);
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id != player) {
            items[NGE_LOCATION_Y(id)][NGE_LOCATION_X(id)] = (uint8_t)(item_kind[id] + 1);
        }
    }
    items[NGE_LOCATION_Y(player)][NGE_LOCATION_X(player)] = 0xFF;
}

int main(void)
{
    static nge_tile_t again[LEVEL_HEIGHT][LEVEL_WIDTH];
    static uint8_t again_items[LEVEL_HEIGHT][LEVEL_WIDTH];
    int depth;
    int ok;
    int alive;
    int differ;
    uint8_t i;
    nge_entity_id_t id;

    test_init();

    nge_entity_init();
    nge_render_init();
    level_init();

    player = nge_entity_create();
    nge_block_set(player, NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE);

    /* ---- every depth: the layout's rules, reachability, the entities ---- */
    ok = 1;
    for (depth = 1; depth <= DEPTHS && ok; depth++) {
        level_generate((uint8_t)depth, player);
        release_dying();
        ok = check_level(depth);
    }
    CHECK(ok);

    /* ---- the same depth gives the same level; another depth doesn't ---- */
    level_generate(7, player);
    take(snapshot, snapshot_items);
    level_generate(8, player);
    release_dying();
    take(again, again_items);
    differ = memcmp(snapshot, again, sizeof(again)) != 0;
    CHECK(differ);
    level_generate(7, player);
    take(again, again_items);
    CHECK(memcmp(snapshot, again, sizeof(again)) == 0);
    CHECK(memcmp(snapshot_items, again_items, sizeof(again_items)) == 0);

    /* ---- regenerating: the last level's items are destroyed, the player kept ---- */
    id = NGE_ENTITY_INVALID;
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id != player) {
            break;
        }
    }
    level_generate(9, player);
    CHECK(NGE_ENTITY_STATE(id) == NGE_ENTITY_DYING);   /* an old item, before the game releases it */
    CHECK(NGE_ENTITY_IS_ALIVE(player));
    alive = 0;
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        alive++;
    }
    release_dying();
    CHECK(check_level(9));
    CHECK(alive == nge_owner_count(LEVEL_AREA));   /* no dying ones left in the area */

    /* The player's other entities (effects) are kept */
    id = nge_entity_create();
    nge_owner_set(id, player);
    level_generate(10, player);
    release_dying();
    CHECK(NGE_ENTITY_IS_ALIVE(id) && NGE_OWNER_OF(id) == player);

    return test_summary("level");
}
