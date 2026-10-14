#ifndef NGE_PRINTF_H
#define NGE_PRINTF_H

/**
 * @file nge_printf.h
 * @brief Small formatted output for streams.
 *
 * Supported conversions: %c %s %d %i %u %x %X %%, with an optional '0' flag,
 * '-' flag (left-justify) and decimal field width, e.g. "%05u", "%-8s", "%4X".
 * Numbers are 16-bit; there are no long, precision or floating-point forms.
 * For an unknown conversion such as %q, the character after % is written.
 *
 * In common memory, like the rest of the output path, so banked code can
 * pass its own string literals (which are in its banked page) to these
 * functions: nothing is remapped before the strings are read.
 */

#include "io/nge_stream.h"

/**
 * @brief Write formatted output to a stream.
 * @param stream Open stream.
 * @param fmt Format string.
 * @return Number of characters written, or NGE_EOF on error.
 */
int nge_printf(NGE_FILE *stream, const char *fmt, ...);

#endif
