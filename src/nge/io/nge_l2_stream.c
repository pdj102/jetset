/*
 * Layer 2 cell streams: common memory, as the output function is called
 * through a pointer. The drawing is banked (layer2/, page 90). The logic
 * follows nge_tm_stream.c step for step, so the two behave alike.
 */
#include "io/nge_l2_stream.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "io/nge_stream_dev.h"
#include "layer2/nge_layer2.h"

#define WHITE  0xFF   /* RRRGGGBB */

/* The text colours, NGE_INK_WHITE onwards, as Layer 2 colours: the tilemap's ink palette's colours */
static const uint8_t ink_colours[] = { 0xFF, 0xFC, 0x1F, 0xE0 };

/* Control-code state machine states */
#define ESC_NONE  0
#define ESC_INK   1
#define ESC_ATTR  2
#define ESC_AT_Y  3
#define ESC_AT_X  4
#define ESC_CLIP  5

/* A text colour (NGE_INK_*) as a Layer 2 colour; anything else is white */
static uint8_t ink_colour(uint8_t n)
{
    uint8_t i = (uint8_t)(n - NGE_INK_WHITE);

    return (i < sizeof(ink_colours)) ? ink_colours[i] : WHITE;
}

/* Move the cursor down a row; at the bottom, scroll unless clipping */
static void l2_linefeed(nge_l2_state_t *l2)
{
    if (++l2->cy > l2->area.y2) {
        l2->cy = l2->area.y2;
        if (!l2->clip) {
            nge_layer2_scroll_up(l2->area.x1, l2->area.y1, l2->area.x2, l2->area.y2, l2->paper);
        }
    }
}

/* Draw c at the cursor and move on */
static void l2_char(nge_l2_state_t *l2, uint8_t c)
{
    nge_layer2_glyph(l2->cx, l2->cy, c, l2->ink, l2->paper);
    l2->cx++;
}

/*
 * Print a character while the cursor is past the right edge (a wrap is
 * pending). Clipping windows drop it; others wrap, then print it
 */
static void l2_wrap_char(nge_l2_state_t *l2, uint8_t c)
{
    if (l2->clip) {
        return;
    }
    l2->cx = l2->area.x1;
    l2_linefeed(l2);
    l2_char(l2, c);
}

/* Handle a control code in the normal (no parameter pending) state */
static void l2_control(nge_l2_state_t *l2, uint8_t c)
{
    if (c >= 0x20) {
        l2_wrap_char(l2, c);
        return;
    }

    switch (c) {
    case NGE_CTRL_LF:
        l2->cx = l2->area.x1;
        l2_linefeed(l2);
        break;
    case NGE_CTRL_CR:
        l2->cx = l2->area.x1;
        break;
    case NGE_CTRL_BS:
        if (l2->cx > l2->area.x1) {
            l2->cx--;
        }
        break;
    case NGE_CTRL_CLREOL:
        /* Paper from the cursor to the right edge; the cursor stays put */
        if (l2->cx <= l2->area.x2) {
            nge_layer2_fill(l2->cx, l2->cy, l2->area.x2, l2->cy, l2->paper);
        }
        break;
    case NGE_CTRL_CLS:
        nge_layer2_fill(l2->area.x1, l2->area.y1, l2->area.x2, l2->area.y2, l2->paper);
        l2->cx = l2->area.x1;
        l2->cy = l2->area.y1;
        break;
    case NGE_CTRL_INK:
        l2->esc = ESC_INK;
        break;
    case NGE_CTRL_ATTR:
        l2->esc = ESC_ATTR;
        break;
    case NGE_CTRL_AT:
        l2->esc = ESC_AT_Y;
        break;
    case NGE_CTRL_CLIP:
        l2->esc = ESC_CLIP;
        break;
    default:
        break;
    }
}

/* Handle a control code or a pending control-code parameter */
static void l2_escape(nge_l2_state_t *l2, uint8_t c)
{
    uint8_t state = l2->esc;
    uint8_t pos;

    l2->esc = ESC_NONE;

    switch (state) {
    case ESC_NONE:
        l2_control(l2, c);
        break;
    case ESC_INK:
        l2->ink = ink_colour(c);
        break;
    case ESC_ATTR:
        l2->ink = c;
        break;
    case ESC_AT_Y:
        l2->arg = c;
        l2->esc = ESC_AT_X;
        break;
    case ESC_AT_X:
        /* Clamp to the window */
        pos = c + l2->area.x1;
        l2->cx = (c > l2->area.x2 - l2->area.x1) ? l2->area.x2 : pos;
        pos = l2->arg + l2->area.y1;
        l2->cy = (l2->arg > l2->area.y2 - l2->area.y1) ? l2->area.y2 : pos;
        break;
    case ESC_CLIP:
        l2->clip = c;
        break;
    }
}

/* Layer 2 device output function: a printable character in the window is the fast path */
static int l2_put(uint8_t c, NGE_FILE *stream)
{
    nge_l2_state_t *l2 = &stream->u.l2;

    if (c >= 0x20 && l2->esc == ESC_NONE && l2->cx <= l2->area.x2) {
        l2_char(l2, c);
    } else {
        l2_escape(l2, c);
    }
    return c;
}

NGE_FILE *nge_open_layer2(const nge_rect_t *cells, uint8_t paper)
{
    NGE_FILE *s;
    nge_l2_state_t *l2;

    nge_util_assert(cells != NULL);
    nge_util_assert(cells->x1 <= cells->x2 && cells->x2 < NGE_LAYER2_COLS);
    nge_util_assert(cells->y1 <= cells->y2 && cells->y2 < NGE_LAYER2_ROWS);

    s = nge_stream_alloc(l2_put, NULL, NULL);
    if (s != NULL) {
        l2 = &s->u.l2;
        l2->area = *cells;
        l2->cx = cells->x1;
        l2->cy = cells->y1;
        l2->ink = WHITE;
        l2->paper = paper;
        l2->esc = ESC_NONE;
        l2->clip = 0;
    }
    return s;
}
