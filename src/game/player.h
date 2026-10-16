#ifndef PLAYER_H
#define PLAYER_H

/**
 * @file player.h
 * @brief The player: what their keys do, their turn, and the view
 *        following them.
 *
 * The player entity is actor_player (game/actors.h). Their timer
 * (TIMER_PLAYER) fires every `speed` ticks: player_on_turn() stops the
 * clock, and the game passes key events to player_command() until a
 * command uses the turn, which starts the clock again. A command that is
 * refused (a wall in the way) tells the player why and keeps their turn.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "input/nge_input.h"

/** @brief The player's timer fired (TIMER_PLAYER): stop the clock until a command uses the turn. */
void player_on_turn(nge_entity_id_t id);

/**
 * @brief Carry out the command for a key event, during the player's turn:
 *        movement keys move, others open a mode (inventory, firing) or quit
 *        (through game_set_state()). Other keys do nothing.
 */
void player_command(nge_key_event_t ev);

/**
 * @brief The direction of a movement key: the cursor keys (also CAPS+5-8)
 *        or h j k l.
 * @return 1 and the direction in (dx, dy), or 0 if ev is not a movement key.
 */
uint8_t player_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy);

/** @brief React to events about the player: the camera follows them (EV_MOVED). */
void player_on_event(const nge_event_t *e);

#endif
