#ifndef NGE_LINE_H
#define NGE_LINE_H

/**
 * @file nge_line.h
 * @brief Reading from input streams: a line editor, and blocking reads.
 *
 * The line editor reads characters from any input stream (for example the
 * keyboard, io/nge_kbd_stream.h) and echoes them to an output stream (for
 * example a tilemap window):
 *
 * - printable characters (32-126) are added and echoed, up to the buffer's
 *   size less one; more are ignored;
 * - DELETE (12) or backspace (8, also CAPS+5 and the LEFT key) removes the
 *   last character and its echo;
 * - ENTER (13) ends the line.
 *
 * The buffer always holds a 0-terminated string.
 *
 * Non-blocking: nge_line_begin(), then nge_line_update() once per frame
 * until it returns 1, so the game's frame loop keeps running. Blocking:
 * nge_gets() and nge_getc_wait() run frames themselves
 * (nge_frame_wait_vblank() and nge_input_update(), nothing else), so
 * sprites and the map stand still while they wait.
 */

#include <stdint.h>

#include "io/nge_stream.h"

/** A line being edited. Set up with nge_line_begin(); the fields are private. */
typedef struct nge_line {
    char *buf;        /**< The line, 0-terminated. */
    uint8_t size;     /**< Buffer size, including the 0. */
    uint8_t len;      /**< Characters in the line. */
    NGE_FILE *in;     /**< Input stream. */
    NGE_FILE *echo;   /**< Output stream for the echo, or NULL. */
} nge_line_t;

/**
 * @brief Start editing an empty line.
 * @param buf Buffer for the line, in common memory.
 * @param size Buffer size, including the 0 (at least 1).
 * @param in Input stream.
 * @param echo Output stream for the echo, or NULL for none.
 */
void nge_line_begin(nge_line_t *l, char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo);

/**
 * @brief Read and edit with every character waiting, without waiting for more.
 * @return 1 when ENTER ends the line (the ENTER is not echoed), 0 otherwise.
 */
uint8_t nge_line_update(nge_line_t *l);

/**
 * @brief Read a character, running frames until one arrives.
 * @param in Input stream.
 * @return The character.
 */
int nge_getc_wait(NGE_FILE *in);

/**
 * @brief Read a line, running frames until ENTER.
 * @see nge_line_begin() for the parameters.
 */
void nge_gets(char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo);

#endif
