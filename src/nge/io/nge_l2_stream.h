#ifndef NGE_L2_STREAM_H
#define NGE_L2_STREAM_H

/**
 * @file nge_l2_stream.h
 * @brief Output streams on rectangles of Layer 2 cells (layer2/nge_layer2.h).
 *
 * A Layer 2 stream draws each character as a glyph in its cell, in its ink
 * on its paper, and interprets the text control codes (io/nge_text.h)
 * exactly as a tilemap stream does: the same cursor, delayed wrap, clip
 * mode and scrolling. INK takes a text colour (NGE_INK_*), shown as the
 * colour in this stream's table, and any other value as white. ATTR sets
 * the ink to a raw Layer 2 colour (RRRGGGBB), and PAPER the paper. Clearing
 * fills with the paper.
 *
 * A stream draws on one Layer 2 buffer, named when it is opened, whichever
 * buffer is shown.
 *
 * It draws a character at a time (nge_layer2_glyph(), a banked call). Its
 * output function is called through a pointer, so it is in common memory
 * (N-BANK-3). Layer 2 must be set up (nge_layer2_init()) before a stream
 * draws.
 */

#include <stdint.h>

#include "core/nge_types.h"
#include "io/nge_stream.h"
#include "io/nge_text.h"

/**
 * @brief Open an output stream on Layer 2 cells. The cursor starts at the
 *        top-left, in white; the cells are not cleared.
 * @param buffer The Layer 2 buffer it draws on (0 to NGE_LAYER2_BUFFERS - 1).
 * @param cells Cells, inclusive, within 40 x 32.
 * @param paper Background colour (RRRGGGBB): what clearing fills with, until PAPER changes it.
 * @return The stream, or NULL if no stream slot is free.
 */
NGE_FILE *nge_open_layer2(uint8_t buffer, const nge_rect_t *cells, uint8_t paper);

#endif
