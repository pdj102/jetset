#ifndef ZXN_PRINT_H
#define ZXN_PRINT_H

/**
 * @file zxn_print.h
 * @brief Device-independent string and number output built on zxn_putc().
 */

#include <stdint.h>

#include "zxn_stream.h"

/**
 * @brief Write raw bytes to a stream.
 * @param buf Bytes to write; may contain control codes and zero bytes.
 * @param len Number of bytes to write.
 * @param stream Open stream.
 * @return Number of bytes written, or ZXN_EOF on error.
 */
int zxn_write(const void *buf, uint16_t len, ZXN_FILE *stream);

/**
 * @brief Write a NUL-terminated string to a stream, without adding a newline.
 * @param s String to write.
 * @param stream Open stream.
 * @return Number of characters written, or ZXN_EOF on error.
 */
int zxn_puts(const char *s, ZXN_FILE *stream);

/**
 * @brief Write an unsigned decimal number.
 * @param v Value.
 * @param stream Open stream.
 */
void zxn_putu16(uint16_t v, ZXN_FILE *stream);

/**
 * @brief Write an unsigned decimal number right-aligned in a fixed width.
 * @param v Value.
 * @param width Minimum number of characters; wider numbers are not truncated.
 * @param pad Padding character, usually ' ' or '0'.
 * @param stream Open stream.
 */
void zxn_putu16w(uint16_t v, uint8_t width, uint8_t pad, ZXN_FILE *stream);

/**
 * @brief Write a signed decimal number.
 * @param v Value.
 * @param stream Open stream.
 */
void zxn_puti16(int16_t v, ZXN_FILE *stream);

/**
 * @brief Write a byte as two upper-case hex digits.
 * @param v Value.
 * @param stream Open stream.
 */
void zxn_puthex8(uint8_t v, ZXN_FILE *stream);

/**
 * @brief Write a word as four upper-case hex digits.
 * @param v Value.
 * @param stream Open stream.
 */
void zxn_puthex16(uint16_t v, ZXN_FILE *stream);

#endif
