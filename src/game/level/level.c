/*
 * Level functions and data in common memory: the map, used every frame by
 * the map and render systems, and the checks made every turn. Setting up
 * and building a level is banked, in level_bank.c.
 */
#include "game/level/level_internal.h"

nge_map_t level_map;

uint8_t level_is_open(uint8_t x, uint8_t y)
{
    uint8_t id = NGE_MAP_TILE(&level_map, x, y).tile_id;

    return id == LEVEL_FLOOR || id == LEVEL_DOOR || id == LEVEL_CORRIDOR;
}
