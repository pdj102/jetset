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

#include <stdint.h>

#include "core/nge_types.h"
#include "io/nge_stream.h"

/** The map view, in tilemap tiles (the window of the level's map). */
extern const nge_rect_t screen_map_view;

/** Stats window stream: clip mode, the contents never move. */
extern NGE_FILE *screen_stats;

/** Message window stream: new lines scroll older messages up. */
extern NGE_FILE *screen_messages;

/** Longest player name: it follows "Name: " on one stats line. */
#define SCREEN_NAME_MAX  6

/** The player's name, shown in the stats window. */
extern char screen_name[SCREEN_NAME_MAX + 1];

/** @brief Set up the tilemap, its palettes and the stats and message windows. */
void screen_init(void);

/** @brief Draw the stats window (placeholder values for now). */
void screen_draw_stats(void);

/** @brief Add a line to the message window. */
void screen_message(const char *text);

/**
 * @brief Ask for the player's name in the message window and read it from
 *        the keyboard into screen_name (blocking: runs frames until ENTER).
 *        An empty name stays "Rogue". Needs nge_input_init().
 */
void screen_ask_name(void);

/** @brief Show or clear the "SPACE held" marker in the stats window. */
void screen_show_held(uint8_t held);

#endif
