#ifndef PLAYER_H
#define PLAYER_H

/**
 * @file player.h
 * @brief The player: what their keys do, the direction prompts, their turn,
 *        and the view following them. The item screens are in game/ui.
 *
 * The player entity is actor_player (game/actors.h). Their timer
 * (TIMER_PLAYER) fires every `speed` ticks: player_on_turn() sets
 * player_turn, and while it is set time stands still (the game advances
 * no timers) and the game passes key events to player_command(). A command
 * that uses the turn clears it, and time passes again. A command that is
 * refused (a wall in the way) tells the player why and keeps their turn.
 * The rest of the tick the player's timer fired in still runs first, so
 * everything due at the same time acts.
 *
 * Each direction prompt (fire, close) is a game state (game/game.h): its
 * entry action and update are here, called by game_set_state() and the
 * frame loop.
 *
 * Banking: player_init(), player_on_turn() and player_on_event() are in
 * common memory (called for every timer firing and event); the commands
 * and screens are
 * banked (NGE_BANKED, page 114): don't call them while game code has its
 * own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "game/actions/act.h"
#include "input/nge_input.h"

/** 1 while it is the player's turn: no time passes until a command uses it. Read-only outside game/player. */
extern uint8_t player_turn;

/** @brief Reset the module for a new game: not the player's turn. */
void player_init(void);

/** @brief The player's timer fired (TIMER_PLAYER): it is their turn (player_turn) until a command uses it. */
void player_on_turn(nge_entity_id_t id);

/**
 * @brief React to events about the player: the camera follows them
 *        (EV_MOVED), the stats window shows their hit points (EV_DAMAGED,
 *        EV_HEALED) and all their stats (EV_ATTRS_CHANGED).
 */
void player_on_event(const nge_event_t *e);

/** @brief Show the player's hit points, AC, abilities and statuses in the stats window (banked, page 114). */
void player_show_stats(void) NGE_BANKED;

/**
 * @brief Carry out the command for a key event, during the player's turn:
 *        movement keys (cursor keys or h j k l) move (into a creature:
 *        attack; into a closed door: open it), g (or ,) picks up the item
 *        here, i d w r u open the inventory, drop, equip, take off and use
 *        screens, f and c the fire and close prompts, x looks (targeting,
 *        game/target/target.h), Q quits. Other keys do nothing.
 */
void player_command(nge_key_event_t ev) NGE_BANKED;

/**
 * @brief The direction of a movement key: the cursor keys (also CAPS+5-8)
 *        or h j k l. The one key map for moving, in play, the direction
 *        prompts and targeting.
 * @return 1 and the direction in (dx, dy), or 0 (and (0, 0)) for any other key.
 */
uint8_t player_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy) NGE_BANKED;

/**
 * @brief Do an action (ACT_*, game/actions/act.h) for the player: on a
 *        refusal they are told why and keep their turn; if the turn is
 *        used, their turn ends and time passes again. a (on the caller's stack) holds the
 *        inputs; the actor is set here.
 */
void player_act(uint8_t action, act_t *a) NGE_BANKED;

/* Direction prompts: rows of a table (player_bank.c), one game state each */
#define PLAYER_PROMPT_FIRE   0   /**< GAME_FIRE: fire (nothing to fire yet). */
#define PLAYER_PROMPT_CLOSE  1   /**< GAME_CLOSE: close the door in that direction (act_close()). */
#define PLAYER_PROMPT_COUNT       2

/** @brief A direction prompt's entry action: ask, and set the help bar. */
void player_direction_enter(uint8_t prompt) NGE_BANKED;

/** @brief A direction prompt's update, once per frame: a key event goes to player_direction_key(). */
void player_direction_update(uint8_t prompt) NGE_BANKED;

/**
 * @brief A direction prompt's answer to a key event: a direction does the
 *        prompt's action that way (player_act()), another key says "Never
 *        mind.". Then back to play.
 */
void player_direction_key(uint8_t prompt, nge_key_event_t ev) NGE_BANKED;

#endif
