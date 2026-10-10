#include "zxn_putc.h"

#include <stddef.h>

#include "zxn_uart.h"
#include "zxn_util.h"

#define TILEMAP_WIDTH  40
#define TILEMAP_HEIGHT 32

#define BLANK_TILE_ID  0x20

/* Control-code state machine states */
#define ESC_NONE  0
#define ESC_INK   1
#define ESC_ATTR  2
#define ESC_AT_Y  3
#define ESC_AT_X  4

static ZXN_FILE streams[ZXN_FILE_MAX];

/* Return a free stream slot, or NULL if all are in use. */
static ZXN_FILE *alloc_stream(zxn_put_fn put)
{
    ZXN_FILE *s = streams;
    uint8_t i;

    for (i = ZXN_FILE_MAX; i; i--, s++) {
        if (s->put == NULL) {
            s->put = put;
            return s;
        }
    }
    return NULL;
}

/* Move the cursor down a row, scrolling the window at the bottom. */
static void tm_linefeed(zxn_tm_state_t *tm)
{
    zxn_tile_t blank;

    if (++tm->cy > tm->area.y2) {
        tm->cy = tm->area.y2;
        blank.tile_id = BLANK_TILE_ID;
        blank.tile_attr = tm->attr;
        zxn_tilemap_scroll_up(&tm->area, &blank);
    }
}

/* Handle a character in the normal (no parameter pending) state. */
static void tm_char(zxn_tm_state_t *tm, uint8_t c)
{
    zxn_tile_t t;

    if (c >= 0x20) {
        t.tile_id = c;
        t.tile_attr = tm->attr;
        zxn_tilemap_set(tm->cx, tm->cy, &t);

        if (++tm->cx > tm->area.x2) {
            tm->cx = tm->area.x1;
            tm_linefeed(tm);
        }
        return;
    }

    switch (c) {
    case ZXN_CTRL_LF:
        tm->cx = tm->area.x1;
        tm_linefeed(tm);
        break;
    case ZXN_CTRL_CR:
        tm->cx = tm->area.x1;
        break;
    case ZXN_CTRL_BS:
        if (tm->cx > tm->area.x1) {
            tm->cx--;
        }
        break;
    case ZXN_CTRL_CLS:
        t.tile_id = BLANK_TILE_ID;
        t.tile_attr = tm->attr;
        zxn_tilemap_fill(&tm->area, &t);
        tm->cx = tm->area.x1;
        tm->cy = tm->area.y1;
        break;
    case ZXN_CTRL_INK:
        tm->esc = ESC_INK;
        break;
    case ZXN_CTRL_ATTR:
        tm->esc = ESC_ATTR;
        break;
    case ZXN_CTRL_AT:
        tm->esc = ESC_AT_Y;
        break;
    default:
        break;
    }
}

/* Tilemap device output function. */
static int tm_put(uint8_t c, ZXN_FILE *stream)
{
    zxn_tm_state_t *tm = &stream->u.tm;
    uint8_t state = tm->esc;
    uint8_t pos;

    tm->esc = ESC_NONE;

    switch (state) {
    case ESC_NONE:
        tm_char(tm, c);
        break;
    case ESC_INK:
        tm->attr = (tm->attr & 0x0F) | (uint8_t)(c << 4);
        break;
    case ESC_ATTR:
        tm->attr = c;
        break;
    case ESC_AT_Y:
        tm->arg = c;
        tm->esc = ESC_AT_X;
        break;
    case ESC_AT_X:
        /* Clamp to the window */
        pos = c + tm->area.x1;
        tm->cx = (c > tm->area.x2 - tm->area.x1) ? tm->area.x2 : pos;
        pos = tm->arg + tm->area.y1;
        tm->cy = (tm->arg > tm->area.y2 - tm->area.y1) ? tm->area.y2 : pos;
        break;
    }
    return c;
}

/* UART device output function. */
static int uart_put(uint8_t c, ZXN_FILE *stream)
{
    (void)stream;
    zxn_uart_write_byte(c);
    return c;
}

ZXN_FILE *zxn_open_tilemap(const zxn_rect_t *area, uint8_t attr)
{
    zxn_util_assert(area != NULL);
    zxn_util_assert(area->x1 <= area->x2 && area->x2 < TILEMAP_WIDTH);
    zxn_util_assert(area->y1 <= area->y2 && area->y2 < TILEMAP_HEIGHT);

    ZXN_FILE *s = alloc_stream(tm_put);

    if (s != NULL) {
        zxn_tm_state_t *tm = &s->u.tm;

        tm->area = *area;
        tm->cx = area->x1;
        tm->cy = area->y1;
        tm->attr = attr;
        tm->esc = ESC_NONE;
    }
    return s;
}

ZXN_FILE *zxn_open_uart(void)
{
    return alloc_stream(uart_put);
}

int zxn_putc(uint8_t c, ZXN_FILE *stream)
{
    if (stream == NULL || stream->put == NULL) {
        return ZXN_EOF;
    }
    return stream->put(c, stream);
}

void zxn_close(ZXN_FILE *stream)
{
    zxn_util_assert(stream != NULL);

    stream->put = NULL;
}
