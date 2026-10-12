#ifndef NGE_TM_STREAM_H
#define NGE_TM_STREAM_H

/**
 * @file nge_tm_stream.h
 * @brief Output streams on tilemap windows.
 *
 * Tilemap streams write the character code directly as the tile id and
 * interpret Spectrum-style control codes:
 *
 * | Code | Parameters | Action                                          |
 * |------|------------|-------------------------------------------------|
 * | 0x05 |            | Clear from the cursor to the end of the line    |
 * | 0x08 |            | Backspace (stops at the left edge)              |
 * | 0x0A |            | New line, scrolling the window at the bottom    |
 * | 0x0C |            | Clear the window to tile 0x20 and home cursor   |
 * | 0x0D |            | Carriage return                                 |
 * | 0x10 | n          | INK: set palette offset (attribute bits 7-4)    |
 * | 0x11 | n          | ATTR: set the whole attribute byte              |
 * | 0x12 | n          | CLIP: n != 0 clips at the edges, 0 wraps/scrolls |
 * | 0x16 | y, x       | AT: move cursor, relative to the window         |
 *
 * Other codes below 0x20 are ignored.
 *
 * Wrapping is delayed: writing the last column leaves the cursor just past
 * the right edge, and the wrap happens only when the next character arrives.
 * A line that exactly fills the width therefore never scrolls the window by
 * itself, and a following '\n' does not leave a blank line.
 *
 * In clip mode (for status bars and score fields) characters past the right
 * edge are dropped and '\n' on the bottom row does not scroll, so the window
 * contents never move. Streams open in wrap/scroll mode.
 */

#include <stdint.h>

#include "output/nge_stream.h"
#include "core/nge_util.h"

#define NGE_CTRL_CLREOL 0x05 /**< Clear to end of line. */
#define NGE_CTRL_BS    0x08 /**< Backspace. */
#define NGE_CTRL_LF    0x0A /**< New line. */
#define NGE_CTRL_CLS   0x0C /**< Clear window. */
#define NGE_CTRL_CR    0x0D /**< Carriage return. */
#define NGE_CTRL_INK   0x10 /**< INK n. */
#define NGE_CTRL_ATTR  0x11 /**< ATTR n. */
#define NGE_CTRL_CLIP  0x12 /**< CLIP n. */
#define NGE_CTRL_AT    0x16 /**< AT y, x. */

/**
 * @name Text colours
 * Palette banks (attribute bits 7-4) reserved for text in tilemap palette 0,
 * for use with nge_tm_ink(). Load them with nge_tm_load_ink_palette(). Banks
 * 0-11 are left for game graphics.
 * @{
 */
#define NGE_INK_WHITE   12
#define NGE_INK_YELLOW  13
#define NGE_INK_CYAN    14
#define NGE_INK_RED     15
/** @} */

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
 * @brief Set the ink colour (palette offset 0-15) for following characters.
 * @param n Palette offset.
 * @param stream Open tilemap stream.
 */
void nge_tm_ink(uint8_t n, NGE_FILE *stream);

/**
 * @brief Set the whole attribute byte for following characters.
 * @param a Attribute byte.
 * @param stream Open tilemap stream.
 */
void nge_tm_attr(uint8_t a, NGE_FILE *stream);

/**
 * @brief Move the cursor, relative to the window.
 * @param y Row.
 * @param x Column.
 * @param stream Open tilemap stream.
 */
void nge_tm_at(uint8_t y, uint8_t x, NGE_FILE *stream);

/**
 * @brief Clear the window and home the cursor.
 * @param stream Open tilemap stream.
 */
void nge_tm_cls(NGE_FILE *stream);

/**
 * @brief Clear from the cursor to the end of the line; the cursor does not move.
 *
 * Use after redrawing a value in place, so a shorter value fully replaces a
 * longer one.
 *
 * @param stream Open tilemap stream.
 */
void nge_tm_clreol(NGE_FILE *stream);

/**
 * @brief Select clip mode or wrap/scroll mode.
 * @param on Non-zero to clip at the window edges and never scroll.
 * @param stream Open tilemap stream.
 */
void nge_tm_clip(uint8_t on, NGE_FILE *stream);

/**
 * @brief Load the text colour banks (NGE_INK_*) into tilemap palette 0.
 *
 * Font tiles use pixel value 0 for the background (transparent) and 2 for
 * ink; a few also use 1 or 3, so entries 1-3 of each bank are the ink colour.
 */
void nge_tm_load_ink_palette(void);

#endif
