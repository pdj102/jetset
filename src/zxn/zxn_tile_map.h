#ifndef ZXN_TILE_MAP_H
#define ZXN_TILE_MAP_H

/**
 * @file zxn_tile_map.h
 * @brief ZX Next tilemap and tile-definition types and operations.
 */

#include <stdint.h>

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
void zxn_tile_map_init(void);

/**
 * @brief Fill the tilemap with the specified tile entry.
 * @param tile_p Pointer to the tile entry to copy to every map position.
 */
void zxnext_tilemap_clear(const zxn_tile_t *tile_p);

/**
 * @brief Set a tilemap position to the specified tile entry.
 * @param x Horizontal tile coordinate.
 * @param y Vertical tile coordinate.
 * @param tile_p Pointer to the tile entry to place at the position.
 */
void zxnext_tilemap_set(uint8_t x, uint8_t y, const zxn_tile_t *tile_p);

/**
 * @brief Set the attribute byte at a tilemap position.
 * @param x Horizontal tile coordinate.
 * @param y Vertical tile coordinate.
 * @param tile_attr Attribute byte to write.
 */
void zxnext_tilemap_set_attr(uint8_t x, uint8_t y, uint8_t tile_attr);


#endif
