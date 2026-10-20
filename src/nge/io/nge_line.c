#include "io/nge_line.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "hw/nge_frame.h"
#include "input/nge_input.h"
#include "io/nge_stream_dev.h"
#include "io/nge_text.h"

#define CHAR_BACKSPACE  8
#define CHAR_ENTER      13
#define CHAR_DELETE     12

void nge_line_begin(nge_line_t *l, char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo)
{
    nge_util_assert(buf != NULL && size >= 1);

    l->buf = buf;
    l->size = size;
    l->len = 0;
    l->in = in;
    l->echo = echo;
    buf[0] = 0;
}

/* Edit with one character; 1 on ENTER */
static uint8_t edit(nge_line_t *l, uint8_t c)
{
    if (c == CHAR_ENTER) {
        return 1;
    }
    if (c == CHAR_DELETE || c == CHAR_BACKSPACE) {
        if (l->len > 0) {
            l->buf[--l->len] = 0;
            nge_putc(NGE_CTRL_BS, l->echo);
            nge_putc(' ', l->echo);
            nge_putc(NGE_CTRL_BS, l->echo);
        }
    } else if (c >= 32 && c <= 126 && l->len + 1 < l->size) {
        l->buf[l->len++] = (char)c;
        l->buf[l->len] = 0;
        nge_putc(c, l->echo);
    }
    return 0;
}

uint8_t nge_line_update(nge_line_t *l)
{
    int c;

    while ((c = nge_getc(l->in)) != NGE_EOF) {
        if (edit(l, (uint8_t)c)) {
            return 1;
        }
    }
    return 0;
}

int nge_getc_wait(NGE_FILE *in)
{
    int c;

    nge_util_assert(in != NULL && in->get != NULL);   /* an input stream, or this never returns */

    while ((c = nge_getc(in)) == NGE_EOF) {
        nge_frame_wait_vblank();
        nge_input_update();
    }
    return c;
}

void nge_gets(char *buf, uint8_t size, NGE_FILE *in, NGE_FILE *echo)
{
    nge_line_t l;

    nge_line_begin(&l, buf, size, in, echo);
    while (!edit(&l, (uint8_t)nge_getc_wait(in))) {
    }
}
