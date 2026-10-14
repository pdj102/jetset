#ifndef SCREEN_H
#define SCREEN_H

/**
 * @file screen.h
 * @brief Screen layout: the tilemap split into the map view, the stats
 *        window and the message window.
 *
 *     col 0                    27 28       39
 *     row 0  +--------------------+----------+
 *            | map view  28 x 27  | stats    |
 *            | (nge_map, sprites  | 12 x 27  |
 *            |  clipped to it)    |          |
 *         26 +--------------------+----------+
 *         27 | messages  40 x 5               |
 *         31 +-------------------------------+
 *
 * The map view is drawn directly by the level's nge_map; the stats and
 * message windows are tilemap streams.
 */

#include "core/nge_types.h"
#include "io/nge_stream.h"

/** The map view, in tilemap tiles (the window of the level's map). */
extern const nge_rect_t screen_map_view;

/** Stats window stream: clip mode, the contents never move. */
extern NGE_FILE *screen_stats;

/** Message window stream: new lines scroll older messages up. */
extern NGE_FILE *screen_messages;

/** @brief Set up the tilemap, its palettes and the stats and message windows. */
void screen_init(void);

/** @brief Draw the stats window (placeholder values for now). */
void screen_draw_stats(void);

/** @brief Add a line to the message window. */
void screen_message(const char *text);

#endif
