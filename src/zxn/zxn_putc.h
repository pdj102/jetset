#ifndef ZXN_PUTC_H
#define ZXN_PUTC_H

/**
 * @file zxn_putc.h
 * @brief Character output streams for tilemap windows and the UART.
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

#include "zxn_tile_map.h"

/** Maximum number of streams open at once. */
#ifndef ZXN_FILE_MAX
#define ZXN_FILE_MAX 4
#endif

/** Value returned by zxn_putc() on error. */
#define ZXN_EOF (-1)

#define ZXN_CTRL_BS    0x08 /**< Backspace. */
#define ZXN_CTRL_LF    0x0A /**< New line. */
#define ZXN_CTRL_CLS   0x0C /**< Clear window. */
#define ZXN_CTRL_CR    0x0D /**< Carriage return. */
#define ZXN_CTRL_INK   0x10 /**< INK n. */
#define ZXN_CTRL_ATTR  0x11 /**< ATTR n. */
#define ZXN_CTRL_AT    0x16 /**< AT y, x. */

typedef struct zxn_file ZXN_FILE;

/** Device-specific character output function. */
typedef int (*zxn_put_fn)(uint8_t c, ZXN_FILE *stream);

/**
 * @brief State of a tilemap window stream.
 */
typedef struct zxn_tm_state {
    zxn_rect_t area; /**< Window in inclusive tile coordinates. */
    uint8_t cx;      /**< Cursor column (absolute tile coordinate). */
    uint8_t cy;      /**< Cursor row (absolute tile coordinate). */
    uint8_t attr;    /**< Attribute byte for written tiles. */
    uint8_t esc;     /**< Control-code state machine state. */
    uint8_t arg;     /**< Saved first parameter of AT. */
} zxn_tm_state_t;

/**
 * @brief An output stream. Treat as opaque.
 */
struct zxn_file {
    zxn_put_fn put;  /**< Device output function; NULL for a free slot. */
    union {
        zxn_tm_state_t tm; /**< Tilemap window state. */
    } u;
};

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
 * @brief Open an output stream on the UART.
 * @return The stream, or NULL if no stream slot is free.
 */
ZXN_FILE *zxn_open_uart(void);

/**
 * @brief Write a character to a stream.
 * @param c Character or control code.
 * @param stream Open stream.
 * @return c on success, or ZXN_EOF on error.
 */
int zxn_putc(uint8_t c, ZXN_FILE *stream);

/**
 * @brief Close a stream and free its slot.
 * @param stream Open stream.
 */
void zxn_close(ZXN_FILE *stream);

#endif
