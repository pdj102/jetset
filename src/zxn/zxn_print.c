#include "zxn_print.h"

static const uint16_t pow10[] = { 10000, 1000, 100, 10, 1 };
static const char hex_digits[] = "0123456789ABCDEF";

int zxn_write(const void *buf, uint16_t len, ZXN_FILE *stream)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint16_t n;

    for (n = len; n; n--) {
        if (zxn_putc(*p++, stream) == ZXN_EOF) {
            return ZXN_EOF;
        }
    }
    return (int)len;
}

int zxn_puts(const char *s, ZXN_FILE *stream)
{
    const char *p = s;

    while (*p) {
        if (zxn_putc((uint8_t)*p++, stream) == ZXN_EOF) {
            return ZXN_EOF;
        }
    }
    return (int)(p - s);
}

void zxn_putu16w(uint16_t v, uint8_t width, uint8_t pad, ZXN_FILE *stream)
{
    uint8_t digits = 5;
    uint8_t i;
    uint8_t d;

    /* Count digits, leaving at least one for zero */
    for (i = 0; i < 4 && v < pow10[i]; i++) {
        digits--;
    }

    while (width > digits) {
        zxn_putc(pad, stream);
        width--;
    }

    /* Repeated subtraction avoids a division routine; i is the first digit */
    for (; i < 5; i++) {
        d = '0';
        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        zxn_putc(d, stream);
    }
}

void zxn_putu16(uint16_t v, ZXN_FILE *stream)
{
    zxn_putu16w(v, 0, ' ', stream);
}

void zxn_puti16(int16_t v, ZXN_FILE *stream)
{
    uint16_t u = (uint16_t)v;

    if (v < 0) {
        zxn_putc('-', stream);
        u = (uint16_t)0 - u; /* also correct for -32768 */
    }
    zxn_putu16w(u, 0, ' ', stream);
}

void zxn_puthex8(uint8_t v, ZXN_FILE *stream)
{
    zxn_putc((uint8_t)hex_digits[v >> 4], stream);
    zxn_putc((uint8_t)hex_digits[v & 0x0F], stream);
}

void zxn_puthex16(uint16_t v, ZXN_FILE *stream)
{
    zxn_puthex8((uint8_t)(v >> 8), stream);
    zxn_puthex8((uint8_t)v, stream);
}
