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

/**
 * Panel stream over the map view (screen_map_view), clip mode: screens
 * like the inventory draw here (game/ui/ui.h). Drawing over the map
 * means it must be redrawn afterwards (nge_map_invalidate()).
 */
extern NGE_FILE *screen_panel;

/** Longest player name: it follows "Name: " on one stats line. */
#define SCREEN_NAME_MAX  6

/** The player's name, shown in the stats window (asked for by the title, game/title/title.h). */
extern char screen_name[SCREEN_NAME_MAX + 1];

/**
 * @brief Set up the tilemap, its palettes and the screen's areas:
 *
 *     rows 0-26   map view (28 wide) | stats (12 wide)
 *     rows 27-30  messages (4 lines)
 *     row  31     help bar: what the keys do in the current state
 */
void screen_init(void);

/** @brief Draw the stats window: the hit points, the armour class, the turn, the statuses, and placeholders for the rest. */
void screen_draw_stats(uint16_t turn, uint8_t hp, uint8_t hp_max, uint8_t ac, uint8_t status);

/** @brief Show the hit points in the stats window: "HP   12/ 12". */
void screen_show_hp(uint8_t hp, uint8_t hp_max);

/** @brief Show the armour class in the stats window: "AC        10". */
void screen_show_ac(uint8_t ac);

/** @brief Show the statuses (STATUS_*, game/components/attrs.h) in the stats window: "Poisoned", or nothing. */
void screen_show_status(uint8_t status);

/** @brief Show the turn count in the stats window. */
void screen_show_turn(uint16_t turn);

/**
 * @brief Add a message on a new line of the message window: the newest
 *        stays on the bottom line, older ones scroll up.
 */
void screen_message(const char *text);

/** @brief Show text on the help bar (row 31), replacing what was there: what the keys do now. */
void screen_help(const char *text);

/**
 * @brief Start a new line in the message window for writing to
 *        screen_messages directly (the title's name prompt): the next
 *        message then goes on the line after it.
 */
void screen_message_line(void);

#endif
