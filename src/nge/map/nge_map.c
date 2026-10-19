/*
 * Map functions in common memory: those used every frame or every move.
 * The set-up functions are banked, in nge_map_setup.c; the data is in slot
 * 1 (nge_map_cdata.c).
 */
#include "map/nge_map.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"

/* Clamp a camera coordinate to 0..max */
static uint8_t clamp(int16_t v, uint8_t max)
{
    if (v < 0) {
        return 0;
    }
    return (v > max) ? max : (uint8_t)v;
}

/* Move the camera to an already clamped position */
static uint8_t camera_to(uint8_t x, uint8_t y)
{
    if (x == nge_map.cam_x && y == nge_map.cam_y) {
        return 0;
    }
    nge_map.cam_x = x;
    nge_map.cam_y = y;
    nge_map.dirty = 1;
    return 1;
}

uint8_t nge_map_set_camera(uint8_t x, uint8_t y)
{
    return camera_to(clamp(x, nge_map.width - nge_map.view_w),
                     clamp(y, nge_map.height - nge_map.view_h));
}

uint8_t nge_map_move_camera(int8_t dx, int8_t dy)
{
    return camera_to(clamp((int16_t)nge_map.cam_x + dx, nge_map.width - nge_map.view_w),
                     clamp((int16_t)nge_map.cam_y + dy, nge_map.height - nge_map.view_h));
}

uint8_t nge_map_centre_camera(uint8_t x, uint8_t y)
{
    uint8_t half_w = nge_map.view_w >> 1;
    uint8_t half_h = nge_map.view_h >> 1;

    return nge_map_set_camera((x > half_w) ? x - half_w : 0,
                              (y > half_h) ? y - half_h : 0);
}

uint8_t nge_map_within(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t r)
{
    uint8_t dx = (x1 > x0) ? (uint8_t)(x1 - x0) : (uint8_t)(x0 - x1);
    uint8_t dy = (y1 > y0) ? (uint8_t)(y1 - y0) : (uint8_t)(y0 - y1);

    /* Fits in 16 bits: dx*dx + dy*dy by nge_map_within_check, r*r + r at most 65280 */
    return (uint16_t)dx * dx + (uint16_t)dy * dy <= (uint16_t)r * r + r;
}

const nge_tile_t *nge_map_get(uint8_t x, uint8_t y)
{
    nge_util_assert(x < nge_map.width && y < nge_map.height);

    return &NGE_MAP_TILE(x, y);
}

void nge_map_set(uint8_t x, uint8_t y, const nge_tile_t *tile)
{
    nge_util_assert(x < nge_map.width && y < nge_map.height);
    nge_util_assert(tile != NULL);

    NGE_MAP_TILE(x, y) = *tile;

    /* Only a change inside the view needs a redraw */
    if (x >= nge_map.cam_x && x < nge_map.cam_x + nge_map.view_w &&
        y >= nge_map.cam_y && y < nge_map.cam_y + nge_map.view_h) {
        nge_map.dirty = 1;
    }
}

nge_block_t nge_map_block(uint8_t x, uint8_t y)
{
    if (x >= nge_map.width || y >= nge_map.height) {
        return NGE_BLOCK_EDGE;
    }
    return NGE_MAP_BLOCK(x, y);
}

void nge_map_invalidate(void)
{
    nge_map.dirty = 1;
}

void nge_map_update(void)
{
    if (nge_map.dirty) {
        nge_map_draw();
    }
}

void nge_map_draw(void)
{
    const nge_tile_t *src = NGE_MAP_ROW(nge_map.cam_y) + nge_map.cam_x;
    nge_tile_t *dst = nge_tilemap_cell(nge_map.window.x1, nge_map.window.y1);
    uint16_t bytes = (uint16_t)nge_map.view_w * sizeof(nge_tile_t);
    uint8_t rows = nge_map.view_h;

    /* One memcpy (a single LDIR) per row */
    while (rows--) {
        memcpy(dst, src, bytes);
        src += NGE_MAP_STRIDE;
        dst += NGE_TILEMAP_WIDTH;
    }
    nge_map.dirty = 0;
}

uint8_t nge_map_to_screen(uint16_t map_x, uint16_t map_y, uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t view_x = (uint16_t)nge_map.cam_x * NGE_MAP_TILE_PIXELS;
    uint16_t view_y = (uint16_t)nge_map.cam_y * NGE_MAP_TILE_PIXELS;

    /*
     * Positions left of or above the view wrap round to large values, which
     * the sprite hardware's 9-bit coordinates show as just off the left or
     * top edge, so a sprite partly in view is still placed correctly.
     */
    *screen_x = (uint16_t)nge_map.window.x1 * NGE_MAP_TILE_PIXELS + map_x - view_x;
    *screen_y = (uint16_t)nge_map.window.y1 * NGE_MAP_TILE_PIXELS + map_y - view_y;

    return map_x >= view_x && map_x < view_x + (uint16_t)nge_map.view_w * NGE_MAP_TILE_PIXELS &&
           map_y >= view_y && map_y < view_y + (uint16_t)nge_map.view_h * NGE_MAP_TILE_PIXELS;
}
