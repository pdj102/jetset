#ifndef LEVEL_INTERNAL_H
#define LEVEL_INTERNAL_H

/*
 * Shared by the level module's files; not for use outside it:
 * - level.c (common memory): the map and the checks used every turn;
 * - level_bank.c (page 102): setting up and building a level.
 */

#include "game/level/level.h"

/* Map cells: an ASCII font tile and its text colour (palette bank, attribute bits 7-4) */
#define LEVEL_ROCK      0      /* tile 0: transparent */
#define LEVEL_WALL_H    '-'
#define LEVEL_WALL_V    '|'
#define LEVEL_FLOOR     '.'
#define LEVEL_DOOR      '+'
#define LEVEL_CORRIDOR  '#'

#endif
