#ifndef NGE_TM_STREAM_H
#define NGE_TM_STREAM_H

/**
 * @file nge_tm_stream.h
 * @brief Output streams on tilemap windows.
 *
 * Tilemap streams write the character code directly as the tile id, with
 * the attribute byte the stream holds, and interpret the text control codes
 * (io/nge_text.h): INK sets the attribute's palette offset (bits 7-4), ATTR
 * the whole attribute byte. Clearing writes tile 0x20. So the tile
 * definitions must hold a font at its character codes: Jetset's hold the
 * map's metatiles instead (src/data/tile_defs_data.asm), and it writes text
 * on Layer 2 (io/nge_l2_stream.h).
 */

#include <stdint.h>

#include "core/nge_util.h"
#include "io/nge_stream.h"
#include "io/nge_text.h"

/**
 * @brief Open an output stream on a tilemap window.
 *
 * The cursor starts at the top-left of the window. The window is not cleared.
 *
 * @param area Window in inclusive tile coordinates.
 * @param attr Initial attribute byte for written tiles.
 * @return The stream, or NULL if no stream slot is free.
 */
NGE_FILE *nge_open_tilemap(const nge_rect_t *area, uint8_t attr);

/**
 * @brief Load the text colour banks (NGE_INK_*) into tilemap palette 0.
 *
 * Font tiles use pixel value 0 for the background (transparent) and 2 for
 * ink; a few also use 1 or 3, so entries 1-3 of each bank are the ink colour.
 */
void nge_tm_load_ink_palette(void);

#endif
