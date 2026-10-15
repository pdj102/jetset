#ifndef GAME_H
#define GAME_H

/**
 * @file game.h
 * @brief The main game loop and the game state.
 *
 * game_run() runs one frame at a time, for ever:
 *
 *     wait for vblank
 *     1. pre-update:  upload the sprites, redraw the map if it changed,
 *                     read the keyboard (nge_input_update)
 *     2. update:      the current state's update function
 *     3. post-update: handle events, release destroyed entities, build the
 *                     next frame's sprites, check the ECS guard
 *
 * Each state's update runs once per frame, takes at most what it needs from
 * the key events (nge_key_get()) and returns: nothing in it waits, so the
 * screen keeps animating. Change state with game_set_state(), which throws
 * away waiting key events (keys typed for one mode don't leak into the next)
 * and runs the new state's entry action.
 */

#include <stdint.h>

#include "input/nge_input.h"

/** Game states. */
typedef enum game_state {
    GAME_TITLE,       /**< Title: any key starts a game (asks the player's name). */
    GAME_PLAY,        /**< Time passes (nge_timer) until the player's turn, then one command per frame. */
    GAME_INVENTORY,   /**< The inventory is shown; any key returns to play. */
    GAME_TARGET,      /**< Choosing a direction to fire in. */
    GAME_DEAD,        /**< The game is over; any key returns to the title. */
    GAME_STATES       /**< Number of states. */
} game_state_t;

/** The current state: read-only, change it with game_set_state(). */
extern game_state_t game_state;

/** @brief Change state: flush the key events and run the new state's entry action. */
void game_set_state(game_state_t state);

/**
 * @brief The direction of a movement key: the cursor keys (also CAPS+5-8)
 *        or h j k l.
 * @return 1 and the direction in (dx, dy), or 0 if ev is not a movement key.
 */
uint8_t game_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy);

/** @brief Run the game, starting at the title. Never returns. */
void game_run(void);

#endif
