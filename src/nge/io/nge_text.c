/*
 * The text control-code functions: common memory, as streams are. Each only
 * sends its code, so any text stream device interprets it.
 */
#include "io/nge_text.h"

void nge_text_ink(uint8_t n, NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_INK, stream);
    nge_putc(n, stream);
}

void nge_text_attr(uint8_t a, NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_ATTR, stream);
    nge_putc(a, stream);
}

void nge_text_paper(uint8_t p, NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_PAPER, stream);
    nge_putc(p, stream);
}

void nge_text_at(uint8_t y, uint8_t x, NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_AT, stream);
    nge_putc(y, stream);
    nge_putc(x, stream);
}

void nge_text_cls(NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_CLS, stream);
}

void nge_text_clreol(NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_CLREOL, stream);
}

void nge_text_clip(uint8_t on, NGE_FILE *stream)
{
    nge_putc(NGE_CTRL_CLIP, stream);
    nge_putc(on, stream);
}
