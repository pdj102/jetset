/*
 * Level functions and data in common memory: the map, used every frame by
 * the map and render systems, and the checks made every turn. Setting up
 * and building a level is banked, in level_bank.c.
 */
#include "game/level/level_internal.h"

nge_map_t level_map;

uint8_t level_can_enter(nge_entity_id_t id, uint8_t x, uint8_t y, nge_entity_id_t *blocker)
{
    nge_block_t pass = NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK) ? NGE_BLOCK_PASS(id)
                                                                         : NGE_BLOCK_NONE;

    return nge_block_can_enter(&level_map, LEVEL_AREA, x, y, pass, id, blocker);
}
