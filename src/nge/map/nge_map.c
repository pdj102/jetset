#include "map/nge_map.h"

#include <string.h>

#include "sprite/nge_sprite.h"

/* Clamp a camera coordinate to 0..max */
static uint8_t clamp(int16_t v, uint8_t max)
{
    if (v < 0) {
        return 0;
    }
    return (v > max) ? max : (uint8_t)v;
}

/* Move the camera to an already clamped position */
static uint8_t camera_to(nge_map_t *m, uint8_t x, uint8_t y)
{
    if (x == m->cam_x && y == m->cam_y) {
        return 0;
    }
    m->cam_x = x;
    m->cam_y = y;
    m->dirty = 1;
    return 1;
}

/* The buffer (tiles, then the block table) fits in an 8K page */
typedef char nge_map_buffer_check[(NGE_MAP_BUFFER_BYTES <= 8192) ? 1 : -1];

void nge_map_init(nge_map_t *m, nge_tile_t *buffer, uint8_t width, uint8_t height,
                  const nge_rect_t *window)
{
    nge_util_assert(m != NULL && buffer != NULL && window != NULL);
    nge_util_assert(window->x1 <= window->x2 && window->x2 < NGE_TILEMAP_WIDTH);
    nge_util_assert(window->y1 <= window->y2 && window->y2 < NGE_TILEMAP_HEIGHT);

    m->tiles = buffer;
    m->block = (nge_block_t *)buffer + NGE_MAP_MAX_BYTES;
    m->width = width;
    m->height = height;
    m->window = *window;
    m->view_w = window->x2 - window->x1 + 1;
    m->view_h = window->y2 - window->y1 + 1;

    /* The view must fit inside the map, and the map in its memory */
    nge_util_assert(width >= m->view_w && width <= NGE_MAP_STRIDE);
    nge_util_assert(height >= m->view_h && height <= NGE_MAP_MAX_HEIGHT);

    m->cam_x = 0;
    m->cam_y = 0;
    m->dirty = 1;
}

uint8_t nge_map_set_camera(nge_map_t *m, uint8_t x, uint8_t y)
{
    return camera_to(m, clamp(x, m->width - m->view_w), clamp(y, m->height - m->view_h));
}

uint8_t nge_map_move_camera(nge_map_t *m, int8_t dx, int8_t dy)
{
    return camera_to(m, clamp((int16_t)m->cam_x + dx, m->width - m->view_w),
                        clamp((int16_t)m->cam_y + dy, m->height - m->view_h));
}

uint8_t nge_map_centre_camera(nge_map_t *m, uint8_t x, uint8_t y)
{
    uint8_t half_w = m->view_w >> 1;
    uint8_t half_h = m->view_h >> 1;

    return nge_map_set_camera(m, (x > half_w) ? x - half_w : 0,
                                 (y > half_h) ? y - half_h : 0);
}

const nge_tile_t *nge_map_get(const nge_map_t *m, uint8_t x, uint8_t y)
{
    nge_util_assert(x < m->width && y < m->height);

    return &NGE_MAP_TILE(m, x, y);
}

void nge_map_set(nge_map_t *m, uint8_t x, uint8_t y, const nge_tile_t *tile)
{
    nge_util_assert(x < m->width && y < m->height);
    nge_util_assert(tile != NULL);

    NGE_MAP_TILE(m, x, y) = *tile;

    /* Only a change inside the view needs a redraw */
    if (x >= m->cam_x && x < m->cam_x + m->view_w &&
        y >= m->cam_y && y < m->cam_y + m->view_h) {
        m->dirty = 1;
    }
}

void nge_map_update(nge_map_t *m)
{
    if (m->dirty) {
        nge_map_draw(m);
    }
}

void nge_map_draw(nge_map_t *m)
{
    const nge_tile_t *src = NGE_MAP_ROW(m, m->cam_y) + m->cam_x;
    nge_tile_t *dst = nge_tilemap_cell(m->window.x1, m->window.y1);
    uint16_t bytes = (uint16_t)m->view_w * sizeof(nge_tile_t);
    uint8_t rows = m->view_h;

    /* One memcpy (a single LDIR) per row */
    while (rows--) {
        memcpy(dst, src, bytes);
        src += NGE_MAP_STRIDE;
        dst += NGE_TILEMAP_WIDTH;
    }
    m->dirty = 0;
}

uint8_t nge_map_to_screen(const nge_map_t *m, uint16_t map_x, uint16_t map_y,
                          uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t view_x = (uint16_t)m->cam_x * NGE_MAP_TILE_PIXELS;
    uint16_t view_y = (uint16_t)m->cam_y * NGE_MAP_TILE_PIXELS;

    /*
     * Positions left of or above the view wrap round to large values, which
     * the sprite hardware's 9-bit coordinates show as just off the left or
     * top edge, so a sprite partly in view is still placed correctly.
     */
    *screen_x = (uint16_t)m->window.x1 * NGE_MAP_TILE_PIXELS + map_x - view_x;
    *screen_y = (uint16_t)m->window.y1 * NGE_MAP_TILE_PIXELS + map_y - view_y;

    return map_x >= view_x && map_x < view_x + (uint16_t)m->view_w * NGE_MAP_TILE_PIXELS &&
           map_y >= view_y && map_y < view_y + (uint16_t)m->view_h * NGE_MAP_TILE_PIXELS;
}

void nge_map_clip_sprites(const nge_map_t *m)
{
    nge_rect_t clip;

    /* Sprite clip X is in 2-pixel units (see nge_sprite_set_clip) */
    clip.x1 = m->window.x1 * (NGE_MAP_TILE_PIXELS / 2);
    clip.x2 = m->window.x2 * (NGE_MAP_TILE_PIXELS / 2) + (NGE_MAP_TILE_PIXELS / 2 - 1);
    clip.y1 = m->window.y1 * NGE_MAP_TILE_PIXELS;
    clip.y2 = m->window.y2 * NGE_MAP_TILE_PIXELS + (NGE_MAP_TILE_PIXELS - 1);
    nge_sprite_set_clip(&clip);
}

void nge_map_clear_blocks(nge_map_t *m, nge_block_t flags)
{
    memset(m->block, flags, NGE_MAP_BLOCK_ENTRIES);
}

void nge_map_set_block(nge_map_t *m, uint8_t tile_id, nge_block_t flags)
{
    m->block[tile_id] = flags;
}

nge_block_t nge_map_block(const nge_map_t *m, uint8_t x, uint8_t y)
{
    if (x >= m->width || y >= m->height) {
        return NGE_BLOCK_EDGE;
    }
    return NGE_MAP_BLOCK(m, x, y);
}
