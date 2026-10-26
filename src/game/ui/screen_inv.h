#ifndef SCREEN_INV_H
#define SCREEN_INV_H

/**
 * @file screen_inv.h
 * @brief The inventory screen: an overlay screen (game/ui/screen.h) listing
 *        what the player has, to choose an item and what to do with it.
 *        A chooser (game/game.h).
 *
 *     col 0                         27 28       39
 *     row 0  |          title (centred)          |
 *         2  | the list: in use, then | the item's|
 *            | the pack; the cursor   | name      |
 *            | row highlighted        |           |
 *        28  |                        | Pack  n/m |
 *        29  | status: why an action isn't allowed |
 *        31  | help line: the keys for the selected item |
 *
 * It opens in one of two modes:
 * - browse (SCREEN_INV_BROWSE, the i key): everything the player has. The
 *   cursor keys (or j and k) select; w wear, r take off, d drop, u use,
 *   ENTER the item's obvious action (take off what is in use, use what can
 *   be used, wear the rest). The help line lists the actions allowed for
 *   the selected item.
 * - choose, for an action (ACT_DROP, ACT_EQUIP, ACT_UNEQUIP, ACT_USE: the
 *   d, w, r and u keys): only the items it applies to, lettered; a letter,
 *   or the cursor and ENTER, chooses. With nothing to list, it isn't opened:
 *   it says so in the message window.
 *
 * SPACE goes back. An action is offered only if the game would allow it:
 * each is asked in check mode (ACT_CHECK, game/actions/act.h), and a
 * refusal's reason (game/messages/messages.h) shows on the status line,
 * the screen staying open. An allowed action closes the screen and is
 * returned: it only chooses, and the command does the action with the
 * item (game/player/player.h), using the turn.
 *
 * Banking: banked (NGE_BANKED, page 116). Don't call it while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "input/nge_input.h"

#include "game/actions/act.h"

#define SCREEN_INV_BROWSE  ACT_COUNT   /**< screen_inv_run()'s mode: browse, no action chosen yet. */
#define SCREEN_INV_BACK    ACT_COUNT   /**< screen_inv_run()'s answer: nothing to do. */

/**
 * @brief Show the inventory, and wait for the player's choice.
 * @param mode SCREEN_INV_BROWSE, or the action to choose an item for:
 *             ACT_DROP, ACT_EQUIP, ACT_UNEQUIP or ACT_USE.
 * @param item Where the chosen item goes (on the caller's stack); left as
 *             it was when nothing is chosen.
 * @return The action to do with *item (ACT_*: in choose mode, mode
 *         itself), or SCREEN_INV_BACK.
 */
uint8_t screen_inv_run(uint8_t mode, nge_entity_id_t *item) NGE_BANKED;

#endif
