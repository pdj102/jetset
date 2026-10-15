/* nge-dataseg: PAGE_103 */
/*
 * Level page data, in page 103 (slot 7 while the level's code in page 102
 * runs): the room list and the corridor links, set by level_generate().
 * Uninitialised. See "Page data" in docs/memory_map.md.
 */
#include "game/level/level_internal.h"

level_room_t level_rooms[LEVEL_CELLS];
uint8_t      level_room_cells[LEVEL_ROOMS_MAX];
uint8_t      level_room_count;
uint8_t      level_links[LEVEL_CELLS];
