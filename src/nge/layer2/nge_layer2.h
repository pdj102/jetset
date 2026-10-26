#ifndef NGE_LAYER2_H
#define NGE_LAYER2_H

/**
 * @file nge_layer2.h
 * @brief Layer 2 at 320 x 256, 8 bits a pixel, as a layer of 8 x 8 cells
 *        for text and panels over the tilemap and sprites.
 *
 * Layer 2 is 80K, 10 pages, which the display reads directly whatever the
 * MMU shows. There are NGE_LAYER2_BUFFERS such images (buffers): buffer b
 * is the 10 pages from NGE_LAYER2_PAGE + 10b (buffer 0: 16K banks 8 to 12,
 * buffer 1: 13 to 17). One buffer is shown (nge_layer2_show_buffer()), and
 * every drawing call names the buffer it draws on, so a buffer can be drawn
 * while another is shown, then shown at once. Layer 2 is written only
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

#define NGE_LAYER2_PAGE         16     /**< First page of buffer 0 (bank 8). */
#define NGE_LAYER2_PAGES        10     /**< Pages a buffer: 80K, 320 x 256 bytes. */
#define NGE_LAYER2_BUFFERS      2      /**< Buffers: pages 16 to 35. */
#define NGE_LAYER2_COLS         40     /**< Cells across. */
#define NGE_LAYER2_ROWS         32     /**< Cells down. */
#define NGE_LAYER2_TRANSPARENT  0xE3   /**< The global transparency colour, set by nge_layer2_init(). */

/**
 * @brief Set up Layer 2: 320 x 256, buffer 0 shown, no scroll, no clip,
 *        an identity palette, every pixel of every buffer transparent,
 *        shown, and drawn on top of the sprites and the tilemap. Clearing
 *        all 160K takes several frames.
 */
void nge_layer2_init(void) NGE_BANKED;

/** @brief Show (1) or hide (0) Layer 2, and put the layer order back to sprites on top when hidden. */
void nge_layer2_show(uint8_t on) NGE_BANKED;

/**
 * @brief Show buffer b (0 to NGE_LAYER2_BUFFERS - 1) from now on. Call it in
 *        the blanking period, so no frame shows part of each.
 */
void nge_layer2_show_buffer(uint8_t b) NGE_BANKED;

/** @brief Fill the cells from (x1, y1) to (x2, y2), inclusive, of buffer b with a colour. */
void nge_layer2_fill(uint8_t b, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) NGE_BANKED;

/** @brief Draw character c in cell (col, row) of buffer b: ink on paper. */
void nge_layer2_glyph(uint8_t b, uint8_t col, uint8_t row, uint8_t c, uint8_t ink, uint8_t paper) NGE_BANKED;

/**
 * @brief Draw string s (in common memory) from cell (col, row) of buffer
 *        b, one cell a character, cut at the right edge. ink on paper.
 */
void nge_layer2_text(uint8_t b, uint8_t col, uint8_t row, const char *s, uint8_t ink, uint8_t paper) NGE_BANKED;

/**
 * @brief Scroll the cells from (x1, y1) to (x2, y2) of buffer b up one row;
 *        the bottom row is filled with a colour.
 */
void nge_layer2_scroll_up(uint8_t b, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill) NGE_BANKED;

#endif
