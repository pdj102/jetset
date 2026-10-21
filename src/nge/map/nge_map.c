/*
 * Map functions in common memory: those used every frame or every move.
 * The set-up functions are banked, in nge_map_setup.c; the data is in slot
 * 1 (nge_map_cdata.c).
 */
#include "map/nge_map.h"

#include <stddef.h>

#include "core/nge_util.h"

/* nge_map_draw() draws each map tile as 2 x 2 hardware tiles */
typedef char nge_map_span_check[(NGE_MAP_TILE_SPAN == 2) ? 1 : -1];

/* Clamp a camera coordinate to 0..max */
static uint8_t clamp(int16_t v, uint8_t max)
{
    if (v < 0) {
        return 0;
    }
    return (v > max) ? max : (uint8_t)v;
}

/* Move the camera to an already clamped position: a tile and the pixels into it */
static uint8_t camera_px_to(uint8_t x, uint8_t y, uint8_t fx, uint8_t fy)
{
    uint8_t moved = 0;

    if (x != nge_map.cam_x || y != nge_map.cam_y) {
        nge_map.cam_x = x;
        nge_map.cam_y = y;
        nge_map.dirty = 1;
        moved = 1;
    }
    if (fx != nge_map.fine_x || fy != nge_map.fine_y) {
        nge_map.fine_x = fx;
        nge_map.fine_y = fy;
        nge_map.scroll_dirty = 1;
        moved = 1;
    }
    return moved;
}

/* Move the camera to an already clamped tile, with no fine offset */
static uint8_t camera_to(uint8_t x, uint8_t y)
{
    return camera_px_to(x, y, 0, 0);
}

uint8_t nge_map_set_camera_px(uint16_t x, uint16_t y)
{
    uint16_t max_x = (uint16_t)(nge_map.width - nge_map.view_w) * NGE_MAP_TILE_PIXELS;
    uint16_t max_y = (uint16_t)(nge_map.height - nge_map.view_h) * NGE_MAP_TILE_PIXELS;

    if (x > max_x) {
        x = max_x;
    }
    if (y > max_y) {
        y = max_y;
    }
    /* A fine offset shows the column or row past the window: the tilemap must have room for it */
    nge_util_assert(x % NGE_MAP_TILE_PIXELS == 0 || nge_map.window.x2 + NGE_MAP_TILE_SPAN < NGE_TILEMAP_WIDTH);
    nge_util_assert(y % NGE_MAP_TILE_PIXELS == 0 || nge_map.window.y2 + NGE_MAP_TILE_SPAN < NGE_TILEMAP_HEIGHT);
    return camera_px_to((uint8_t)(x / NGE_MAP_TILE_PIXELS), (uint8_t)(y / NGE_MAP_TILE_PIXELS),
                        (uint8_t)(x % NGE_MAP_TILE_PIXELS), (uint8_t)(y % NGE_MAP_TILE_PIXELS));
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

uint8_t nge_map_centre_camera_px(uint16_t x, uint16_t y)
{
    uint16_t half_w = (uint16_t)(nge_map.view_w >> 1) * NGE_MAP_TILE_PIXELS;
    uint16_t half_h = (uint16_t)(nge_map.view_h >> 1) * NGE_MAP_TILE_PIXELS;

    return nge_map_set_camera_px((x > half_w) ? x - half_w : 0,
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
    if (nge_map.scroll_dirty) {
        nge_tilemap_set_scroll(nge_map.fine_x, nge_map.fine_y);
        nge_map.scroll_dirty = 0;
    }
}

/*
 * Draw a row of cols map tiles from src as two rows of hardware tiles from
 * top: each map tile m is IDs 4m, 4m+1 on the first row and 4m+2, 4m+3 on
 * the second, all with its attribute, whose bit 0 is set to bit 6 of m
 * (bit 8 of the hardware ID, in 512-tile mode)
 */
#ifdef __SDCC
/*
 * Assembly: about 250 T-states a map tile, against about 450 for zsdcc's
 * code, which keeps its pointers in the stack frame. A whole view (15 x 14
 * map tiles with the extra column and row) is about 53,000 T-states, under
 * 2 ms at 28 MHz. Registers: DE the map tiles, HL the tilemap, B the ID, C
 * the attribute; the other set: B the map tiles left. The arguments are on
 * the stack as zsdcc passes them: src, top (words), then cols (a byte)
 */
typedef char nge_map_row_check[(NGE_TILEMAP_WIDTH * sizeof(nge_tile_t) == 80) ? 1 : -1];   /* a row is 80 bytes */

static void draw_row(const nge_tile_t *src, uint8_t *top, uint8_t cols) __naked
{
    (void)src;
    (void)top;
    (void)cols;

    __asm
        ld hl, 2
        add hl, sp
        ld e, (hl)
        inc hl
        ld d, (hl)              ; de = src
        inc hl
        ld c, (hl)
        inc hl
        ld b, (hl)              ; bc = top
        inc hl
        ld a, (hl)              ; a = cols
        or a
        ret z
        exx
        ld b, a                 ; the other set: b = map tiles left
        exx
        ld h, b
        ld l, c                 ; hl = top
    nge_map_draw_tile:
        ld a, (de)              ; a = m
        inc de
        ld b, a
        rlca
        rlca
        and 1
        ld c, a                 ; c = bit 6 of m
        ld a, (de)
        inc de
        and 0xFE
        or c
        ld c, a                 ; c = the attribute
        ld a, b
        add a, a
        add a, a
        ld b, a                 ; b = 4m, low 8 bits
        ld (hl), b              ; top left
        inc hl
        ld (hl), c
        inc hl
        inc b
        ld (hl), b              ; top right
        inc hl
        ld (hl), c
        add hl, 77              ; down a row and back to the left: 80 - 3 bytes
        inc b
        ld (hl), b              ; bottom left
        inc hl
        ld (hl), c
        inc hl
        inc b
        ld (hl), b              ; bottom right
        inc hl
        ld (hl), c
        add hl, -79             ; up a row, to the next map tile: 4 - 83 bytes
        exx
        dec b
        exx
        jr nz, nge_map_draw_tile
        ret
    __endasm;
}
#else
static void draw_row(const nge_tile_t *src, uint8_t *top, uint8_t cols)
{
    uint8_t *bottom = top + NGE_TILEMAP_WIDTH * sizeof(nge_tile_t);

    while (cols--) {
        uint8_t m = src->tile_id;
        uint8_t id = (uint8_t)(m << 2);
        uint8_t attr = (uint8_t)((src->tile_attr & 0xFE) | ((m >> 6) & 1));

        *top++ = id;
        *top++ = attr;
        *top++ = (uint8_t)(id + 1);
        *top++ = attr;
        *bottom++ = (uint8_t)(id + 2);
        *bottom++ = attr;
        *bottom++ = (uint8_t)(id + 3);
        *bottom++ = attr;
        src++;
    }
}
#endif

void nge_map_draw(void)
{
    const nge_tile_t *src = NGE_MAP_ROW(nge_map.cam_y) + nge_map.cam_x;
    uint8_t *dst = (uint8_t *)nge_tilemap_cell(nge_map.window.x1, nge_map.window.y1);
    /*
     * The view, and a column and a row more where the map has them and the
     * tilemap has room past the window: what a fine offset shows
     */
    uint8_t cols = (uint8_t)(nge_map.view_w + (nge_map.cam_x + nge_map.view_w < nge_map.width &&
                                               nge_map.window.x2 + NGE_MAP_TILE_SPAN < NGE_TILEMAP_WIDTH));
    uint8_t rows = (uint8_t)(nge_map.view_h + (nge_map.cam_y + nge_map.view_h < nge_map.height &&
                                               nge_map.window.y2 + NGE_MAP_TILE_SPAN < NGE_TILEMAP_HEIGHT));

    while (rows--) {
        draw_row(src, dst, cols);
        src += NGE_MAP_STRIDE;
        dst += NGE_MAP_TILE_SPAN * NGE_TILEMAP_WIDTH * sizeof(nge_tile_t);
    }
    nge_map.dirty = 0;
}

uint8_t nge_map_to_screen(uint16_t map_x, uint16_t map_y, uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t view_x = NGE_MAP_CAMERA_PX();
    uint16_t view_y = NGE_MAP_CAMERA_PY();

    /*
     * Positions left of or above the view wrap round to large values, which
     * the sprite hardware's 9-bit coordinates show as just off the left or
     * top edge, so a sprite partly in view is still placed correctly.
     */
    *screen_x = (uint16_t)nge_map.window.x1 * NGE_TILEMAP_TILE_PIXELS + map_x - view_x;
    *screen_y = (uint16_t)nge_map.window.y1 * NGE_TILEMAP_TILE_PIXELS + map_y - view_y;

    return map_x >= view_x && map_x < view_x + (uint16_t)nge_map.view_w * NGE_MAP_TILE_PIXELS &&
           map_y >= view_y && map_y < view_y + (uint16_t)nge_map.view_h * NGE_MAP_TILE_PIXELS;
}
