#ifndef ZXN_TILE_MAP_H
#define ZXN_TILE_MAP_H

/**
 * @file zxn_tile_map.h
 * @brief ZX Next tilemap and tile-definition types and operations.
 */

#include <stdint.h>


#define ZXN_TILEMAP_X_OFFSET_MAX    319

/**
 * @brief A tile entry in the ZX Next tilemap.
 */
typedef struct zxn_tile {
    /** ID of the tile definition displayed at this map position. */
    uint8_t tile_id;

    /** Attribute byte controlling this tile's display properties. */
    uint8_t tile_attr;
} zxn_tile_t;

/**
 * @brief Inclusive rectangle coordinates on the ZX Next tilemap.
 *
 * Used in pixels for the clip window and in tiles for tilemap areas.
 */
typedef struct zxn_rect {
    uint8_t x1;
    uint8_t x2;
    uint8_t y1;
    uint8_t y2;
} zxn_rect_t;

/**
 * @brief An 8x8 ZX Next tile definition containing 4-bit pixel indices.
 *
 * Pixels are stored in row-major order. Each byte contains two adjacent
 * pixel indices, with the earlier pixel in the high nibble and the next pixel
 * in the low nibble.
 */
typedef struct zxn_tile_definition {
    /** Packed pixel indices, with two pixels stored in each byte. */
    uint8_t pixels[32];
} zxn_tile_definition_t;

/**
 * @brief Initialize the ZX Next tilemap hardware.
 */
void zxn_tilemap_init(void);

/**
 * @brief Set the inclusive tilemap clip window in pixels (default is 0,159,0,255 after a Reset).
 * 
 * The X coordinates are internally doubled (40x32 mode) or quadrupled (80x32 mode)
 * origin [0,0] is 32* pixels left and above the top-left ULA pixel, i.e. Tilemap mode does use same coordinates as Sprites
 *  
 * @param r Clip coordinates in hardware order: X1, X2, Y1, Y2.
 */
void zxn_tilemap_set_clip(const zxn_rect_t *r);

/**
 * @brief Set the tilemap scroll offset in pixels.
 * @param x Horizontal offset (0-319 in 40-column mode, 0-639 in 80-column mode).
 * @param y Vertical offset (0-255).
 */
void zxn_tilemap_set_scroll(uint16_t x, uint8_t y);

/**
 * @brief Fill the tilemap with the specified tile entry.
 * @param tile_p Pointer to the tile entry to copy to every map position.
 */
void zxn_tilemap_clear(const zxn_tile_t *tile_p);

/**
 * @brief Set a tilemap position to the specified tile entry.
 * @param x Horizontal tile coordinate.
 * @param y Vertical tile coordinate.
 * @param tile_p Pointer to the tile entry to place at the position.
 */
void zxn_tilemap_set(uint8_t x, uint8_t y, const zxn_tile_t *tile_p);

/**
 * @brief Set the attribute byte at a tilemap position.
 * @param x Horizontal tile coordinate.
 * @param y Vertical tile coordinate.
 * @param tile_attr Attribute byte to write.
 */
void zxn_tilemap_set_attr(uint8_t x, uint8_t y, uint8_t tile_attr);

/**
 * @brief Fill a tilemap area with the specified tile entry.
 * @param r Area in inclusive tile coordinates.
 * @param fill_p Pointer to the tile entry to copy to every position in the area.
 */
void zxn_tilemap_fill(const zxn_rect_t *r, const zxn_tile_t *fill_p);

/**
 * @brief Scroll a tilemap area up one row and fill the bottom row.
 * @param r Area in inclusive tile coordinates.
 * @param fill_p Pointer to the tile entry used to fill the bottom row.
 */
void zxn_tilemap_scroll_up(const zxn_rect_t *r, const zxn_tile_t *fill_p);


#endif
