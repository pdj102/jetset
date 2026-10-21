#ifndef NGE_LAYER2_INTERNAL_H
#define NGE_LAYER2_INTERNAL_H

/*
 * Shared by the layer2 module's files; not for use outside it:
 * - nge_layer2_draw.c (page 90): set-up and drawing;
 * - nge_layer2_font.c (page 90): the font, read by the drawing.
 */

#include <stdint.h>

#define NGE_LAYER2_CELL        8    /* pixels a cell, each way */
#define NGE_LAYER2_FONT_FIRST  32   /* the font's characters: space to DEL */
#define NGE_LAYER2_FONT_COUNT  96

/*
 * The font as column bitmasks: 8 bytes a character, one a pixel column,
 * bit 7 the top pixel. A column of Layer 2 is 8 contiguous bytes, so a
 * glyph's column is drawn from one byte. In page 90, the code page, not
 * the odd data page: the drawing maps a Layer 2 page into slot 7, where
 * page 91 would be, so the font must be readable in slot 6 then
 */
extern const uint8_t nge_layer2_font[NGE_LAYER2_FONT_COUNT][NGE_LAYER2_CELL];

#endif
