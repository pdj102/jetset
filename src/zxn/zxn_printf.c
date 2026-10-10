#include "zxn_printf.h"

#include <stdarg.h>
#include <stdint.h>

#define FLAG_ZERO  0x01
#define FLAG_LEFT  0x02

static const uint16_t pow10[] = { 10000, 1000, 100, 10, 1 };
static const char hex_lower[] = "0123456789abcdef";
static const char hex_upper[] = "0123456789ABCDEF";

/* Convert v to digits in buf, without leading zeros; return the length. */
static uint8_t fmt_dec(char *buf, uint16_t v)
{
    uint8_t n = 0;
    uint8_t i;
    char d;

    /* Repeated subtraction avoids a division routine */
    for (i = 0; i < 5; i++) {
        d = '0';
        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        if (n || d != '0' || i == 4) {
            buf[n++] = d;
        }
    }
    return n;
}

/* Convert v to hex digits in buf, without leading zeros; return the length. */
static uint8_t fmt_hex(char *buf, uint16_t v, const char *digits)
{
    uint8_t n = 0;
    uint8_t shift = 12;
    uint8_t d;

    for (;;) {
        d = (uint8_t)(v >> shift) & 0x0F;
        if (n || d || shift == 0) {
            buf[n++] = digits[d];
        }
        if (shift == 0) {
            break;
        }
        shift -= 4;
    }
    return n;
}

/* Write c n times; return n. */
static uint8_t put_repeat(uint8_t c, uint8_t n, ZXN_FILE *stream)
{
    uint8_t i;

    for (i = n; i; i--) {
        zxn_putc(c, stream);
    }
    return n;
}

int zxn_printf(ZXN_FILE *stream, const char *fmt, ...)
{
    va_list ap;
    char buf[6];
    const char *str;
    uint16_t u;
    int16_t v;
    uint8_t len;
    uint8_t width;
    uint8_t flags;
    uint8_t pad;
    uint8_t total;
    uint8_t npad;
    char sign;
    int count = 0;
    char c;

    va_start(ap, fmt);

    while ((c = *fmt++) != 0) {
        if (c != '%') {
            if (zxn_putc((uint8_t)c, stream) == ZXN_EOF) {
                return ZXN_EOF;
            }
            count++;
            continue;
        }

        /* Flags and width */
        flags = 0;
        for (;;) {
            if (*fmt == '0') {
                flags |= FLAG_ZERO;
            } else if (*fmt == '-') {
                flags |= FLAG_LEFT;
            } else {
                break;
            }
            fmt++;
        }
        width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = (uint8_t)(width * 10 + (*fmt++ - '0'));
        }

        sign = 0;
        str = buf;

        switch (c = *fmt++) {
        case 'c':
            buf[0] = (char)va_arg(ap, int);
            len = 1;
            break;
        case 's':
            str = va_arg(ap, const char *);
            if (str == 0) {
                str = "(null)";
            }
            for (len = 0; str[len] && len < 255; len++) {
            }
            break;
        case 'd':
        case 'i':
            v = (int16_t)va_arg(ap, int);
            u = (uint16_t)v;
            if (v < 0) {
                sign = '-';
                u = (uint16_t)0 - u; /* also correct for -32768 */
            }
            len = fmt_dec(buf, u);
            break;
        case 'u':
            len = fmt_dec(buf, (uint16_t)va_arg(ap, unsigned int));
            break;
        case 'x':
            len = fmt_hex(buf, (uint16_t)va_arg(ap, unsigned int), hex_lower);
            break;
        case 'X':
            len = fmt_hex(buf, (uint16_t)va_arg(ap, unsigned int), hex_upper);
            break;
        case 0:
            /* Format ends after '%' */
            fmt--;
            /* fall through */
        default:
            /* "%%" and unknown conversions are written literally */
            buf[0] = c ? c : '%';
            len = 1;
            break;
        }

        /* Number of padding characters needed to reach the width */
        total = (uint8_t)(len + (sign ? 1 : 0));
        npad = (width > total) ? (uint8_t)(width - total) : 0;
        pad = ((flags & (FLAG_ZERO | FLAG_LEFT)) == FLAG_ZERO && c != 's' && c != 'c') ? '0' : ' ';

        /* Layout: [spaces][sign][zeros]digits[spaces] */
        if (!(flags & FLAG_LEFT) && pad == ' ') {
            count += put_repeat(' ', npad, stream);
        }
        if (sign) {
            zxn_putc((uint8_t)sign, stream);
            count++;
        }
        if (pad == '0') {
            count += put_repeat('0', npad, stream);
        }
        for (; len; len--, count++) {
            if (zxn_putc((uint8_t)*str++, stream) == ZXN_EOF) {
                return ZXN_EOF;
            }
        }
        if (flags & FLAG_LEFT) {
            count += put_repeat(' ', npad, stream);
        }
    }

    va_end(ap);
    return count;
}
