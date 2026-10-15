#ifndef LEVEL_INTERNAL_H
#define LEVEL_INTERNAL_H

/*
 * Shared by the level module's files; not for use outside it:
 * - level.c (common memory): the map and the checks used every turn;
 * - level_bank.c (page 102): setting up and building a level.
 */

#include "game/level/level.h"

/*
 * Map cells: an ASCII font tile and its text colour (palette bank, attribute
 * bits 7-4). Several patterns can share a kind of terrain: what each blocks
 * is set by tile ID in the map's block table (level_bank.c).
 */
#define LEVEL_ROCK      0      /* tile 0: transparent; solid */
#define LEVEL_WALL_H    '-'    /* walls: solid */
#define LEVEL_WALL_V    '|'
#define LEVEL_CORNER    '+'
#define LEVEL_FLOOR     '.'    /* floors: open */
#define LEVEL_FLOOR2    ','
#define LEVEL_DOOR      '\''   /* an open door: open */
#define LEVEL_CORRIDOR  '#'    /* open */

#endif
