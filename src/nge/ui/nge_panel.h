#ifndef NGE_PANEL_H
#define NGE_PANEL_H

/**
 * @file nge_panel.h
 * @brief Panels: the rectangles of Layer 2 cells a screen is made of.
 *
 * A panel is a rectangle of cells with a paper colour. A text panel is
 * drawn through a Layer 2 stream (io/nge_l2_stream.h), opened on it with
 * its paper; a panel whose paper is NGE_LAYER2_TRANSPARENT is a hole,
 * showing the layers under Layer 2 (the tilemap and the sprites), and has
 * no stream. A screen describes its panels in a table and opens them all
 * at once:
 *
 *     static const nge_panel_t panels[2] = {
 *         { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, BLACK, NGE_PANEL_CLIP },    // title
 *         { { .x1 = 0, .x2 = 39, .y1 = 2, .y2 = 30 }, BLACK, NGE_PANEL_SCROLL }, // log
 *     };
 *     NGE_FILE *streams[2];
 *
 *     if (nge_panel_open_all(1, panels, 2, streams)) {   // on buffer 1
 *         nge_panel_centre(&panels[0], 0, "Title", streams[0]);
 *         ...
 *     }
 *     nge_panel_close_all(streams, 2);
 *
 * Banking: common memory, so a banked caller can pass tables and strings
 * in its own page; the fills are banked calls to layer2/.
 */

#include <stdint.h>

#include "core/nge_types.h"
#include "io/nge_stream.h"

/* Panel modes: how its stream handles the edges */
#define NGE_PANEL_CLIP    1   /**< Characters past the right edge are dropped; never scrolls (fields). */
#define NGE_PANEL_SCROLL  0   /**< Wraps at the right edge and scrolls at the bottom (a log). */

/** A panel: cells, paper (NGE_LAYER2_TRANSPARENT for a hole) and mode (NGE_PANEL_*). */
typedef struct nge_panel {
    nge_rect_t area;    /**< Cells, inclusive, within 40 x 32. */
    uint8_t    paper;   /**< Background colour (RRRGGGBB), or NGE_LAYER2_TRANSPARENT for a hole. */
    uint8_t    mode;    /**< NGE_PANEL_CLIP or NGE_PANEL_SCROLL. */
} nge_panel_t;

/** @brief Fill a panel of Layer 2 buffer b with its paper: a hole is made transparent. */
void nge_panel_fill(uint8_t b, const nge_panel_t *p);

/**
 * @brief Fill n panels of Layer 2 buffer b with their papers and open a
 *        stream on each text panel, in its mode, into streams (NULL for a
 *        hole).
 * @return 1, or 0 if a stream slot ran out: the streams opened are closed
 *         again, and every entry of streams is NULL.
 */
uint8_t nge_panel_open_all(uint8_t b, const nge_panel_t *panels, uint8_t n, NGE_FILE **streams);

/** @brief Close n streams opened by nge_panel_open_all() (NULL entries are skipped), and set them to NULL. */
void nge_panel_close_all(NGE_FILE **streams, uint8_t n);

/**
 * @brief Write s centred on row (relative to the panel) of panel p, on its
 *        stream; text wider than the panel starts at its left edge.
 */
void nge_panel_centre(const nge_panel_t *p, uint8_t row, const char *s, NGE_FILE *stream);

#endif
