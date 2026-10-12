#include "output/nge_print.h"

static const uint16_t pow10[] = { 10000, 1000, 100, 10, 1 };
static const char hex_digits[] = "0123456789ABCDEF";

int nge_write(const void *buf, uint16_t len, NGE_FILE *stream)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint16_t n;

    for (n = len; n; n--) {
        if (nge_putc(*p++, stream) == NGE_EOF) {
            return NGE_EOF;
        }
    }
    return (int)len;
}

int nge_puts(const char *s, NGE_FILE *stream)
{
    const char *p = s;

    while (*p) {
        if (nge_putc((uint8_t)*p++, stream) == NGE_EOF) {
            return NGE_EOF;
        }
    }
    return (int)(p - s);
}

void nge_putu16w(uint16_t v, uint8_t width, uint8_t pad, NGE_FILE *stream)
{
    uint8_t digits = 5;
    uint8_t i;
    uint8_t d;

    /* Count digits, leaving at least one for zero */
    for (i = 0; i < 4 && v < pow10[i]; i++) {
        digits--;
    }

    while (width > digits) {
        nge_putc(pad, stream);
        width--;
    }

    /* Repeated subtraction avoids a division routine; i is the first digit */
    for (; i < 5; i++) {
        d = '0';
        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        nge_putc(d, stream);
    }
}

void nge_putu16(uint16_t v, NGE_FILE *stream)
{
    nge_putu16w(v, 0, ' ', stream);
}

void nge_puti16(int16_t v, NGE_FILE *stream)
{
    uint16_t u = (uint16_t)v;

    if (v < 0) {
        nge_putc('-', stream);
        u = (uint16_t)0 - u; /* also correct for -32768 */
    }
    nge_putu16w(u, 0, ' ', stream);
}

void nge_puthex8(uint8_t v, NGE_FILE *stream)
{
    nge_putc((uint8_t)hex_digits[v >> 4], stream);
    nge_putc((uint8_t)hex_digits[v & 0x0F], stream);
}

void nge_puthex16(uint16_t v, NGE_FILE *stream)
{
    nge_puthex8((uint8_t)(v >> 8), stream);
    nge_puthex8((uint8_t)v, stream);
}
