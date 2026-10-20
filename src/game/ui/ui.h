#ifndef UI_H
#define UI_H

/**
 * @file ui.h
 * @brief Screens shown in the map window: the player's item screens
 *        (inventory, drop, equip, take off, use), as choosers (game/game.h).
 *
 * A screen draws a title and a lettered list of items on an opaque panel
 * over the map window (screen_panel, game/screen.h), and sets the help
 * bar. It waits for a key (game_wait_key()), then makes the panel
 * transparent again (screen_show_map()), so the map and sprites under it
 * show as they were, shows play's help and returns. Each item
 * screen is a row of a table of descriptions (ui_bank.c): its title, its
 * help, what to say when it has nothing to list, and which items it lists,
 * so a new one (read, zap) is a new row and a UI_* number.
 *
 * A screen only chooses: the command that opened it does the action with
 * the item it returns (game/player/player.h).
 *
 * Banking: everything here is banked (NGE_BANKED, page 116), as screens
 * run on a key press, not every frame. Don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "input/nge_input.h"

/* The item screens: rows of the description table */
#define UI_INVENTORY  0   /**< What the player carries and uses: ui_show_inventory(). */
#define UI_DROP       1   /**< Choose an item to drop. */
#define UI_EQUIP      2   /**< Choose an item to equip. */
#define UI_UNEQUIP    3   /**< Choose an item to take off. */
#define UI_USE        4   /**< Choose an item to use up: drink, eat. */
#define UI_SCREEN_COUNT    5

/**
 * @brief Show a choosing screen (any UI_* but UI_INVENTORY) and wait for
 *        the player's choice. A screen with nothing to list isn't shown:
 *        it says so in the message window.
 * @return The item whose letter was pressed, or NGE_ENTITY_INVALID (another
 *         key: "Never mind."; or nothing to choose).
 */
nge_entity_id_t ui_choose_item(uint8_t screen) NGE_BANKED;

/** @brief Show the inventory until any key. */
void ui_show_inventory(void) NGE_BANKED;

/**
 * @brief A choosing screen's answer to one key event, for a list of n
 *        items (ids, on the caller's stack): the item for a letter a to the
 *        nth, or NGE_ENTITY_INVALID for any other key ("Never mind.").
 */
nge_entity_id_t ui_item_key(const nge_entity_id_t *ids, uint8_t n, nge_key_event_t ev) NGE_BANKED;

#endif
