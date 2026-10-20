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
 * The map is on the tilemap, drawn by the level's nge_map, and nothing else
 * is. The windows are on Layer 2 (layer2/nge_layer2.h), above the sprites
 * and the tilemap: the stats, messages and help bar are Layer 2 streams
 * (io/nge_l2_stream.h) on a black background, and the panel stream lies
 * over the map view, transparent until something is drawn on it.
 */

#include <stdint.h>

#include "core/nge_types.h"
#include "ecs/systems/nge_render.h"
#include "io/nge_stream.h"

/*
 * Hardware sprites: 0 to NGE_RENDER_SPRITES-1 are the render system's (the
 * level's entities), then the overlay sprites, drawn on top of them
 */
#define SCREEN_SPRITE_OVERLAY  NGE_RENDER_SPRITES   /**< The first overlay sprite's hardware number. */

/*
 * Overlay sprites: the game's own sprites, not entities, for things that
 * aren't on the map's level (the targeting cursor; later projectiles and
 * effects). Their records are in common memory, and the frame start
 * (game_frame_start(), game/game.h) uploads them every frame
 */
#define SCREEN_OVERLAY_CURSOR  0   /**< The targeting cursor (game/target/target.h). */
#define SCREEN_OVERLAY_COUNT   4   /**< Overlay sprites: hardware sprites SCREEN_SPRITE_OVERLAY onwards. */

/** The map view, in tilemap tiles (the window of the level's map). */
extern const nge_rect_t screen_map_view;

/** Stats window stream: clip mode, the contents never move. */
extern NGE_FILE *screen_stats;

/** Message window stream: new lines scroll older messages up. Write with screen_message(), or call screen_message_line() first. */
extern NGE_FILE *screen_messages;

/**
 * Panel stream over the map view (screen_map_view), clip mode: screens
 * like the inventory draw here (game/ui/ui.h). Clearing it
 * (nge_text_cls()) makes the map view an opaque black panel, which covers
 * the map and the sprites; screen_show_map() makes it transparent again,
 * and nothing under it needs drawing again.
 */
extern NGE_FILE *screen_panel;

/** @brief Make the map view transparent again: what a panel covered shows. */
void screen_show_map(void);

/** Longest player name: it follows "Name: " on one stats line. */
#define SCREEN_NAME_MAX  6

/** The player's name, shown in the stats window (asked for by the title, game/title/title.h). */
extern char screen_name[SCREEN_NAME_MAX + 1];

/**
 * @brief Set up the tilemap for the map, Layer 2 for the windows
 *        (nge_layer2_init()), and the screen's areas:
 *
 *     rows 0-26   map view (28 wide) | stats (12 wide)
 *     rows 27-30  messages (4 lines)
 *     row  31     help bar: what the keys do in the current state
 */
void screen_init(void);

/**
 * @brief Draw the stats window's fixed parts: the name, and a placeholder
 *        for the gold. The values are shown with screen_show_*().
 */
void screen_draw_stats(void);

/** @brief Show the hit points in the stats window: "HP   12/ 12". */
void screen_show_hp(uint8_t hp, uint8_t hp_max);

/** @brief Show the armour class in the stats window: "AC        10". */
void screen_show_ac(uint8_t ac);

/** @brief Show the six ability scores (by ABIL_*, game/components/stats.h), one a row: "Str      16". */
void screen_show_abilities(const uint8_t *scores);

/** @brief Show the statuses (STATUS_*, game/components/attrs.h) in the stats window: "Poisoned", or nothing. */
void screen_show_status(uint8_t status);

/** @brief Show the depth (the current level) in the stats window: "Depth     2". */
void screen_show_depth(uint8_t depth);

/** @brief Show the turn count in the stats window. */
void screen_show_turn(uint16_t turn);

/**
 * @brief Add a message on a new line of the message window: the newest
 *        stays on the bottom line, older ones scroll up.
 */
void screen_message(const char *text);

/**
 * @brief Show overlay sprite n (SCREEN_OVERLAY_*) at screen position (x, y)
 *        with an 8-bit pattern, from the next frame.
 */
void screen_overlay_set(uint8_t n, uint16_t x, uint16_t y, uint8_t pattern);

/** @brief Hide overlay sprite n, from the next frame. */
void screen_overlay_hide(uint8_t n);

/** @brief Send the overlay sprites to the hardware: once a frame, in the blanking period (game_frame_start()). */
void screen_overlay_upload(void);

/** @brief Show text on the help bar (row 31), replacing what was there: what the keys do now. */
void screen_help(const char *text);

/**
 * @brief Start a new line in the message window for writing to
 *        screen_messages directly (the title's name prompt): the next
 *        message then goes on the line after it.
 */
void screen_message_line(void);

#endif
