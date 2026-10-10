#ifndef ZXN_PRINTF_H
#define ZXN_PRINTF_H

/**
 * @file zxn_printf.h
 * @brief Small formatted output for streams.
 *
 * Supported conversions: %c %s %d %i %u %x %X %%, with an optional '0' flag,
 * '-' flag (left-justify) and decimal field width, e.g. "%05u", "%-8s", "%4X".
 * Numbers are 16-bit; there are no long, precision or floating-point forms.
 * For an unknown conversion such as %q, the character after % is written.
 */

#include "zxn_stream.h"

/**
 * @brief Write formatted output to a stream.
 * @param stream Open stream.
 * @param fmt Format string.
 * @return Number of characters written, or ZXN_EOF on error.
 */
int zxn_printf(ZXN_FILE *stream, const char *fmt, ...);

#endif
