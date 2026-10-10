#ifndef ZXN_STREAM_H
#define ZXN_STREAM_H

/**
 * @file zxn_stream.h
 * @brief Device-independent character output streams.
 *
 * Streams are opened by a device driver (see zxn_tm_stream.h and
 * zxn_uart_stream.h) and written through zxn_putc() or the helpers in
 * zxn_print.h.
 */

#include <stdint.h>

/** Maximum number of streams open at once. */
#ifndef ZXN_FILE_MAX
#define ZXN_FILE_MAX 4
#endif

/** Value returned on error. */
#define ZXN_EOF (-1)

/** An output stream. Opaque outside device drivers. */
typedef struct zxn_file ZXN_FILE;

/**
 * @brief Write a character to a stream.
 * @param c Character or device control code.
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
