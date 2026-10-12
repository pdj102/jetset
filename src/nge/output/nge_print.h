#ifndef NGE_PRINT_H
#define NGE_PRINT_H

/**
 * @file nge_print.h
 * @brief Device-independent string and number output built on nge_putc().
 *
 * The functions are banked (NGE_BANKED, bank 20): each call maps bank 20 into
 * slots 6-7 and back. Call nge_hw_init() before the first one, and don't call
 * them while game code has its own page mapped in slot 6 or 7 (see
 * core/nge_bank.h).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "output/nge_stream.h"

/**
 * @brief Write raw bytes to a stream.
 * @param buf Bytes to write; may contain control codes and zero bytes.
 * @param len Number of bytes to write.
 * @param stream Open stream.
 * @return Number of bytes written, or NGE_EOF on error.
 */
int nge_write(const void *buf, uint16_t len, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write a NUL-terminated string to a stream, without adding a newline.
 * @param s String to write.
 * @param stream Open stream.
 * @return Number of characters written, or NGE_EOF on error.
 */
int nge_puts(const char *s, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write an unsigned decimal number.
 * @param v Value.
 * @param stream Open stream.
 */
void nge_putu16(uint16_t v, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write an unsigned decimal number right-aligned in a fixed width.
 * @param v Value.
 * @param width Minimum number of characters; wider numbers are not truncated.
 * @param pad Padding character, usually ' ' or '0'.
 * @param stream Open stream.
 */
void nge_putu16w(uint16_t v, uint8_t width, uint8_t pad, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write a signed decimal number.
 * @param v Value.
 * @param stream Open stream.
 */
void nge_puti16(int16_t v, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write a byte as two upper-case hex digits.
 * @param v Value.
 * @param stream Open stream.
 */
void nge_puthex8(uint8_t v, NGE_FILE *stream) NGE_BANKED;

/**
 * @brief Write a word as four upper-case hex digits.
 * @param v Value.
 * @param stream Open stream.
 */
void nge_puthex16(uint16_t v, NGE_FILE *stream) NGE_BANKED;

#endif
