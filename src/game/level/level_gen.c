/*
 * The level generator: banked in page 102 with its literals and constants,
 * its room list and work data in page 103 (level_pdata.c). It runs once per
 * level.
 *
 * The classic Rogue layout: the map is split into a grid of cells (see
 * level_internal.h). Some cells get a room, the others a junction, a single
 * corridor tile. A random spanning tree over the grid, plus a few extra
 * links, joins neighbouring cells with corridors, so every room can be
 * reached. A corridor between two cells runs through the gap between them,
 * so it never cuts through another room.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "game/components/feature.h"
#include "game/gen/gen.h"
#include "io/nge_tm_stream.h"

#define PLACE_ATTEMPTS  20   /* tries to find a free floor tile */

/* A cell's column and row in the grid (unsigned: no sign handling in the division) */
#define CELL_GX(cell)  ((uint8_t)((unsigned int)(cell) % LEVEL_GRID_W))
#define CELL_GY(cell)  ((uint8_t)((unsigned int)(cell) / LEVEL_GRID_W))

/* A random number from 0 to n-1 */
static uint8_t roll(uint8_t n)
{
    return (uint8_t)((unsigned int)rand() % n);
}

/* A random number from lo to hi */
static uint8_t roll_between(uint8_t lo, uint8_t hi)
{
    return (uint8_t)(lo + roll((uint8_t)(hi - lo + 1)));
}

static void put(uint8_t x, uint8_t y, uint8_t id, uint8_t ink)
{
    nge_tile_t *t = &NGE_MAP_TILE(x, y);

    t->tile_id = id;
    t->tile_attr = (uint8_t)(ink << 4);
}

/* A corridor tile, only over rock: doors, walls and floors stay */
static void put_corridor(uint8_t x, uint8_t y)
{
    if (NGE_MAP_TILE(x, y).tile_id == LEVEL_ROCK) {
        put(x, y, LEVEL_CORRIDOR, NGE_INK_CYAN);
    }
}

/* ---- Clearing ---- */

/* Rock everywhere, and none of the last level's entities but the player */
static void clear(nge_entity_id_t player)
{
    nge_entity_id_t id;
    uint8_t i;
    uint8_t x;
    uint8_t y;

    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            put(x, y, LEVEL_ROCK, 0);
        }
    }
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id != player) {
            if (FEATURE_IS(id)) {
                nge_location_remove(id);   /* the map is new: no floor to put back where it was */
            }
            nge_entity_destroy(id);
        }
    }
    level_doorway_count = 0;
}

/* ---- Rooms and junctions ---- */

/* Draw a room: walls on the edges, floor inside */
static void draw_room(const level_room_t *r)
{
    uint8_t x;
    uint8_t y;

    for (y = r->y1; y <= r->y2; y++) {
        for (x = r->x1; x <= r->x2; x++) {
            if ((y == r->y1 || y == r->y2) && (x == r->x1 || x == r->x2)) {
                put(x, y, LEVEL_CORNER, NGE_INK_WHITE);
            } else if (y == r->y1 || y == r->y2) {
                put(x, y, LEVEL_WALL_H, NGE_INK_WHITE);
            } else if (x == r->x1 || x == r->x2) {
                put(x, y, LEVEL_WALL_V, NGE_INK_WHITE);
            } else {
                /* Floor, with a variant pattern here and there */
                put(x, y, (rand() & 7) == 0 ? LEVEL_FLOOR2 : LEVEL_FLOOR, LEVEL_FLOOR_INK);
            }
        }
    }
}

/*
 * Choose which cells get rooms (the first `rooms` of a shuffled order) and
 * lay out every cell: a room of random size and position, at least one tile
 * inside its cell on every side, or a junction.
 */
static void make_cells(void)
{
    uint8_t order[LEVEL_CELLS];
    uint8_t rooms = roll_between(LEVEL_ROOMS_MIN, LEVEL_ROOMS_MAX);
    uint8_t i;

    for (i = 0; i < LEVEL_CELLS; i++) {
        order[i] = i;
    }
    for (i = LEVEL_CELLS - 1; i > 0; i--) {
        uint8_t j = roll((uint8_t)(i + 1));
        uint8_t t = order[i];

        order[i] = order[j];
        order[j] = t;
    }

    level_room_count = 0;
    for (i = 0; i < LEVEL_CELLS; i++) {
        uint8_t cell = order[i];
        level_room_t *r = &level_rooms[cell];
        uint8_t cx = (uint8_t)(CELL_GX(cell) * LEVEL_CELL_W);
        uint8_t cy = (uint8_t)(CELL_GY(cell) * LEVEL_CELL_H);

        if (i < rooms) {
            /* Walls included: 5x4 (3x2 of floor) up to the cell less a tile each side */
            uint8_t w = roll_between(5, LEVEL_CELL_W - 2);
            uint8_t h = roll_between(4, LEVEL_CELL_H - 2);

            r->x1 = roll_between((uint8_t)(cx + 1), (uint8_t)(cx + LEVEL_CELL_W - 1 - w));
            r->y1 = roll_between((uint8_t)(cy + 1), (uint8_t)(cy + LEVEL_CELL_H - 1 - h));
            r->x2 = (uint8_t)(r->x1 + w - 1);
            r->y2 = (uint8_t)(r->y1 + h - 1);
            r->junction = 0;
            draw_room(r);
            level_room_cells[level_room_count++] = cell;
        } else {
            r->x1 = r->x2 = roll_between((uint8_t)(cx + 1), (uint8_t)(cx + LEVEL_CELL_W - 2));
            r->y1 = r->y2 = roll_between((uint8_t)(cy + 1), (uint8_t)(cy + LEVEL_CELL_H - 2));
            r->junction = 1;
            put_corridor(r->x1, r->y1);
        }
    }
}

/* ---- Links between cells ---- */

/* Link cell a with its neighbour b (to its left, right, above or below) */
static void link(uint8_t a, uint8_t b)
{
    if (a > b) {
        uint8_t t = a;

        a = b;
        b = t;
    }
    level_links[a] |= (b == a + 1) ? LEVEL_LINK_RIGHT : LEVEL_LINK_DOWN;
}

/*
 * A random spanning tree over the grid (a randomised depth-first search,
 * with its own stack): every cell is linked to the others, so every room
 * can be reached
 */
static void make_tree(void)
{
    uint8_t visited[LEVEL_CELLS];
    uint8_t stack[LEVEL_CELLS];
    uint8_t sp = 0;
    uint8_t i;

    for (i = 0; i < LEVEL_CELLS; i++) {
        visited[i] = 0;
        level_links[i] = 0;
    }

    i = roll(LEVEL_CELLS);
    visited[i] = 1;
    stack[sp++] = i;

    while (sp > 0) {
        uint8_t cell = stack[sp - 1];
        uint8_t gx = CELL_GX(cell);
        uint8_t gy = CELL_GY(cell);
        uint8_t next[4];
        uint8_t n = 0;

        if (gx > 0 && !visited[cell - 1]) {
            next[n++] = (uint8_t)(cell - 1);
        }
        if (gx < LEVEL_GRID_W - 1 && !visited[cell + 1]) {
            next[n++] = (uint8_t)(cell + 1);
        }
        if (gy > 0 && !visited[cell - LEVEL_GRID_W]) {
            next[n++] = (uint8_t)(cell - LEVEL_GRID_W);
        }
        if (gy < LEVEL_GRID_H - 1 && !visited[cell + LEVEL_GRID_W]) {
            next[n++] = (uint8_t)(cell + LEVEL_GRID_W);
        }

        if (n == 0) {
            sp--;   /* every neighbour visited: back up */
        } else {
            uint8_t to = next[roll(n)];

            link(cell, to);
            visited[to] = 1;
            stack[sp++] = to;
        }
    }
}

/* Up to LEVEL_LOOPS_MAX extra links between neighbours not yet linked: more than one route */
static void make_loops(void)
{
    uint8_t loops = roll(LEVEL_LOOPS_MAX + 1);
    uint8_t tries = PLACE_ATTEMPTS;

    while (loops > 0 && tries-- > 0) {
        uint8_t cell = roll(LEVEL_CELLS);

        if (rand() & 1) {
            if (CELL_GX(cell) < LEVEL_GRID_W - 1 && !(level_links[cell] & LEVEL_LINK_RIGHT)) {
                level_links[cell] |= LEVEL_LINK_RIGHT;
                loops--;
            }
        } else {
            if (CELL_GY(cell) < LEVEL_GRID_H - 1 && !(level_links[cell] & LEVEL_LINK_DOWN)) {
                level_links[cell] |= LEVEL_LINK_DOWN;
                loops--;
            }
        }
    }
}

/* ---- Corridors ---- */

/* A doorway: floor for now (so no corridor covers it), remembered for place_doors() */
static void door(uint8_t x, uint8_t y)
{
    nge_util_assert(level_doorway_count < LEVEL_DOORWAYS_MAX);

    put(x, y, LEVEL_FLOOR, LEVEL_FLOOR_INK);
    level_doorway_x[level_doorway_count] = x;
    level_doorway_y[level_doorway_count] = y;
    level_doorway_count++;
}

/*
 * Join cell a to the cell on its right, b. Each end is a door in the
 * facing wall (not a corner) with the corridor starting outside it, or the
 * junction itself. The corridor runs across to a random column in the gap
 * between them, along it, and across again: the gap holds no room, as the
 * rooms are at least one tile inside their cells.
 */
static void corridor_right(const level_room_t *a, const level_room_t *b)
{
    uint8_t x0;
    uint8_t y0;
    uint8_t x1;
    uint8_t y1;
    uint8_t mid;
    uint8_t x;
    uint8_t y;

    if (a->junction) {
        x0 = a->x1;
        y0 = a->y1;
    } else {
        y0 = roll_between((uint8_t)(a->y1 + 1), (uint8_t)(a->y2 - 1));
        door(a->x2, y0);
        x0 = (uint8_t)(a->x2 + 1);
    }
    if (b->junction) {
        x1 = b->x1;
        y1 = b->y1;
    } else {
        y1 = roll_between((uint8_t)(b->y1 + 1), (uint8_t)(b->y2 - 1));
        door(b->x1, y1);
        x1 = (uint8_t)(b->x1 - 1);
    }
    nge_util_assert(x0 <= x1);

    mid = roll_between(x0, x1);
    for (x = x0; x <= mid; x++) {
        put_corridor(x, y0);
    }
    for (y = (y0 < y1) ? y0 : y1; y <= ((y0 < y1) ? y1 : y0); y++) {
        put_corridor(mid, y);
    }
    for (x = mid; x <= x1; x++) {
        put_corridor(x, y1);
    }
}

/* The same, joining cell a to the cell below it, b */
static void corridor_down(const level_room_t *a, const level_room_t *b)
{
    uint8_t x0;
    uint8_t y0;
    uint8_t x1;
    uint8_t y1;
    uint8_t mid;
    uint8_t x;
    uint8_t y;

    if (a->junction) {
        x0 = a->x1;
        y0 = a->y1;
    } else {
        x0 = roll_between((uint8_t)(a->x1 + 1), (uint8_t)(a->x2 - 1));
        door(x0, a->y2);
        y0 = (uint8_t)(a->y2 + 1);
    }
    if (b->junction) {
        x1 = b->x1;
        y1 = b->y1;
    } else {
        x1 = roll_between((uint8_t)(b->x1 + 1), (uint8_t)(b->x2 - 1));
        door(x1, b->y1);
        y1 = (uint8_t)(b->y1 - 1);
    }
    nge_util_assert(y0 <= y1);

    mid = roll_between(y0, y1);
    for (y = y0; y <= mid; y++) {
        put_corridor(x0, y);
    }
    for (x = (x0 < x1) ? x0 : x1; x <= ((x0 < x1) ? x1 : x0); x++) {
        put_corridor(x, mid);
    }
    for (y = mid; y <= y1; y++) {
        put_corridor(x1, y);
    }
}

static void make_corridors(void)
{
    uint8_t cell;

    for (cell = 0; cell < LEVEL_CELLS; cell++) {
        if (level_links[cell] & LEVEL_LINK_RIGHT) {
            corridor_right(&level_rooms[cell], &level_rooms[cell + 1]);
        }
        if (level_links[cell] & LEVEL_LINK_DOWN) {
            corridor_down(&level_rooms[cell], &level_rooms[cell + LEVEL_GRID_W]);
        }
    }
}

/* ---- Entities ---- */

/*
 * A door in about one doorway in LEVEL_DOOR_CHANCE, LEVEL_DOORS_MAX at
 * most, half of them open; the other doorways stay floor. Stops if no
 * entity is free
 */
static void place_doors(void)
{
    nge_entity_id_t id;
    uint8_t doors = 0;
    uint8_t i;

    for (i = 0; i < level_doorway_count && doors < LEVEL_DOORS_MAX; i++) {
        if (roll(LEVEL_DOOR_CHANCE) != 0) {
            continue;
        }
        id = gen_feature(FT_DOOR, LEVEL_AREA);
        if (id == NGE_ENTITY_INVALID) {
            return;
        }
        feature_place(id, level_doorway_x[i], level_doorway_y[i]);
        if (rand() & 1) {
            feature_open(id);
        }
        doors++;
    }
}

/*
 * A random floor tile, in a random room, with no entity on it
 * @return 1 and the tile in (x, y), or 0 if none was found in PLACE_ATTEMPTS tries
 */
static uint8_t free_floor(uint8_t *x, uint8_t *y)
{
    uint8_t tries;

    for (tries = 0; tries < PLACE_ATTEMPTS; tries++) {
        const level_room_t *r = &level_rooms[level_room_cells[roll(level_room_count)]];

        *x = roll_between((uint8_t)(r->x1 + 1), (uint8_t)(r->x2 - 1));
        *y = roll_between((uint8_t)(r->y1 + 1), (uint8_t)(r->y2 - 1));
        if (nge_location_find(LEVEL_AREA, *x, *y, 0, 0) == NGE_ENTITY_INVALID) {
            return 1;
        }
    }
    return 0;
}

/*
 * Up to n random entities (gen_item_random() or gen_creature_random()) for
 * the depth, each on free floor in a room. A tile is found first, so an
 * entity is only created where it can go
 */
static void place_random(uint8_t n, uint8_t depth, uint8_t creatures)
{
    nge_entity_id_t id;
    uint8_t x;
    uint8_t y;

    while (n-- > 0) {
        if (!free_floor(&x, &y)) {
            continue;
        }
        id = creatures ? gen_creature_random(depth, LEVEL_AREA) : gen_item_random(depth, LEVEL_AREA);
        if (id != NGE_ENTITY_INVALID) {
            nge_location_set(id, x, y);
        }
    }
}

static void place_player(nge_entity_id_t player)
{
    uint8_t x;
    uint8_t y;

    /* Rooms have at least 6 floor tiles, and there are 15 entities at most: keep looking */
    while (!free_floor(&x, &y)) {
    }
    nge_owner_set(player, LEVEL_AREA);
    nge_location_set(player, x, y);
    nge_map_centre_camera(x, y);
}

void level_generate(uint8_t depth, nge_entity_id_t player) NGE_BANKED
{
    nge_util_assert(player < NGE_ENTITY_MAX);

    srand(depth);   /* for development: each depth's level is always the same */

    clear(player);
    make_cells();
    make_tree();
    make_loops();
    make_corridors();
    place_doors();
    place_random(roll_between(LEVEL_ITEMS_MIN, LEVEL_ITEMS_MAX), depth, 0);
    place_random(roll_between(LEVEL_MONSTERS_MIN, LEVEL_MONSTERS_MAX), depth, 1);
    place_player(player);
}
