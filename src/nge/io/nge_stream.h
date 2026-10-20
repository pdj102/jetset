#ifndef NGE_STREAM_H
#define NGE_STREAM_H

/**
 * @file nge_stream.h
 * @brief Device-independent character streams, for output and input.
 *
 * Streams are opened by a device driver (see nge_tm_stream.h,
 * nge_l2_stream.h, nge_uart_stream.h and nge_kbd_stream.h). Output streams
 * are written through nge_putc() or the helpers in nge_print.h. Input
 * streams are read with nge_getc(), which is non-blocking (it returns at once, with NGE_EOF
 * if nothing is waiting), or with the line editor and the blocking reads in
 * nge_line.h (nge_getc_wait(), nge_gets()).
 */

#include <stdint.h>

/** Maximum number of streams open at once (the game's screen areas, the keyboard, debug output). */
#ifndef NGE_FILE_MAX
#define NGE_FILE_MAX 6
#endif

/** Value returned on error. */
#define NGE_EOF (-1)

/** A stream. Opaque outside device drivers. */
typedef struct nge_file NGE_FILE;

/**
 * @brief Write a character to a stream.
 * @param c Character or device control code.
 * @param stream Open stream.
 * @return c on success, or NGE_EOF on error.
 */
int nge_putc(uint8_t c, NGE_FILE *stream);

/**
 * @brief Non-blocking: read a character from a stream, returning at once.
 *
 * nge_getc_wait() in nge_line.h is the blocking version.
 * @param stream Open stream.
 * @return The next character, or NGE_EOF if none is waiting or the stream
 *         is not an input stream.
 */
int nge_getc(NGE_FILE *stream);

/**
 * @brief Send any output the device is holding in a buffer.
 * @param stream Open stream, or NULL (ignored).
 */
void nge_flush(NGE_FILE *stream);

/**
 * @brief Close a stream, flushing any buffered output, and free its slot.
 * @param stream Open stream, or NULL (ignored).
 */
void nge_close(NGE_FILE *stream);

#endif
