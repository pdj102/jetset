/*
 * Map set-up functions: banked in page 44 with their literals, as they run
 * once per level. The per-frame functions are in nge_map.c.
 */
#ifdef __SDCC
#pragma codeseg PAGE_44
#pragma constseg PAGE_44
#endif

#include "map/nge_map.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "hw/nge_mmu.h"
#include "sprite/nge_sprite.h"

void nge_map_init(uint8_t width, uint8_t height, const nge_rect_t *window) NGE_BANKED
{
    nge_util_assert(window != NULL);
    nge_util_assert(window->x1 <= window->x2 && window->x2 < NGE_TILEMAP_WIDTH);
    nge_util_assert(window->y1 <= window->y2 && window->y2 < NGE_TILEMAP_HEIGHT);

    nge_mmu_map(NGE_MAP_SLOT, NGE_MAP_PAGE);

    nge_map.width = width;
    nge_map.height = height;
    nge_map.window = *window;
    nge_map.view_w = window->x2 - window->x1 + 1;
    nge_map.view_h = window->y2 - window->y1 + 1;

    /* The view must fit inside the map, and the map in its memory */
    nge_util_assert(width >= nge_map.view_w && width <= NGE_MAP_STRIDE);
    nge_util_assert(height >= nge_map.view_h && height <= NGE_MAP_HEIGHT_MAX);

    nge_map.cam_x = 0;
    nge_map.cam_y = 0;
    nge_map.dirty = 1;

    /* A clean map: every tile 0, every tile ID blocking nothing (a new level starts from nothing) */
    memset(nge_map_tiles, 0, sizeof(nge_map_tiles));
    memset(nge_map_blocks, NGE_BLOCK_NONE, sizeof(nge_map_blocks));
}

void nge_map_clear_blocks(nge_block_t flags) NGE_BANKED
{
    memset(nge_map_blocks, flags, NGE_MAP_BLOCK_ENTRIES);
}

void nge_map_set_block(uint8_t tile_id, nge_block_t flags) NGE_BANKED
{
    nge_map_blocks[tile_id] = flags;
}

void nge_map_clip_sprites(void) NGE_BANKED
{
    nge_rect_t clip;

    /* Sprite clip X is in 2-pixel units (see nge_sprite_set_clip) */
    clip.x1 = nge_map.window.x1 * (NGE_MAP_TILE_PIXELS / 2);
    clip.x2 = nge_map.window.x2 * (NGE_MAP_TILE_PIXELS / 2) + (NGE_MAP_TILE_PIXELS / 2 - 1);
    clip.y1 = nge_map.window.y1 * NGE_MAP_TILE_PIXELS;
    clip.y2 = nge_map.window.y2 * NGE_MAP_TILE_PIXELS + (NGE_MAP_TILE_PIXELS - 1);
    nge_sprite_set_clip(&clip);
}
