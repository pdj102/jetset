#ifndef PLAYER_H
#define PLAYER_H

/**
 * @file player.h
 * @brief The player: what their keys do, their screens (inventory,
 *        dropping, firing), their turn, and the view following them.
 *
 * The player entity is actor_player (game/actors.h). Their timer
 * (TIMER_PLAYER) fires every `speed` ticks: player_on_turn() stops the
 * clock, and the game passes key events to player_command() until a
 * command uses the turn, which starts the clock again. A command that is
 * refused (a wall in the way) tells the player why and keeps their turn.
 *
 * The screens are game states (game/game.h): their entry actions and
 * updates are here, called by game_set_state() and the frame loop.
 *
 * Banking: player_on_turn() and player_on_event() are in common memory
 * (called for every timer firing and event); the commands and screens are
 * banked (NGE_BANKED, page 114): don't call them while game code has its
 * own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "input/nge_input.h"

/** @brief The player's timer fired (TIMER_PLAYER): stop the clock until a command uses the turn. */
void player_on_turn(nge_entity_id_t id);

/** @brief React to events about the player: the camera follows them (EV_MOVED). */
void player_on_event(const nge_event_t *e);

/**
 * @brief Carry out the command for a key event, during the player's turn:
 *        movement keys (cursor keys or h j k l) move, g (or ,) picks up the
 *        item here, i d f open the inventory, drop and firing screens, Q
 *        quits. Other keys do nothing.
 */
void player_command(nge_key_event_t ev) NGE_BANKED;

/** @brief GAME_INVENTORY: list what the player carries ("a) sword  b) potion"); any key returns to play. */
void player_inventory_enter(void) NGE_BANKED;
void player_inventory_update(void) NGE_BANKED;

/** @brief GAME_DROP: list the items and ask which; its letter drops it, any other key returns to play. */
void player_drop_enter(void) NGE_BANKED;
void player_drop_update(void) NGE_BANKED;

/** @brief GAME_TARGET: ask for a direction to fire in (nothing to fire yet). */
void player_target_enter(void) NGE_BANKED;
void player_target_update(void) NGE_BANKED;

#endif
