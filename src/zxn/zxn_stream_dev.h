#ifndef ZXN_STREAM_DEV_H
#define ZXN_STREAM_DEV_H

/**
 * @file zxn_stream_dev.h
 * @brief Stream internals for device drivers. Not for application code.
 *
 * Adding a device: give it a state type and a member in the union below, and
 * a driver file that opens streams with zxn_stream_alloc().
 */

#include <stdint.h>

#include "zxn_stream.h"
#include "zxn_util.h"

/** Device-specific character output function. */
typedef int (*zxn_put_fn)(uint8_t c, ZXN_FILE *stream);

/** Device-specific function that sends any buffered output. */
typedef void (*zxn_flush_fn)(ZXN_FILE *stream);

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
 * @brief An output stream.
 */
struct zxn_file {
    zxn_put_fn put;     /**< Device output function; NULL for a free slot. */
    zxn_flush_fn flush; /**< Device flush function; NULL if unbuffered. */
    union {
        zxn_tm_state_t tm; /**< Tilemap window state. */
    } u;
};

/**
 * @brief Claim a free stream slot for a device.
 * @param put Device output function.
 * @param flush Device flush function, or NULL if the device is unbuffered.
 * @return The stream, or NULL if no slot is free.
 */
ZXN_FILE *zxn_stream_alloc(zxn_put_fn put, zxn_flush_fn flush);

#endif
