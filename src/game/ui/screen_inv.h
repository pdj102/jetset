#ifndef SCREEN_INV_H
#define SCREEN_INV_H

/**
 * @file screen_inv.h
 * @brief The inventory screen: an overlay screen (game/ui/screen.h) listing
 *        what the player has, to choose an item and what to do with it.
 *        A chooser (game/game.h).
 *
 *     col 0 1 2   5 6                  25 26           39
 *     row 0  |   title (centred)                Pack 12/20 |
 *         2  |     1 short sword        1d6 [+1,+0]     |  in use: cyan
 *            |    15 sheaf of arrows    [+0,+0]         |
 *            |     1 potion of healing                  |  the pack: white
 *        28  |                                          |
 *        29  | status: an action's words, or why it isn't allowed |
 *        31  | help line: the keys for the selected item |
 *
 * Each row is a count (a stack's, or 1), the name and its details: an
 * attack's dice and bonuses, ammo's bonuses, armour's AC, a wand's
 * charges... (docs/ui_inventory.md).
 *
 * It opens in one of two modes:
 * - browse (SCREEN_INV_BROWSE, the i key): everything the player has. The
 *   cursor keys (or j and k) select; w wear, x exchange (wear in place
 *   of what is in the slot), r take off, d drop, u use, a aim, ENTER the item's obvious action (take off what is in use, use
 *   what can be used, aim what can be aimed, wear the rest). Wear, exchange,
 *   take off, drop and use are done here, the screen staying open, as many as
 *   the player likes for one turn: their words show on the status line
 *   and in the message window, and their events are handled at once
 *   (game_pause_events()). Aim needs the map, so it closes the screen and
 *   is returned. The help line lists the actions allowed for the selected
 *   item. If the player dies, the screen closes.
 * - choose, for an action (ACT_DROP, ACT_EQUIP, ACT_UNEQUIP, ACT_USE,
 *   ACT_AIM, ACT_UNLOCK, ACT_EXCHANGE: the d, w, r, u, a, p and e keys):
 *   only the items it applies to, lettered; a letter, or the cursor and
 *   ENTER, chooses, and the screen closes. An exchange is done here, in
 *   either mode, and not returned: a ring with both ring slots in use
 *   first asks which ring goes, a list of the two, lettered. With nothing to list, it isn't opened: it says so
 *   in the message window.
 *
 * SPACE goes back. An action is offered only if the game would allow it:
 * each is asked in check mode (ACT_CHECK, game/actions/act.h), and a
 * refusal's reason (game/messages/messages.h) shows on the status line,
 * the screen staying open. An action returned is only chosen: the command
 * does it with the item (game/player/player.h), using the turn.
 *
 * Banking: banked (NGE_BANKED, page 116). Don't call it while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

#include "game/actions/act.h"

#define SCREEN_INV_BROWSE  ACT_COUNT   /**< screen_inv_run()'s mode: browse, no action chosen yet. */
#define SCREEN_INV_BACK    ACT_COUNT   /**< screen_inv_run()'s answer: nothing to do. */

/**
 * @brief Show the inventory, do what the player does in it (browse mode),
 *        and wait for the player's choice.
 * @param mode SCREEN_INV_BROWSE, or the action to choose an item for:
 *             ACT_DROP, ACT_EQUIP, ACT_UNEQUIP, ACT_USE, ACT_AIM, ACT_UNLOCK
 *             or ACT_EXCHANGE (done here: SCREEN_INV_BACK, *used set).
 * @param item Where the chosen item goes (on the caller's stack); left as
 *             it was when nothing is chosen.
 * @param used Set to 1 if actions done in the screen used the turn, else
 *             0 (on the caller's stack): the turn is used even if the
 *             action returned is then given up (an aim with no target).
 * @return The action to do with *item (ACT_*: in choose mode, mode
 *         itself), or SCREEN_INV_BACK.
 */
uint8_t screen_inv_run(uint8_t mode, nge_entity_id_t *item, uint8_t *used) NGE_BANKED;

#endif
