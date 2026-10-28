/*
 * game/level: the level generator (level_gen.c), for many depths and
 * seeds, and going from level to level (level_travel.c): what is kept,
 * parked and released, arrivals, requests, entities sent away. The layout
 * is checked in terrain, whatever the art; the map's looks and blocking
 * against it (the decorator, level_deco.c, has its own test: test_deco).
 */
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "map/nge_map.h"

#include "event/nge_event.h"

#include "data/tile_data.h"

#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/level/level_internal.h"
#include "game/ui/screen_main.h"

#define SEEDS  20   /* level_dev_seed values: SEEDS * LEVEL_DEPTH_BOTTOM levels checked */

/* The screen's map view (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

static nge_entity_id_t player;

/* The look drawn at (x, y) */
static uint8_t tile(uint8_t x, uint8_t y)
{
    return NGE_MAP_TILE(x, y).tile_id;
}

static uint8_t terrain(uint8_t x, uint8_t y)
{
    return level_terrain[y][x];
}

/* A room's floor: where entities are placed */
static int is_floor(uint8_t x, uint8_t y)
{
    return terrain(x, y) == TERRAIN_FLOOR;
}

/* What each terrain blocks (docs/dungeon_decoration.md R4): the map's block table must agree */
static nge_block_t terrain_block(uint8_t t)
{
    static const nge_block_t blocks[TERRAIN_COUNT] = {
        [TERRAIN_ROCK] = NGE_BLOCK_SOLID, [TERRAIN_WALL] = NGE_BLOCK_SOLID,
        [TERRAIN_FLOOR] = NGE_BLOCK_NONE, [TERRAIN_CORRIDOR] = NGE_BLOCK_NONE,
        [TERRAIN_DOORWAY] = NGE_BLOCK_NONE, [TERRAIN_HOLE] = NGE_BLOCK_NO_FLOOR,
    };

    return blocks[t];
}

/* Totals over every depth checked */
static int doors_open;
static int doors_shut;
static int doorways_seen;

/* The events posted: kept to look at, then thrown away */
static nge_event_t seen[32];
static int seen_count;

static void record(const nge_event_t *e)
{
    if (seen_count < (int)(sizeof(seen) / sizeof(seen[0]))) {
        seen[seen_count] = *e;
    }
    seen_count++;
}

/* Go to depth as the game does: the level, its events, then the frame's clean-up */
static void go(uint8_t depth, uint8_t arrival)
{
    seen_count = 0;
    level_generate(depth, player, arrival);
    nge_event_process(record);
    level_release_dying();
}

/* The top owner of an entity in use: an area or the world */
static nge_owner_t top_owner(nge_entity_id_t id)
{
    nge_owner_t o = NGE_OWNER_OF(id);

    while (NGE_OWNER_IS_ENTITY(o)) {
        o = NGE_OWNER_OF(o);
    }
    return o;
}

/* An entity or one of its owners is persistent */
static int kept(nge_entity_id_t id)
{
    nge_owner_t o = id;

    while (NGE_OWNER_IS_ENTITY(o)) {
        if (NGE_ENTITY_HAS_FLAGS(o, NGE_ENTITY_PERSISTENT)) {
            return 1;
        }
        o = NGE_OWNER_OF(o);
    }
    return 0;
}

/* The room (index into level_rooms) whose floor holds (x, y), or -1 */
static int room_of(uint8_t x, uint8_t y)
{
    int cell;

    for (cell = 0; cell < LEVEL_CELLS; cell++) {
        const level_room_t *r = &level_rooms[cell];

        if (!r->junction && x > r->x1 && x < r->x2 && y > r->y1 && y < r->y2) {
            return cell;
        }
    }
    return -1;
}

/* A located creature of kind in the current area, on free floor */
static nge_entity_id_t placed_creature(uint8_t kind)
{
    nge_entity_id_t id = gen_creature(kind, LEVEL_AREA);
    uint8_t x;
    uint8_t y;

    for (y = 1; y < LEVEL_HEIGHT; y++) {
        for (x = 1; x < LEVEL_WIDTH; x++) {
            if (is_floor(x, y) && nge_location_find(LEVEL_AREA, x, y, 0, 0) == NGE_ENTITY_INVALID) {
                nge_location_set(id, x, y);
                return id;
            }
        }
    }
    return NGE_ENTITY_INVALID;
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
    int monsters = 0;
    int doors = 0;
    int ups = 0;
    int downs = 0;
    int up_room = -1;
    int down_room = -1;
    int x;
    int y;
    uint8_t i;
    nge_entity_id_t id;

    /* Rooms: 5-10, each in its cell with a tile to spare, walled, with floor inside */
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
            if (r->x1 != r->x2 || r->y1 != r->y2 || terrain(r->x1, r->y1) != TERRAIN_CORRIDOR) {
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
                uint8_t t = terrain((uint8_t)x, (uint8_t)y);

                if (edge_x && edge_y) {
                    if (t != TERRAIN_WALL) {
                        return fail(depth, "a corner isn't wall");
                    }
                } else if (edge_x || edge_y) {
                    if (t != TERRAIN_WALL && t != TERRAIN_DOORWAY) {
                        return fail(depth, "a wall broken by something not a doorway");
                    }
                } else if (t != TERRAIN_FLOOR) {
                    return fail(depth, "a room's floor isn't floor");
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
        if (terrain((uint8_t)x, 0) != TERRAIN_ROCK || terrain((uint8_t)x, LEVEL_HEIGHT - 1) != TERRAIN_ROCK) {
            return fail(depth, "the top or bottom edge isn't rock");
        }
    }
    for (y = 0; y < LEVEL_HEIGHT; y++) {
        if (terrain(0, (uint8_t)y) != TERRAIN_ROCK || terrain(LEVEL_WIDTH - 1, (uint8_t)y) != TERRAIN_ROCK) {
            return fail(depth, "the left or right edge isn't rock");
        }
    }

    /*
     * The map against the terrain: every cell blocks what its terrain blocks
     * (a feature's look blocks nothing, and stands on open terrain), and
     * every cell without a feature is decorated: drawing it again gives the
     * same look
     */
    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            nge_tile_t before = NGE_MAP_TILE(x, y);

            if (nge_map_block((uint8_t)x, (uint8_t)y) != terrain_block(terrain((uint8_t)x, (uint8_t)y))) {
                return fail(depth, "a cell's look doesn't block what its terrain blocks");
            }
            if (nge_location_find(LEVEL_AREA, (uint8_t)x, (uint8_t)y, COMPONENT_FEATURE, 0) == NGE_ENTITY_INVALID) {
                level_decorate((uint8_t)x, (uint8_t)y);
                if (memcmp(&before, &NGE_MAP_TILE(x, y), sizeof(before)) != 0) {
                    return fail(depth, "a cell isn't drawn as the decorator draws it");
                }
            }
        }
    }

    /* The player: alive, in the area, alone on floor or on stairs */
    if (level_depth != depth) {
        return fail(depth, "level_depth isn't the depth");
    }
    if (!NGE_ENTITY_IS_ALIVE(player) || NGE_OWNER_OF(player) != LEVEL_AREA ||
        !NGE_ENTITY_HAS_COMPONENTS(player, NGE_COMPONENT_LOCATION)) {
        return fail(depth, "the player isn't placed in the area");
    }
    x = NGE_LOCATION_X(player);
    y = NGE_LOCATION_Y(player);
    if (!(is_floor((uint8_t)x, (uint8_t)y) && entities_at((uint8_t)x, (uint8_t)y) == 1) &&
        !((tile((uint8_t)x, (uint8_t)y) == LOOK_STAIRS_UP || tile((uint8_t)x, (uint8_t)y) == LOOK_STAIRS_DOWN) &&
          entities_at((uint8_t)x, (uint8_t)y) == 2)) {
        return fail(depth, "the player isn't alone on floor or on stairs");
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

    /*
     * Items (4-10) and monsters (2-5): generated kinds, each alone on floor
     * in a room (so reachable); monsters block, items don't
     */
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id == player) {
            continue;
        }
        if (KIND_IS(id, CLASS_FEATURE) && (kind_index[id] == FT_STAIRS_UP || kind_index[id] == FT_STAIRS_DOWN)) {
            /* Stairs: on floor in a room, drawn as their tile, blocking nothing, alone (but for the player) */
            uint8_t sx = NGE_LOCATION_X(id);
            uint8_t sy = NGE_LOCATION_Y(id);
            int up = kind_index[id] == FT_STAIRS_UP;

            if (!NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION) ||
                tile(sx, sy) != (up ? LOOK_STAIRS_UP : LOOK_STAIRS_DOWN) ||
                NGE_BLOCK_OF(id) != NGE_BLOCK_NONE || nge_map_block(sx, sy) != NGE_BLOCK_NONE ||
                room_of(sx, sy) < 0) {
                return fail(depth, "stairs aren't drawn on a room's floor, open");
            }
            if (entities_at(sx, sy) != ((NGE_LOCATION_X(player) == sx && NGE_LOCATION_Y(player) == sy) ? 2 : 1)) {
                return fail(depth, "something else is on the stairs");
            }
            if (level_stairs[up ? 0 : 1] != id) {
                return fail(depth, "the stairs aren't the ones remembered");
            }
            if (up) {
                ups++;
                up_room = room_of(sx, sy);
            } else {
                downs++;
                down_room = room_of(sx, sy);
            }
            continue;
        }
        if (KIND_IS(id, CLASS_FEATURE)) {
            /*
             * Doors: placed alone in a doorway, drawn as a map tile (no
             * sprite) for its state, blocking only while closed
             */
            uint8_t fx;
            uint8_t fy;
            uint8_t d;

            if (kind_index[id] != FT_DOOR || !FEATURE_IS(id) ||
                !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION | NGE_COMPONENT_BLOCK) ||
                NGE_ENTITY_HAS_ANY_COMPONENTS(id, NGE_COMPONENT_SPRITE)) {
                return fail(depth, "a feature isn't a placed door drawn as a tile");
            }
            fx = NGE_LOCATION_X(id);
            fy = NGE_LOCATION_Y(id);
            for (d = 0; d < level_doorway_count; d++) {
                if (level_doorway_x[d] == fx && level_doorway_y[d] == fy) {
                    break;
                }
            }
            if (d == level_doorway_count || entities_at(fx, fy) != 1) {
                return fail(depth, "a door isn't alone in a doorway");
            }
            if (FEATURE_IS_OPEN(id) ? (tile(fx, fy) != LOOK_DOOR_OPEN || NGE_BLOCK_OF(id) != NGE_BLOCK_NONE)
                                    : (tile(fx, fy) != LOOK_DOOR_SHUT || NGE_BLOCK_OF(id) != NGE_BLOCK_SOLID)) {
                return fail(depth, "a door's tile or blocking doesn't match its state");
            }
            if (nge_map_block(fx, fy) != NGE_BLOCK_NONE) {
                return fail(depth, "a door's map tile blocks");
            }
            doors++;
            if (FEATURE_IS_OPEN(id)) {
                doors_open++;
            } else {
                doors_shut++;
            }
            continue;
        }
        if (!NGE_ENTITY_HAS_COMPONENTS(id, COMPONENT_KIND | NGE_COMPONENT_LOCATION | NGE_COMPONENT_SPRITE)) {
            return fail(depth, "an entity in the area isn't a placed, drawn, generated one");
        }
        if (kind_class[id] == CLASS_ITEM && kind_index[id] < IT_COUNT && ITEM_IS(id) &&
            !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK)) {
            items++;
        } else if (kind_class[id] == CLASS_CREATURE && kind_index[id] < CR_COUNT &&
                   kind_index[id] != CR_PLAYER && NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK)) {
            monsters++;
        } else {
            return fail(depth, "an entity isn't an item or a monster");
        }
        if (!is_floor(NGE_LOCATION_X(id), NGE_LOCATION_Y(id)) ||
            entities_at(NGE_LOCATION_X(id), NGE_LOCATION_Y(id)) != 1) {
            return fail(depth, "an item or monster isn't alone on floor");
        }
    }
    /* One up stairs; one down stairs, in another room, except at the bottom */
    if (ups != 1 || downs != (depth < LEVEL_DEPTH_BOTTOM ? 1 : 0)) {
        return fail(depth, "stairs count");
    }
    if (downs == 1 && up_room == down_room) {
        return fail(depth, "both stairs in one room");
    }
    if (downs == 0 && level_stairs[1] != NGE_ENTITY_INVALID) {
        return fail(depth, "down stairs remembered at the bottom");
    }
    if (items < LEVEL_ITEMS_MIN || items > LEVEL_ITEMS_MAX) {
        return fail(depth, "item count");
    }
    if (monsters < LEVEL_MONSTERS_MIN || monsters > LEVEL_MONSTERS_MAX) {
        return fail(depth, "monster count");
    }
    /* Doors: at most the maximum; every doorway is doorway terrain, drawn as one if it has no door; two per link */
    if (doors > LEVEL_DOORS_MAX) {
        return fail(depth, "door count");
    }
    for (i = 0; i < level_doorway_count; i++) {
        uint8_t dx = level_doorway_x[i];
        uint8_t dy = level_doorway_y[i];

        if (terrain(dx, dy) != TERRAIN_DOORWAY) {
            return fail(depth, "a doorway isn't doorway terrain");
        }
        if (nge_location_find(LEVEL_AREA, dx, dy, COMPONENT_FEATURE, 0) == NGE_ENTITY_INVALID &&
            tile(dx, dy) != LOOK_DOORWAY) {
            return fail(depth, "a doorway without a door isn't drawn as a doorway");
        }
    }
    if (level_doorway_count > links * 2) {
        return fail(depth, "more doorways than link ends");
    }
    doorways_seen += level_doorway_count;
    return 1;
}

/* ---- A level's fingerprint, to compare two generations ---- */

static nge_tile_t snapshot[LEVEL_HEIGHT][LEVEL_WIDTH];
static uint8_t snapshot_items[LEVEL_HEIGHT][LEVEL_WIDTH];   /* class * 16 + kind, 0 for none */

/* The map and the generated entities: not the player, nor the entity `skip` (one parked there) */
static void take(nge_tile_t tiles[LEVEL_HEIGHT][LEVEL_WIDTH], uint8_t items[LEVEL_HEIGHT][LEVEL_WIDTH],
                 nge_entity_id_t skip)
{
    nge_entity_id_t id;
    uint8_t i;
    int y;

    for (y = 0; y < LEVEL_HEIGHT; y++) {
        memcpy(tiles[y], &NGE_MAP_TILE(0, y), sizeof(nge_tile_t) * LEVEL_WIDTH);
    }
    memset(items, 0, LEVEL_HEIGHT * LEVEL_WIDTH);
    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id != player && id != skip) {
            items[NGE_LOCATION_Y(id)][NGE_LOCATION_X(id)] = (uint8_t)(kind_class[id] * 16 + kind_index[id]);
        }
    }
}

/* At the same tile */
static int same_tile(nge_entity_id_t a, nge_entity_id_t b)
{
    return NGE_LOCATION_X(a) == NGE_LOCATION_X(b) && NGE_LOCATION_Y(a) == NGE_LOCATION_Y(b);
}

/* The first entity of class c in the current area that isn't the player, or NGE_ENTITY_INVALID */
static nge_entity_id_t first_of(uint8_t c)
{
    nge_entity_id_t id;
    uint8_t i;

    NGE_OWNER_FOR_EACH_OWNED(i, id, LEVEL_AREA) {
        if (id != player && KIND_IS(id, c)) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

int main(void)
{
    static nge_tile_t again[LEVEL_HEIGHT][LEVEL_WIDTH];
    static uint8_t again_items[LEVEL_HEIGHT][LEVEL_WIDTH];
    int seed;
    int depth;
    int ok;
    int n;
    uint8_t free_before;
    uint8_t d;
    uint8_t arrival;
    uint8_t i;
    nge_entity_id_t id;
    nge_entity_id_t monster;
    nge_entity_id_t potion;
    nge_entity_id_t ring;
    nge_entity_id_t sword;
    nge_entity_id_t boss;
    nge_entity_id_t poison;
    nge_entity_id_t regen;

    test_init();

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    level_init();

    player = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    CHECK(NGE_ENTITY_HAS_FLAGS(player, NGE_ENTITY_PERSISTENT) && PC_IS(player));

    /* ---- every depth, many seeds: the layout's rules, reachability, the entities, the stairs ---- */
    ok = 1;
    for (seed = 0; seed < SEEDS && ok; seed++) {
        level_dev_seed = (uint16_t)(seed * 100);
        for (depth = 1; depth <= LEVEL_DEPTH_BOTTOM && ok; depth++) {
            go((uint8_t)depth, (uint8_t)((seed + depth) % 3));
            ok = check_level(depth);
        }
    }
    CHECK(ok);
    level_dev_seed = 0;
    /* About a third of doorways get doors, about half of them open */
    CHECK(doors_open + doors_shut > doorways_seen / 5 && doors_open + doors_shut < doorways_seen / 2);
    CHECK(doors_open > (doors_open + doors_shut) / 3 && doors_shut > (doors_open + doors_shut) / 3);

    /* ---- arrivals: on the stairs taken, or anywhere (R16) ---- */
    go(4, LEVEL_ARRIVE_UP_STAIRS);
    CHECK(level_stairs[0] != NGE_ENTITY_INVALID && same_tile(player, level_stairs[0]));
    go(4, LEVEL_ARRIVE_DOWN_STAIRS);
    CHECK(level_stairs[1] != NGE_ENTITY_INVALID && same_tile(player, level_stairs[1]));
    go(4, LEVEL_ARRIVE_ANY);
    CHECK(check_level(4) && is_floor(NGE_LOCATION_X(player), NGE_LOCATION_Y(player)));
    go(LEVEL_DEPTH_BOTTOM, LEVEL_ARRIVE_DOWN_STAIRS);   /* no down stairs: free floor instead */
    CHECK(check_level(LEVEL_DEPTH_BOTTOM) && is_floor(NGE_LOCATION_X(player), NGE_LOCATION_Y(player)));
    CHECK(seen_count == 0);   /* level_generate() posts nothing: EV_ARRIVED is the game's */

    /* ---- debug seeding: a depth is the same whatever happened before, parked entities aside (R7, R7a) ---- */
    go(7, LEVEL_ARRIVE_ANY);
    take(snapshot, snapshot_items, NGE_ENTITY_INVALID);
    go(8, LEVEL_ARRIVE_UP_STAIRS);
    take(again, again_items, NGE_ENTITY_INVALID);
    CHECK(memcmp(snapshot, again, sizeof(again)) != 0);
    for (n = 0; n < 37; n++) {
        rand();   /* play: combat rolls */
    }
    sword = gen_item(IT_SWORD, NGE_OWNER_AREA(7));   /* parked on depth 7 */
    nge_entity_set_flags(sword, NGE_ENTITY_PERSISTENT);
    go(7, LEVEL_ARRIVE_DOWN_STAIRS);
    take(again, again_items, sword);
    CHECK(memcmp(snapshot, again, sizeof(again)) == 0);
    CHECK(memcmp(snapshot_items, again_items, sizeof(again_items)) == 0);
    CHECK(NGE_OWNER_OF(sword) == LEVEL_AREA && NGE_ENTITY_HAS_COMPONENTS(sword, NGE_COMPONENT_LOCATION) &&
          is_floor(NGE_LOCATION_X(sword), NGE_LOCATION_Y(sword)));   /* placed again (R8) */

    /* ---- leaving a level: only kept entities stay, parked, unlocated (R12, R13) ---- */
    go(5, LEVEL_ARRIVE_ANY);
    monster = first_of(CLASS_CREATURE);
    CHECK(monster != NGE_ENTITY_INVALID);
    potion = gen_item(IT_POTION, NGE_OWNER_WORLD);
    inv_stow(potion, monster);                        /* a monster's possession: goes with it */
    ring = gen_item(IT_RING, NGE_OWNER_WORLD);
    inv_stow(ring, monster);
    nge_entity_set_flags(ring, NGE_ENTITY_PERSISTENT);   /* a kept one held by an unkept one */
    id = gen_item(IT_FOOD, NGE_OWNER_WORLD);
    inv_stow(id, player);                             /* the player's pack goes with them */
    sword = gen_item(IT_SWORD, NGE_OWNER_WORLD);
    inv_stow(sword, player);
    level_leave(player);   /* the first half of a change of level */
    CHECK(NGE_ENTITY_STATE(monster) == NGE_ENTITY_FREE && NGE_ENTITY_STATE(potion) == NGE_ENTITY_FREE);
    CHECK(NGE_OWNER_OF(player) == NGE_OWNER_WORLD);
    go(6, LEVEL_ARRIVE_UP_STAIRS);
    CHECK(NGE_ENTITY_IS_ALIVE(ring) && NGE_OWNER_OF(ring) == NGE_OWNER_AREA(5) &&
          !NGE_ENTITY_HAS_COMPONENTS(ring, NGE_COMPONENT_LOCATION));   /* parked on depth 5 */
    CHECK(NGE_ENTITY_IS_ALIVE(id) && NGE_OWNER_OF(id) == player);
    CHECK(NGE_ENTITY_IS_ALIVE(sword) && NGE_OWNER_OF(sword) == player);
    ok = 1;
    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        if (top_owner(id) == NGE_OWNER_AREA(5) &&
            (!kept(id) || NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION))) {
            ok = 0;
        }
    }
    CHECK(ok);
    CHECK(nge_entity_dying_count() == 0);
    CHECK(check_level(6));

    /* ---- many transitions: no entity is lost (D5) ---- */
    go(3, LEVEL_ARRIVE_ANY);
    free_before = nge_entity_free_count();
    for (n = 0; n < 60; n++) {
        go((uint8_t)(1 + n % LEVEL_DEPTH_BOTTOM), (uint8_t)(n % 3));
    }
    go(3, LEVEL_ARRIVE_ANY);
    CHECK(nge_entity_free_count() == free_before);

    /* ---- parked entities are frozen: limited effects end, lasting ones stay (R11) ---- */
    go(2, LEVEL_ARRIVE_ANY);
    boss = placed_creature(CR_GOBLIN);
    CHECK(boss != NGE_ENTITY_INVALID);
    nge_entity_set_flags(boss, NGE_ENTITY_PERSISTENT);
    CHECK(effect_apply(EF_POISON, boss, NGE_ENTITY_INVALID));
    CHECK(effect_apply(EF_REGEN, boss, NGE_ENTITY_INVALID));
    poison = NGE_ENTITY_INVALID;
    regen = NGE_ENTITY_INVALID;
    NGE_OWNER_FOR_EACH_OWNED(i, id, boss) {
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_EFFECT)) {
            if (nge_timer_cycles[id] != 0) {
                poison = id;
            } else {
                regen = id;
            }
        }
    }
    CHECK(poison != NGE_ENTITY_INVALID && regen != NGE_ENTITY_INVALID);
    CHECK(level_holds(boss) && level_holds(poison));
    go(3, LEVEL_ARRIVE_UP_STAIRS);
    CHECK(NGE_ENTITY_IS_ALIVE(boss) && NGE_OWNER_OF(boss) == NGE_OWNER_AREA(2) &&
          !NGE_ENTITY_HAS_COMPONENTS(boss, NGE_COMPONENT_LOCATION));
    CHECK(!level_holds(boss) && !level_holds(regen));
    CHECK(NGE_ENTITY_IS_ALIVE(regen) && NGE_OWNER_OF(regen) == boss);   /* lasts for ever: stays */
    CHECK(NGE_ENTITY_STATE(poison) == NGE_ENTITY_FREE || NGE_OWNER_OF(poison) != boss);   /* ended */
    CHECK(!(stats_status[boss] & STATUS_POISONED));
    go(2, LEVEL_ARRIVE_DOWN_STAIRS);
    CHECK(NGE_OWNER_OF(boss) == LEVEL_AREA && NGE_ENTITY_HAS_COMPONENTS(boss, NGE_COMPONENT_LOCATION) &&
          is_floor(NGE_LOCATION_X(boss), NGE_LOCATION_Y(boss)) && level_holds(boss));

    /* ---- a persistent boss killed: its persistent item survives, dropped where it died (R10, R10a) ---- */
    ring = gen_item(IT_RING, NGE_OWNER_WORLD);
    inv_stow(ring, boss);
    nge_entity_set_flags(ring, NGE_ENTITY_PERSISTENT);
    potion = gen_item(IT_SWORD, NGE_OWNER_WORLD);
    inv_stow(potion, boss);
    item_slot[potion] = SLOT_WEAPON;                  /* equipped */
    nge_entity_set_flags(potion, NGE_ENTITY_PERSISTENT);
    {
        uint8_t bx = NGE_LOCATION_X(boss);
        uint8_t by = NGE_LOCATION_Y(boss);

        health_damage(boss, NGE_ENTITY_INVALID, 255, DMG_MAGIC);
        CHECK(NGE_ENTITY_STATE(boss) == NGE_ENTITY_DYING);   /* persistent, but it dies */
        level_release_dying();
        CHECK(NGE_ENTITY_STATE(boss) == NGE_ENTITY_FREE);
        CHECK(NGE_ENTITY_IS_ALIVE(ring) && NGE_ENTITY_HAS_FLAGS(ring, NGE_ENTITY_PERSISTENT) &&
              NGE_OWNER_OF(ring) == LEVEL_AREA && NGE_LOCATION_X(ring) == bx && NGE_LOCATION_Y(ring) == by);
        CHECK(NGE_ENTITY_IS_ALIVE(potion) && NGE_OWNER_OF(potion) == LEVEL_AREA &&
              item_slot[potion] == SLOT_NONE && NGE_LOCATION_X(potion) == bx && NGE_LOCATION_Y(potion) == by);
    }
    /* The player at 0 hp is kept: the game ends instead */
    health_damage(player, NGE_ENTITY_INVALID, 255, DMG_MAGIC);
    CHECK(NGE_ENTITY_IS_ALIVE(player) && NGE_ENTITY_HAS_FLAGS(player, NGE_ENTITY_PERSISTENT));
    health_hp[player] = health_hp_max[player];   /* healing does nothing at 0 hp: back by hand */

    /* ---- requests: kept in range, the first wins (R2, R15) ---- */
    level_clear_request();
    CHECK(!level_pending());
    level_request(0, LEVEL_ARRIVE_ANY);                 /* a fall upwards: not out of the dungeon */
    CHECK(level_pending());
    level_request(5, LEVEL_ARRIVE_ANY);                 /* ignored: the first wins */
    level_take_request(&d, &arrival);
    CHECK(d == 1 && arrival == LEVEL_ARRIVE_ANY && !level_pending());
    level_request(0, LEVEL_ARRIVE_DOWN_STAIRS);         /* up the stairs from depth 1: out */
    level_take_request(&d, &arrival);
    CHECK(d == 0 && arrival == LEVEL_ARRIVE_DOWN_STAIRS);
    level_request(LEVEL_DEPTH_BOTTOM + 1, LEVEL_ARRIVE_UP_STAIRS);
    level_take_request(&d, &arrival);
    CHECK(d == LEVEL_DEPTH_BOTTOM);
    level_request(3, LEVEL_ARRIVE_ANY);
    level_clear_request();
    CHECK(!level_pending());

    /* ---- entities sent away (R18) ---- */
    go(4, LEVEL_ARRIVE_ANY);
    monster = placed_creature(CR_GOBLIN);
    potion = gen_item(IT_POTION, NGE_OWNER_WORLD);
    inv_stow(potion, monster);
    ring = gen_item(IT_RING, NGE_OWNER_WORLD);
    inv_stow(ring, monster);
    nge_entity_set_flags(ring, NGE_ENTITY_PERSISTENT);
    CHECK(level_send(monster, 4) == 0 && NGE_ENTITY_IS_ALIVE(monster));   /* the same depth: nothing */
    seen_count = 0;
    {
        uint8_t mx = NGE_LOCATION_X(monster);
        uint8_t my = NGE_LOCATION_Y(monster);

        CHECK(level_send(monster, 5) == 1);
        CHECK(NGE_ENTITY_STATE(monster) == NGE_ENTITY_DYING && NGE_ENTITY_STATE(potion) == NGE_ENTITY_DYING);
        nge_event_process(record);
        CHECK(seen_count == 1 && seen[0].id == EV_LEFT_LEVEL && seen[0].target == monster && seen[0].p1 == 5);
        level_release_dying();
        CHECK(NGE_ENTITY_STATE(monster) == NGE_ENTITY_FREE && NGE_ENTITY_STATE(potion) == NGE_ENTITY_FREE);
        CHECK(NGE_ENTITY_IS_ALIVE(ring) && NGE_OWNER_OF(ring) == LEVEL_AREA &&
              NGE_LOCATION_X(ring) == mx && NGE_LOCATION_Y(ring) == my);   /* dropped where it was */
    }
    boss = placed_creature(CR_GOBLIN);
    nge_entity_set_flags(boss, NGE_ENTITY_PERSISTENT);
    CHECK(level_send(boss, LEVEL_DEPTH_BOTTOM + 3) == 1);   /* kept to the bottom */
    CHECK(NGE_ENTITY_IS_ALIVE(boss) && NGE_OWNER_OF(boss) == NGE_OWNER_AREA(LEVEL_DEPTH_BOTTOM) &&
          !NGE_ENTITY_HAS_COMPONENTS(boss, NGE_COMPONENT_LOCATION));
    go(LEVEL_DEPTH_BOTTOM, LEVEL_ARRIVE_ANY);
    CHECK(NGE_OWNER_OF(boss) == LEVEL_AREA && NGE_ENTITY_HAS_COMPONENTS(boss, NGE_COMPONENT_LOCATION));

    /* ---- a boss left behind with what it carries: both parked, still held (R10a, R12) ---- */
    go(5, LEVEL_ARRIVE_ANY);
    boss = placed_creature(CR_GOBLIN);
    nge_entity_set_flags(boss, NGE_ENTITY_PERSISTENT);
    ring = gen_item(IT_RING, NGE_OWNER_WORLD);
    inv_stow(ring, boss);                              /* not persistent itself: kept by its holder */
    sword = gen_item(IT_SWORD, NGE_OWNER_WORLD);
    inv_stow(sword, boss);
    nge_entity_set_flags(sword, NGE_ENTITY_PERSISTENT);
    go(6, LEVEL_ARRIVE_UP_STAIRS);
    CHECK(NGE_ENTITY_IS_ALIVE(boss) && NGE_OWNER_OF(boss) == NGE_OWNER_AREA(5));
    CHECK(NGE_ENTITY_IS_ALIVE(ring) && NGE_OWNER_OF(ring) == boss &&
          !NGE_ENTITY_HAS_COMPONENTS(ring, NGE_COMPONENT_LOCATION));
    CHECK(NGE_ENTITY_IS_ALIVE(sword) && NGE_OWNER_OF(sword) == boss);
    go(5, LEVEL_ARRIVE_DOWN_STAIRS);
    CHECK(NGE_OWNER_OF(boss) == LEVEL_AREA && NGE_OWNER_OF(ring) == boss && NGE_OWNER_OF(sword) == boss);

    return test_summary("level");
}
