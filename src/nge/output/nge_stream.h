#ifndef NGE_STREAM_H
#define NGE_STREAM_H

/**
 * @file nge_stream.h
 * @brief Device-independent character output streams.
 *
 * Streams are opened by a device driver (see nge_tm_stream.h and
 * nge_uart_stream.h) and written through nge_putc() or the helpers in
 * nge_print.h.
 */

#include <stdint.h>

/** Maximum number of streams open at once. */
#ifndef NGE_FILE_MAX
#define NGE_FILE_MAX 4
#endif

/** Value returned on error. */
#define NGE_EOF (-1)

/** An output stream. Opaque outside device drivers. */
typedef struct nge_file NGE_FILE;

/**
 * @brief Write a character to a stream.
 * @param c Character or device control code.
 * @param stream Open stream.
 * @return c on success, or NGE_EOF on error.
 */
int nge_putc(uint8_t c, NGE_FILE *stream);

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
