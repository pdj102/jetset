#ifndef NGE_STREAM_DEV_H
#define NGE_STREAM_DEV_H

/**
 * @file nge_stream_dev.h
 * @brief Stream internals for device drivers. Not for application code.
 *
 * Adding a device: give it a state type and a member in the union below, and
 * a driver file that opens streams with nge_stream_alloc().
 */

#include <stdint.h>

#include "io/nge_stream.h"
#include "tilemap/nge_tilemap.h"
#include "core/nge_util.h"

/** Device-specific character output function. */
typedef int (*nge_put_fn)(uint8_t c, NGE_FILE *stream);

/** Device-specific character input function: a character, or NGE_EOF if none is waiting. */
typedef int (*nge_get_fn)(NGE_FILE *stream);

/** Device-specific function that sends any buffered output. */
typedef void (*nge_flush_fn)(NGE_FILE *stream);

/**
 * @brief State of a tilemap window stream.
 */
typedef struct nge_tm_state {
    nge_rect_t area; /**< Window in inclusive tile coordinates. */
    uint8_t cx;      /**< Cursor column (absolute); x2 + 1 means a wrap is pending. */
    uint8_t cy;      /**< Cursor row (absolute tile coordinate). */
    nge_tile_t *cell; /**< Tilemap address of the cursor position. */
    uint8_t attr;    /**< Attribute byte for written tiles. */
    uint8_t esc;     /**< Control-code state machine state. */
    uint8_t arg;     /**< Saved first parameter of AT. */
    uint8_t clip;    /**< Non-zero: clip at the edges, never wrap or scroll. */
} nge_tm_state_t;

/**
 * @brief An output stream.
 */
struct nge_file {
    nge_put_fn put;     /**< Device output function; NULL for an input-only stream. */
    nge_get_fn get;     /**< Device input function; NULL for an output-only stream. */
    nge_flush_fn flush; /**< Device flush function; NULL if unbuffered. */
    union {
        nge_tm_state_t tm; /**< Tilemap window state. */
    } u;
};

/**
 * @brief Claim a free stream slot for a device. A slot is free when it has
 *        neither a put nor a get function.
 * @param put Device output function, or NULL for an input-only device.
 * @param get Device input function, or NULL for an output-only device.
 * @param flush Device flush function, or NULL if the device is unbuffered.
 * @return The stream, or NULL if no slot is free.
 */
NGE_FILE *nge_stream_alloc(nge_put_fn put, nge_get_fn get, nge_flush_fn flush);

#endif
