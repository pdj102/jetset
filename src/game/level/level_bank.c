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

#include "game/ui/screen_main.h"

/*
 * What each tile ID blocks. Everything is solid unless set otherwise, so a
 * tile that isn't a look of any terrain can't be walked on. The terrains'
 * looks take their terrain's block value (level_deco.c).
 */
static void set_up_blocks(void)
{
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    level_deco_blocks();
    /* Features' looks: the feature blocks (a closed door), not its tile */
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_STAIRS_UP, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_STAIRS_DOWN, NGE_BLOCK_NONE);
}

void level_redraw(uint8_t x, uint8_t y) NGE_BANKED
{
    level_decorate_around(x, y);
}

void level_init(void) NGE_BANKED
{
    nge_map_init(LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);   /* maps the map's page into slot 1 */

    set_up_blocks();
    nge_map_clip_sprites();
}
