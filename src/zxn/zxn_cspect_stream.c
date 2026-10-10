#include "zxn_cspect_stream.h"

#include <stddef.h>
#include <stdint.h>

#ifdef ZXN_CSPECT

#include "zxn_cspect.h"
#include "zxn_stream_dev.h"

/* Shared by all debug streams; one extra byte for the NUL */
static char line[ZXN_CSPECT_LINE_MAX + 1];
static uint8_t line_len;

/* Send the buffered line to the console. */
static void cspect_flush(ZXN_FILE *stream)
{
    (void)stream;

    if (line_len) {
        line[line_len] = 0;
        zxn_cspect_print(line);
        line_len = 0;
    }
}

/* CSpect device output function. */
static int cspect_put(uint8_t c, ZXN_FILE *stream)
{
    if (c == '\n') {
        cspect_flush(stream);
        return c;
    }

    if (c == '%') {
        /* CSpect treats '%' as a format code and has no escape for it */
        line[line_len++] = ZXN_CSPECT_PERCENT;
    } else if (c >= 0x20) {
        line[line_len++] = (char)c;
    }
    /* Other control codes, including 0x00, are dropped */

    if (line_len >= ZXN_CSPECT_LINE_MAX) {
        cspect_flush(stream);
    }
    return c;
}

ZXN_FILE *zxn_open_cspect_debug(void)
{
    return zxn_stream_alloc(cspect_put, cspect_flush);
}

#else

ZXN_FILE *zxn_open_cspect_debug(void)
{
    return NULL;
}

#endif
