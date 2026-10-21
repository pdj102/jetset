#ifndef NGE_LAYER2_H
#define NGE_LAYER2_H

/**
 * @file nge_layer2.h
 * @brief Layer 2 at 320 x 256, 8 bits a pixel, as a layer of 8 x 8 cells
 *        for text and panels over the tilemap and sprites.
 *
 * Layer 2 is 80K, 10 pages from NGE_LAYER2_PAGE (16K banks 8 to 12), which
 * the display reads directly whatever the MMU shows. It is written only
 * through the MMU (N-DATA-4): each function maps the page it writes into
 * slot 7 and maps the old page back before it returns.
 *
 * At 320 x 256 the pixels are stored by column: pixel (x, y) is byte
 * x * 256 + y, so an 8K page holds 32 columns, 4 cells across. The cells
 * match the tilemap's grid (40 x 32), so cell (col, row) covers tilemap
 * tile (col, row).
 *
 * Colours are RRRGGGBB, through an identity palette (nge_layer2_init()).
 * NGE_LAYER2_TRANSPARENT (the global transparency colour) shows the layers
 * underneath. Text is drawn from the module's own 8 x 8 font, a bitmask a
 * pixel column (nge_layer2_font.c, page 90), for characters 32 to 127
 * (others draw as a space).
 *
 * Banking: banked (NGE_BANKED, page 90). Cells are passed by value, so no
 * caller passes a pointer into its own page. Strings passed in must be in
 * common memory (the stack, or common data): the caller's page is mapped
 * out while they are read (N-BANK-5).
 */

#include <stdint.h>

#include "core/nge_bank.h"

#define NGE_LAYER2_PAGE         16     /**< First of its 10 pages (bank 8). */
#define NGE_LAYER2_PAGES        10     /**< 80K: 320 x 256 bytes. */
#define NGE_LAYER2_COLS         40     /**< Cells across. */
#define NGE_LAYER2_ROWS         32     /**< Cells down. */
#define NGE_LAYER2_TRANSPARENT  0xE3   /**< The global transparency colour, set by nge_layer2_init(). */

/**
 * @brief Set up Layer 2: 320 x 256 in pages 16 to 25, no scroll, no clip,
 *        an identity palette, every pixel transparent, shown, and drawn on
 *        top of the sprites and the tilemap. Clearing all 80K takes a few
 *        frames.
 */
void nge_layer2_init(void) NGE_BANKED;

/** @brief Show (1) or hide (0) Layer 2, and put the layer order back to sprites on top when hidden. */
void nge_layer2_show(uint8_t on) NGE_BANKED;

/** @brief Fill the cells from (x1, y1) to (x2, y2), inclusive, with a colour. */
void nge_layer2_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) NGE_BANKED;

/** @brief Draw character c in cell (col, row): ink on paper. */
void nge_layer2_glyph(uint8_t col, uint8_t row, uint8_t c, uint8_t ink, uint8_t paper) NGE_BANKED;

/**
 * @brief Draw string s (in common memory) from cell (col, row), one cell a
 *        character, cut at the right edge. ink on paper.
 */
void nge_layer2_text(uint8_t col, uint8_t row, const char *s, uint8_t ink, uint8_t paper) NGE_BANKED;

/** @brief Scroll the cells from (x1, y1) to (x2, y2) up one row; the bottom row is filled with a colour. */
void nge_layer2_scroll_up(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill) NGE_BANKED;

#endif
