#include <stdio.h>
#include <arch/zxn.h>

#include "zxn/zxn_frame.h"
#include "zxn/zxn_hw.h"
#include "zxn/zxn_palette.h"
#include "zxn/zxn_tile_map.h"

static const zxn_palette_entry_9_t tile_palette[] = {
    { 0x00, 0x00 },
    { 0xE0, 0x00 },
    { 0x1F, 0x00 },
    { 0xFF, 0x00 },
};

static const zxn_tile_definition_t blank_tile = {{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
}};

static const zxn_tile_definition_t wall_tile = {{
    0x11, 0x11, 0x11, 0x11,
    0x12, 0x22, 0x22, 0x21,
    0x12, 0x32, 0x23, 0x21,
    0x12, 0x22, 0x22, 0x21,
    0x11, 0x11, 0x11, 0x11,
    0x12, 0x22, 0x22, 0x21,
    0x12, 0x32, 0x23, 0x21,
    0x12, 0x22, 0x22, 0x21,
}};

static const zxn_tile_t wall_tile_entry = {
    .tile_id = 0,
    .tile_attr = 0,
};

static const zxn_tile_t blank_tile_entry = {
    .tile_id = 1,
    .tile_attr = 0,
};

int main(void)
{
    uint16_t scroll_x = 0;
    uint8_t scroll_y = 0;
    uint8_t tile_x;
    uint8_t tile_y;

    zxn_rect_t clip_window = {
        .x1 = 0,
        .x2 = 20*8 - 1,
        .y1 = 0,
        .y2 = 32*8 - 1,
    };

    zxn_hw_init();
    zxn_write_palette9( RPC_SELECT_TILEMAP_PALETTE_0, tile_palette, 4);


    zxn_tilemap_init();

    for (tile_y = 0; tile_y < 32; tile_y++)
    {
        for (tile_x = 0; tile_x < 40; tile_x++)
        {
            const zxn_tile_t *tile = ((tile_x + tile_y) & 1)
                ? &wall_tile_entry
                : &blank_tile_entry;
            zxnext_tilemap_set(tile_x, tile_y, tile);
        }
    }

    zxn_tilemap_set_scroll(0, 0);

    // zxn_tilemap_set_clip(&clip_window);
    while (1)
    {
        zxn_frame_wait_vblank();
        zxn_tilemap_set_scroll(scroll_x, scroll_y);

        scroll_x = (scroll_x == ZXN_TILEMAP_X_OFFSET_MAX) ? 0 : scroll_x + 1;
        scroll_y++;
    }

}