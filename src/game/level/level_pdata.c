/* nge-dataseg: PAGE_103 */
/*
 * Level page data, in page 103 (slot 7 while the level's code in page 102
 * runs): the room list, the corridor links, the stairs, and each cell's
 * terrain, set by level_generate() and kept while the level lasts.
 * Uninitialised. See "Page data" in docs/memory_map.md.
 */
#include "game/level/level_internal.h"

level_room_t level_rooms[LEVEL_CELLS];
uint8_t      level_room_cells[LEVEL_ROOMS_MAX];
uint8_t      level_room_count;
uint8_t      level_links[LEVEL_CELLS];
uint8_t      level_doorway_x[LEVEL_DOORWAYS_MAX];
uint8_t      level_doorway_y[LEVEL_DOORWAYS_MAX];
uint8_t      level_doorway_count;
nge_entity_id_t level_stairs[2];
uint8_t      level_terrain[LEVEL_HEIGHT][LEVEL_WIDTH];
uint8_t      level_deco_seed;
