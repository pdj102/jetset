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

/** Message window stream: new lines scroll older messages up. Write with screen_message(), or call screen_message_line() first. */
extern NGE_FILE *screen_messages;

/** Longest player name: it follows "Name: " on one stats line. */
#define SCREEN_NAME_MAX  6

/** The player's name, shown in the stats window (asked for by the title, game/title/title.h). */
extern char screen_name[SCREEN_NAME_MAX + 1];

/** @brief Set up the tilemap, its palettes and the stats and message windows. */
void screen_init(void);

/** @brief Draw the stats window (placeholder values for now, and the turn). */
void screen_draw_stats(uint16_t turn);

/** @brief Show the turn count in the stats window. */
void screen_show_turn(uint16_t turn);

/**
 * @brief Add a message on a new line of the message window: the newest
 *        stays on the bottom line, older ones scroll up.
 */
void screen_message(const char *text);

/**
 * @brief Start a new line in the message window for writing to
 *        screen_messages directly (the title's name prompt): the next
 *        message then goes on the line after it.
 */
void screen_message_line(void);

#endif
