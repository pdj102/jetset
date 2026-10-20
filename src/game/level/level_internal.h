#ifndef LEVEL_INTERNAL_H
#define LEVEL_INTERNAL_H

/*
 * Shared by the level module's files; not for use outside it:
 * - level.c (common memory): the checks used every turn;
 * - level_bank.c (page 102): setting up the map;
 * - level_gen.c (page 102): generating a level;
 * - level_travel.c (page 102): leaving a level, parking and sending entities;
 * - level_pdata.c (page 103): the room list and the generator's work data,
 *   mapped into slot 7 while the page 102 code runs.
 */

#include "game/level/level.h"
#include "io/nge_text.h"

/*
 * Map cells: an ASCII font tile and its text colour (palette bank, attribute
 * bits 7-4). Several patterns can share a kind of terrain: what each blocks
 * is set by tile ID in the map's block table (level_bank.c).
 */
#define LEVEL_ROCK      0      /* tile 0: transparent; solid */
#define LEVEL_WALL_H    '-'    /* walls: solid */
#define LEVEL_WALL_V    '|'
#define LEVEL_CORNER    '-'    /* a room's corners: as its top and bottom walls ('+' is a closed door) */
#define LEVEL_FLOOR     '.'    /* floors: open */
#define LEVEL_FLOOR2    ','
#define LEVEL_FLOOR_INK NGE_INK_YELLOW
#define LEVEL_CORRIDOR  '#'    /* open */

/*
 * The generator's grid: the map is split into LEVEL_GRID_W x LEVEL_GRID_H
 * cells, each holding a room or a junction (a point corridors pass
 * through). Cells are numbered across then down: cell = gy * LEVEL_GRID_W + gx.
 */
#define LEVEL_GRID_W    4
#define LEVEL_GRID_H    3
#define LEVEL_CELLS     (LEVEL_GRID_W * LEVEL_GRID_H)
#define LEVEL_CELL_W    15     /* tiles */
#define LEVEL_CELL_H    13

#define LEVEL_ROOMS_MIN     5  /* rooms per level; the other cells are junctions */
#define LEVEL_ROOMS_MAX     10
#define LEVEL_LOOPS_MAX     2  /* extra links after the spanning tree: 0 to LEVEL_LOOPS_MAX */
#define LEVEL_DOORS_MAX     12 /* door entities per level */
#define LEVEL_DOOR_CHANCE   3  /* a doorway gets a door 1 time in LEVEL_DOOR_CHANCE */
#define LEVEL_ITEMS_MIN     4  /* items per level */
#define LEVEL_ITEMS_MAX     10
#define LEVEL_MONSTERS_MIN  2  /* monsters per level */
#define LEVEL_MONSTERS_MAX  5

/* Each depth has an area of its own */
typedef char level_depth_check[(LEVEL_DEPTH_BOTTOM < NGE_OWNER_AREA_MAX) ? 1 : -1];

typedef char level_grid_check[(LEVEL_GRID_W * LEVEL_CELL_W <= LEVEL_WIDTH &&
                               LEVEL_GRID_H * LEVEL_CELL_H <= LEVEL_HEIGHT &&
                               LEVEL_ROOMS_MAX <= LEVEL_CELLS) ? 1 : -1];

/*
 * A cell's room: its walls are on the edges of (x1, y1)-(x2, y2), its floor
 * inside. A junction is the single tile x1 = x2, y1 = y2.
 */
typedef struct level_room {
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
    uint8_t junction;   /* 1: a junction, not a room */
} level_room_t;

/* Page data in page 103 (level_pdata.c): set by level_generate() */
extern level_room_t level_rooms[LEVEL_CELLS];       /* by cell */
extern uint8_t      level_room_cells[LEVEL_ROOMS_MAX];  /* the cells with rooms */
extern uint8_t      level_room_count;               /* entries in level_room_cells */
extern uint8_t      level_links[LEVEL_CELLS];       /* LEVEL_LINK_* to the cell's right and below */

/* Doorways: where corridors meet rooms, found while making corridors (page 103) */
#define LEVEL_DOORWAYS_MAX  (2 * (LEVEL_CELLS - 1 + LEVEL_LOOPS_MAX))   /* two per link */
extern uint8_t level_doorway_x[LEVEL_DOORWAYS_MAX];
extern uint8_t level_doorway_y[LEVEL_DOORWAYS_MAX];
extern uint8_t level_doorway_count;

/* The stairs, by LEVEL_ARRIVE_UP_STAIRS / _DOWN_STAIRS - 1: NGE_ENTITY_INVALID if none (page 103) */
extern nge_entity_id_t level_stairs[2];

/* Leave the current level: the player taken out of its area, its unkept entities released, the rest parked */
void level_leave(nge_entity_id_t player) NGE_BANKED;

#define LEVEL_LINK_RIGHT  0x01
#define LEVEL_LINK_DOWN   0x02

#endif
