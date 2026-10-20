#include "io/nge_tm_stream.h"

#include <stddef.h>

#include "io/nge_stream_dev.h"
#include "tilemap/nge_tilemap.h"

#define BLANK_TILE_ID  0x20

/* Control-code state machine states */
#define ESC_NONE  0
#define ESC_INK   1
#define ESC_ATTR  2
#define ESC_AT_Y  3
#define ESC_AT_X  4
#define ESC_CLIP  5

/* Point the cached cell at the cursor after a non-sequential move. */
static void tm_sync(nge_tm_state_t *tm)
{
    tm->cell = nge_tilemap_cell(tm->cx, tm->cy);
}

/* Move the cursor down a row; at the bottom, scroll unless clipping. */
static void tm_linefeed(nge_tm_state_t *tm)
{
    nge_tile_t blank;

    if (++tm->cy > tm->area.y2) {
        tm->cy = tm->area.y2;
        if (!tm->clip) {
            blank.tile_id = BLANK_TILE_ID;
            blank.tile_attr = tm->attr;
            nge_tilemap_scroll_up(&tm->area, &blank);
        }
    }
    tm_sync(tm);
}

/*
 * Print a character while the cursor is past the right edge (a wrap is
 * pending). Clipping windows drop it; others wrap, then print it.
 */
static void tm_wrap_char(nge_tm_state_t *tm, uint8_t c)
{
    nge_tile_t *cell;

    if (tm->clip) {
        return;
    }
    tm->cx = tm->area.x1;
    tm_linefeed(tm);

    cell = tm->cell;
    cell->tile_id = c;
    cell->tile_attr = tm->attr;
    tm->cx++;
    tm->cell = cell + 1;
}

/* Handle a control code in the normal (no parameter pending) state. */
static void tm_control(nge_tm_state_t *tm, uint8_t c)
{
    nge_tile_t t;
    nge_rect_t r;

    if (c >= 0x20) {
        tm_wrap_char(tm, c);
        return;
    }

    switch (c) {
    case NGE_CTRL_LF:
        tm->cx = tm->area.x1;
        tm_linefeed(tm);
        break;
    case NGE_CTRL_CR:
        tm->cx = tm->area.x1;
        tm_sync(tm);
        break;
    case NGE_CTRL_BS:
        if (tm->cx > tm->area.x1) {
            tm->cx--;
            tm->cell--;
        }
        break;
    case NGE_CTRL_CLREOL:
        /* Blank from the cursor to the right edge; the cursor stays put */
        if (tm->cx <= tm->area.x2) {
            t.tile_id = BLANK_TILE_ID;
            t.tile_attr = tm->attr;
            r.x1 = tm->cx;
            r.x2 = tm->area.x2;
            r.y1 = tm->cy;
            r.y2 = tm->cy;
            nge_tilemap_fill(&r, &t);
        }
        break;
    case NGE_CTRL_CLS:
        t.tile_id = BLANK_TILE_ID;
        t.tile_attr = tm->attr;
        nge_tilemap_fill(&tm->area, &t);
        tm->cx = tm->area.x1;
        tm->cy = tm->area.y1;
        tm_sync(tm);
        break;
    case NGE_CTRL_INK:
        tm->esc = ESC_INK;
        break;
    case NGE_CTRL_ATTR:
        tm->esc = ESC_ATTR;
        break;
    case NGE_CTRL_AT:
        tm->esc = ESC_AT_Y;
        break;
    case NGE_CTRL_CLIP:
        tm->esc = ESC_CLIP;
        break;
    default:
        break;
    }
}

/* Handle a control code or a pending control-code parameter. */
static void tm_escape(nge_tm_state_t *tm, uint8_t c)
{
    uint8_t state = tm->esc;
    uint8_t pos;

    tm->esc = ESC_NONE;

    switch (state) {
    case ESC_NONE:
        tm_control(tm, c);
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
        tm_sync(tm);
        break;
    case ESC_CLIP:
        tm->clip = c;
        break;
    }
}

/* Tilemap device output function. Kept small so SDCC compiles the fast path
   without a large stack frame; everything else is in tm_escape(). */
static int tm_put(uint8_t c, NGE_FILE *stream)
{
    nge_tm_state_t *tm = &stream->u.tm;
    nge_tile_t *cell;

    /*
     * Fast path: a printable character with no parameter pending and the
     * cursor inside the window is two stores through the cached cell pointer.
     * Writing the last column leaves the cursor one past the right edge; the
     * wrap happens only when the next character arrives (see tm_wrap_char).
     */
    if (c >= 0x20 && tm->esc == ESC_NONE && tm->cx <= tm->area.x2) {
        cell = tm->cell;
        cell->tile_id = c;
        cell->tile_attr = tm->attr;
        tm->cx++;
        tm->cell = cell + 1;
    } else {
        tm_escape(tm, c);
    }
    return c;
}

NGE_FILE *nge_open_tilemap(const nge_rect_t *area, uint8_t attr)
{
    nge_util_assert(area != NULL);
    nge_util_assert(area->x1 <= area->x2 && area->x2 < NGE_TILEMAP_WIDTH);
    nge_util_assert(area->y1 <= area->y2 && area->y2 < NGE_TILEMAP_HEIGHT);

    NGE_FILE *s = nge_stream_alloc(tm_put, NULL, NULL);

    if (s != NULL) {
        nge_tm_state_t *tm = &s->u.tm;

        tm->area = *area;
        tm->cx = area->x1;
        tm->cy = area->y1;
        tm->attr = attr;
        tm->esc = ESC_NONE;
        tm->clip = 0;
        tm_sync(tm);
    }
    return s;
}
