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
 * Blocking or not:
 *
 * | Function           | Blocking? | Returns                                        |
 * |--------------------|-----------|------------------------------------------------|
 * | nge_line_begin()   | no        | at once: an empty line is ready to edit         |
 * | nge_line_update()  | no        | at once: 1 if ENTER ended the line, 0 if not    |
 * | nge_getc_wait()    | yes       | when a character arrives                        |
 * | nge_gets()         | yes       | when ENTER ends the line                        |
 *
 * Non-blocking functions use only the input already waiting and return at
 * once. Call nge_line_update() once per frame from the game's frame loop
 * until it returns 1: the loop keeps running, so sprites animate and the
 * map and screen keep updating while the player types.
 *
 * Blocking functions don't return until the input arrives. While they wait
 * they run frames themselves, calling nge_frame_wait_vblank() and
 * nge_input_update() and nothing else: the game's own frame work (the
 * render system, map updates, animation) does not run, so the screen stands
 * still. Use them for modal prompts, such as asking for a name at start-up.
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
 * @brief Non-blocking: start editing an empty line.
 * @param buf Buffer for the line, in common memory.
 * @param size Buffer size, including the 0 (at least 1).
 * @param in Input stream.
 * @param echo Output stream for the echo, or NULL for none.
 */
void nge_line_begin(nge_line_t *l, char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo);

/**
 * @brief Non-blocking: edit the line with every character already waiting
 *        in the input stream, then return without waiting for more.
 *
 * Call once per frame until it returns 1. Characters after the ENTER stay
 * in the stream.
 * @return 1 when ENTER ends the line (the ENTER is not echoed), 0 otherwise.
 */
uint8_t nge_line_update(nge_line_t *l);

/**
 * @brief Blocking: read a character, waiting until one arrives.
 *
 * While it waits it runs frames (nge_frame_wait_vblank() and
 * nge_input_update() only). nge_getc() is the non-blocking version.
 * @param in Input stream (asserts if it is not an input stream, which
 *           would never return a character).
 * @return The character.
 */
int nge_getc_wait(NGE_FILE *in);

/**
 * @brief Blocking: read a line, waiting until ENTER ends it.
 *
 * While it waits it runs frames (nge_frame_wait_vblank() and
 * nge_input_update() only). nge_line_begin() and nge_line_update() are the
 * non-blocking version.
 * @see nge_line_begin() for the parameters.
 */
void nge_gets(char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo);

#endif
