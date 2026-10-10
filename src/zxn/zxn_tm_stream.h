#ifndef ZXN_TM_STREAM_H
#define ZXN_TM_STREAM_H

/**
 * @file zxn_tm_stream.h
 * @brief Output streams on tilemap windows.
 *
 * Tilemap streams write the character code directly as the tile id and
 * interpret Spectrum-style control codes:
 *
 * | Code | Parameters | Action                                         |
 * |------|------------|------------------------------------------------|
 * | 0x08 |            | Backspace (stops at the left edge)             |
 * | 0x0A |            | New line, scrolling the window at the bottom   |
 * | 0x0C |            | Clear the window to tile 0x20 and home cursor  |
 * | 0x0D |            | Carriage return                                |
 * | 0x10 | n          | INK: set palette offset (attribute bits 7-4)   |
 * | 0x11 | n          | ATTR: set the whole attribute byte             |
 * | 0x16 | y, x       | AT: move cursor, relative to the window        |
 *
 * Other codes below 0x20 are ignored.
 */

#include <stdint.h>

#include "zxn_stream.h"
#include "zxn_util.h"

#define ZXN_CTRL_BS    0x08 /**< Backspace. */
#define ZXN_CTRL_LF    0x0A /**< New line. */
#define ZXN_CTRL_CLS   0x0C /**< Clear window. */
#define ZXN_CTRL_CR    0x0D /**< Carriage return. */
#define ZXN_CTRL_INK   0x10 /**< INK n. */
#define ZXN_CTRL_ATTR  0x11 /**< ATTR n. */
#define ZXN_CTRL_AT    0x16 /**< AT y, x. */

/**
 * @brief Open an output stream on a tilemap window.
 *
 * The cursor starts at the top-left of the window. The window is not cleared.
 *
 * @param area Window in inclusive tile coordinates.
 * @param attr Initial attribute byte for written tiles.
 * @return The stream, or NULL if no stream slot is free.
 */
ZXN_FILE *zxn_open_tilemap(const zxn_rect_t *area, uint8_t attr);

/**
 * @brief Set the ink colour (palette offset 0-15) for following characters.
 * @param n Palette offset.
 * @param stream Open tilemap stream.
 */
void zxn_tm_ink(uint8_t n, ZXN_FILE *stream);

/**
 * @brief Set the whole attribute byte for following characters.
 * @param a Attribute byte.
 * @param stream Open tilemap stream.
 */
void zxn_tm_attr(uint8_t a, ZXN_FILE *stream);

/**
 * @brief Move the cursor, relative to the window.
 * @param y Row.
 * @param x Column.
 * @param stream Open tilemap stream.
 */
void zxn_tm_at(uint8_t y, uint8_t x, ZXN_FILE *stream);

/**
 * @brief Clear the window and home the cursor.
 * @param stream Open tilemap stream.
 */
void zxn_tm_cls(ZXN_FILE *stream);

#endif
