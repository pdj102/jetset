#ifndef SCREEN_MAIN_H
#define SCREEN_MAIN_H

/**
 * @file screen_main.h
 * @brief The main screen, of play (game/ui/screen.h): the map view, the
 *        stats window and the message window, on Layer 2 buffer
 *        SCREEN_BUFFER_MAIN.
 *
 *     col 0                    27 28       39
 *     row 0  +--------------------+----------+
 *            | map view  28 x 26  | stats    |
 *            | (nge_map, 14 x 13  | 12 x 26  |
 *            |  map tiles; sprites|          |
 *            |  clipped to it)    |          |
 *         26 +--------------------+----------+
 *            | messages  40 x 5              |
 *         31 | help line 40 x 1              |
 *            +-------------------------------+
 *
 * The map is on the tilemap, drawn by the level's nge_map, and nothing else
 * is. The panels are on Layer 2 (ui/nge_panel.h), above the sprites and
 * the tilemap: the map view is a hole in it, and the stats, messages and
 * help line are Layer 2 streams (io/nge_l2_stream.h) on a black
 * background. Overlay screens are drawn on the other buffer, so nothing
 * here is ever covered or drawn again: what is written here while one is
 * shown appears when it closes.
 *
 * Banking: the messages, the overlay sprites and the data are in common
 * memory: banked callers pass their literals to screen_message(), and the
 * frames upload the overlay sprites. Setting up and the stats are banked
 * (NGE_BANKED, page 116): don't call them while game code has its own
 * page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
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

/** The map view, in tilemap tiles (the window of the level's map). In common memory: nge_map reads it. */
extern const nge_rect_t screen_map_view;

/** Stats window stream: clip mode, the contents never move. */
extern NGE_FILE *screen_stats;

/** Message window stream: new lines scroll older messages up. Write with screen_message(), or call screen_message_line() first. */
extern NGE_FILE *screen_messages;

/** Longest player name: it follows "Name: " on one stats line. */
#define SCREEN_NAME_MAX  6

/** The player's name, shown in the stats window (asked for by the title, game/ui/screen_title.h). */
extern char screen_name[SCREEN_NAME_MAX + 1];

/**
 * @brief Set up the tilemap for the map, Layer 2 (nge_layer2_init()), the
 *        main screen's panels and help line on buffer SCREEN_BUFFER_MAIN,
 *        shown, and no overlay screen open:
 *
 *     rows 0-25   map view (28 wide, a hole onto the tilemap) | stats (12 wide)
 *     rows 26-30  messages (5 lines)
 *     row  31     help line: what the keys do now (screen_help(), game/ui/screen.h)
 */
void screen_init(void) NGE_BANKED;

/**
 * @brief Draw the stats window's fixed parts: the name, and a placeholder
 *        for the gold. The values are shown with screen_show_*().
 */
void screen_draw_stats(void) NGE_BANKED;

/** @brief Show the hit points in the stats window: "HP   12/ 12". */
void screen_show_hp(uint8_t hp, uint8_t hp_max) NGE_BANKED;

/** @brief Show the armour class in the stats window: "AC        10". */
void screen_show_ac(uint8_t ac) NGE_BANKED;

/**
 * @brief Show the six ability scores (by ABIL_*, game/components/stats.h), one a row: "Str      16".
 * @param scores In common memory (the caller's stack).
 */
void screen_show_abilities(const uint8_t *scores) NGE_BANKED;

/** @brief Show the statuses (STATUS_*, game/components/attrs.h) in the stats window: "Poisoned", or nothing. */
void screen_show_status(uint8_t status) NGE_BANKED;

/** @brief Show the depth (the current level) in the stats window: "Depth     2". */
void screen_show_depth(uint8_t depth) NGE_BANKED;

/** @brief Show the turn count in the stats window. */
void screen_show_turn(uint16_t turn) NGE_BANKED;

/**
 * @brief Add a message on a new line of the message window: the newest
 *        stays on the bottom line, older ones scroll up. While messages
 *        are sent elsewhere (screen_message_to()), it goes there instead.
 */
void screen_message(const char *text);

/**
 * @brief Send the messages to stream instead, from now on, until called
 *        with NULL: each one clears it and is written alone. An overlay
 *        screen uses it to show, on its own panel, what the game says to
 *        the player (game/messages/messages.h), such as why an action
 *        would be refused.
 */
void screen_message_to(NGE_FILE *stream);

/**
 * @brief Start a new line in the message window for writing to
 *        screen_messages directly: the next message then goes on the line
 *        after it.
 */
void screen_message_line(void);

/**
 * @brief Show overlay sprite n (SCREEN_OVERLAY_*) at screen position (x, y)
 *        with a 4-bit pattern (data/sprite_data.h) and its flags
 *        (NGE_SPRITE_PALETTE(BANK_*)...), from the next frame.
 */
void screen_overlay_set(uint8_t n, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags);

/** @brief Hide overlay sprite n, from the next frame. */
void screen_overlay_hide(uint8_t n);

/** @brief Send the overlay sprites to the hardware: once a frame, in the blanking period (game_frame_start()). */
void screen_overlay_upload(void);

#endif
