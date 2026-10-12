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
 * The functions are banked (NGE_BANKED, page 40): each call maps pages 40-41
 * into slots 6-7 and back. Call nge_hw_init() before the first one, and don't call
 * them while game code has its own page mapped in slot 6 or 7 (see
 * core/nge_bank.h).
 */

#include "core/nge_bank.h"
#include "output/nge_stream.h"

/**
 * @brief Write formatted output to a stream.
 * @param stream Open stream.
 * @param fmt Format string.
 * @return Number of characters written, or NGE_EOF on error.
 */
int nge_printf(NGE_FILE *stream, const char *fmt, ...) NGE_BANKED;

#endif
