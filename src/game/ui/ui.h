#ifndef UI_H
#define UI_H

/**
 * @file ui.h
 * @brief Screens shown in the map window: the player's item screens
 *        (inventory, drop, equip, take off, use).
 *
 * A screen hides the sprites, draws a title and a lettered list of items
 * over the map window (screen_panel, game/screen.h), and sets the help
 * bar. Each item screen is a row of a table of descriptions (ui_bank.c):
 * its title, its help, which items it lists and what choosing one does,
 * so a new one (read, zap) is a new row and a new game state.
 *
 * Leaving a screen returns to GAME_PLAY, whose entry action redraws the map
 * and the stats and shows the sprites again (game.c).
 *
 * Banking: everything here is banked (NGE_BANKED, page 116), as screens
 * run on a key press, not every frame. Don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "input/nge_input.h"

/* The item screens: rows of the description table */
#define UI_INVENTORY  0   /**< What the player carries and uses; any key: back. */
#define UI_DROP       1   /**< Choose an item to drop. */
#define UI_EQUIP      2   /**< Choose an item to equip. */
#define UI_UNEQUIP    3   /**< Choose an item to take off. */
#define UI_USE        4   /**< Choose an item to use up: drink, eat. */
#define UI_SCREENS    5

/**
 * @brief Entry action of an item screen's state: show the screen. A screen
 *        for choosing with nothing to choose says so and returns to play.
 */
void ui_items_enter(uint8_t screen) NGE_BANKED;

/** @brief An item screen's update, once per frame: a key event goes to ui_items_key(). */
void ui_items_update(uint8_t screen) NGE_BANKED;

/**
 * @brief An item screen's answer to a key event: act on the item a letter
 *        chooses (player_act()), or say "Never mind." for another key; any
 *        key leaves a screen that only shows. Then back to play.
 */
void ui_items_key(uint8_t screen, nge_key_event_t ev) NGE_BANKED;

#endif
