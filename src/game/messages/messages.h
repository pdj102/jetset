#ifndef MESSAGES_H
#define MESSAGES_H

/**
 * @file messages.h
 * @brief The message window's reactions to events and to the player's
 *        refused commands: what the player is told.
 *
 * Banking: banked (NGE_BANKED, page 118): don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "game/actions/act.h"

/**
 * @brief Add a message for the events the player should hear about. What a
 *        monster with AI does (wakes, notices them, opens, picks up,
 *        wields, drinks) is said only if they see it (ai_sees()).
 */
void messages_on_event(const nge_event_t *e) NGE_BANKED;

/** @brief Tell the player why their action failed (a->reason, a on the caller's stack), if they need telling. */
void messages_act_failed(const act_t *a) NGE_BANKED;

/** @brief Why the game ended (GAME_OVER_*, game/game.h), and to press a key. */
void messages_game_over(uint8_t reason) NGE_BANKED;

/**
 * @brief Look at tile (x, y) of the current level: one message for each game
 *        entity on it, in ID order ("You see a ring.", "You see yourself."),
 *        or "You see nothing there.".
 */
void messages_look(uint8_t x, uint8_t y) NGE_BANKED;

#endif
