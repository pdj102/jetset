#ifndef NGE_TYPES_H
#define NGE_TYPES_H

/**
 * @file nge_types.h
 * @brief Basic types shared by the nge library.
 */

#include <stdint.h>

/**
 * @brief Inclusive rectangle coordinates on the ZX Next tilemap.
 *
 * Used in pixels for the clip window and in tiles for tilemap areas.
 */
typedef struct nge_rect {
    uint8_t x1;
    uint8_t x2;
    uint8_t y1;
    uint8_t y2;
} nge_rect_t;

#endif
