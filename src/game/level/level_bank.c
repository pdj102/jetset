/*
 * Setting up the map: banked in page 102 with its literals and constants
 * (the first banked game module, see "Banked code" in docs/memory_map.md).
 * Generating a level is in level_gen.c, in the same page; the per-turn
 * checks are in common memory, in level.c.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include "game/screen.h"

/*
 * What each tile ID blocks. Everything is solid unless set otherwise, so a
 * tile that isn't listed (rock, a new wall pattern) can't be walked on.
 */
static void set_up_blocks(void)
{
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block(LEVEL_FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_FLOOR2, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_CORRIDOR, NGE_BLOCK_NONE);
    /* Features' tiles: the feature blocks (a closed door), not its tile */
    nge_map_set_block(LEVEL_TILE_DOOR_SHUT, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_TILE_DOOR_OPEN, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_TILE_STAIRS_UP, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_TILE_STAIRS_DOWN, NGE_BLOCK_NONE);
}

void level_put_floor(uint8_t x, uint8_t y) NGE_BANKED
{
    nge_tile_t tile;

    tile.tile_id = LEVEL_FLOOR;
    tile.tile_attr = (uint8_t)(LEVEL_FLOOR_INK << 4);
    nge_map_set(x, y, &tile);
}

void level_init(void) NGE_BANKED
{
    nge_map_init(LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);   /* maps the map's page into slot 1 */

    set_up_blocks();
    nge_map_clip_sprites();
}
