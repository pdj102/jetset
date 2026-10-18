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
 *     3. post-update: handle events, change level if asked to (level_request(),
 *                     game/level/level.h), release destroyed entities, build
 *                     the next frame's sprites, check the ECS guard
 *
 * Each state's update runs once per frame, takes at most what it needs from
 * the key events (nge_key_get()) and returns: nothing in it waits, so the
 * screen keeps animating. Change state with game_set_state(), which throws
 * away waiting key events (keys typed for one mode don't leak into the next)
 * and runs the new state's entry action.
 */

#include <stdint.h>

/** Game states. */
typedef enum game_state {
    GAME_TITLE,       /**< Title: any key asks the player's name and starts a new game. */
    GAME_NEW_GAME,    /**< Passing: its entry action resets everything for a new game, then GAME_PLAY. */
    GAME_PLAY,        /**< Time passes (nge_timer) until the player's turn, then one command per frame. */
    GAME_INVENTORY,   /**< The inventory is shown; any key returns to play. */
    GAME_DROP,        /**< Choosing an item to drop: its letter drops it, any other key returns to play. */
    GAME_EQUIP,       /**< Choosing an item to equip. */
    GAME_UNEQUIP,     /**< Choosing an item to take off. */
    GAME_USE,         /**< Choosing an item to use up. */
    GAME_FIRE,        /**< Choosing a direction to fire in. */
    GAME_CLOSE,       /**< Choosing a direction to close a door in. */
    GAME_OVER,        /**< The game is over (game_over()); any key returns to the title. */
    GAME_STATE_COUNT       /**< Number of states. */
} game_state_t;

/** Why the game ended (game_over()). */
#define GAME_OVER_DIED     0   /**< The player died. */
#define GAME_OVER_QUIT     1   /**< The player quit. */
#define GAME_OVER_ESCAPED  2   /**< The player climbed out of the dungeon (later: with the quest object, a win). */
#define GAME_OVER_COUNT    3   /**< Number of reasons. */

/** The current state: read-only, change it with game_set_state(). */
extern game_state_t game_state;

/** @brief Change state: flush the key events and run the new state's entry action. */
void game_set_state(game_state_t state);

/** @brief End the game for reason (GAME_OVER_*): the clock stops, and the game-over state says why. */
void game_over(uint8_t reason);

/** @brief Run the game, starting at the title. Never returns. */
void game_run(void);

#endif
