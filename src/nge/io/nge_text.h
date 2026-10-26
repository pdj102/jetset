#ifndef NGE_TEXT_H
#define NGE_TEXT_H

/**
 * @file nge_text.h
 * @brief Text streams' control codes, text colours, and the functions that
 *        send the codes, for any text stream: a tilemap window
 *        (io/nge_tm_stream.h) or Layer 2 cells (io/nge_l2_stream.h).
 *
 * The codes are Spectrum-style, sent in the text itself:
 *
 * | Code | Parameters | Action                                          |
 * |------|------------|-------------------------------------------------|
 * | 0x05 |            | Clear from the cursor to the end of the line    |
 * | 0x08 |            | Backspace (stops at the left edge)              |
 * | 0x0A |            | New line, scrolling the window at the bottom    |
 * | 0x0C |            | Clear the window and home the cursor            |
 * | 0x0D |            | Carriage return                                 |
 * | 0x10 | n          | INK: a text colour (NGE_INK_*)                  |
 * | 0x11 | n          | ATTR: the device's raw colour or attribute byte |
 * | 0x12 | n          | CLIP: n != 0 clips at the edges, 0 wraps/scrolls |
 * | 0x13 | n          | PAPER: the device's raw background colour       |
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

#include "io/nge_stream.h"

#define NGE_CTRL_CLREOL 0x05 /**< Clear to end of line. */
#define NGE_CTRL_BS    0x08 /**< Backspace. */
#define NGE_CTRL_LF    0x0A /**< New line. */
#define NGE_CTRL_CLS   0x0C /**< Clear window. */
#define NGE_CTRL_CR    0x0D /**< Carriage return. */
#define NGE_CTRL_INK   0x10 /**< INK n. */
#define NGE_CTRL_ATTR  0x11 /**< ATTR n. */
#define NGE_CTRL_CLIP  0x12 /**< CLIP n. */
#define NGE_CTRL_PAPER 0x13 /**< PAPER n. */
#define NGE_CTRL_AT    0x16 /**< AT y, x. */

/**
 * @name Text colours
 * For nge_text_ink(). A tilemap stream shows them through the palette banks
 * of the same numbers (nge_tm_load_ink_palette()), and the map's tiles use
 * them too; a Layer 2 stream maps them to its colours.
 * @{
 */
#define NGE_INK_WHITE   12
#define NGE_INK_YELLOW  13
#define NGE_INK_CYAN    14
#define NGE_INK_RED     15
/** @} */

/** @brief Set the text colour (NGE_INK_*) for following characters. */
void nge_text_ink(uint8_t n, NGE_FILE *stream);

/** @brief Set the device's raw colour or attribute byte for following characters. */
void nge_text_attr(uint8_t a, NGE_FILE *stream);

/**
 * @brief Set the device's raw background colour for following characters
 *        and clearing: a Layer 2 stream's paper (RRRGGGBB). A tilemap stream
 *        has no paper, and ignores it.
 */
void nge_text_paper(uint8_t p, NGE_FILE *stream);

/** @brief Move the cursor to row y, column x, relative to the window. */
void nge_text_at(uint8_t y, uint8_t x, NGE_FILE *stream);

/** @brief Clear the window and home the cursor. */
void nge_text_cls(NGE_FILE *stream);

/**
 * @brief Clear from the cursor to the end of the line; the cursor does not
 *        move. Use after redrawing a value in place, so a shorter value
 *        fully replaces a longer one.
 */
void nge_text_clreol(NGE_FILE *stream);

/** @brief Clip at the window's edges and never scroll (1), or wrap and scroll (0). */
void nge_text_clip(uint8_t on, NGE_FILE *stream);

#endif
