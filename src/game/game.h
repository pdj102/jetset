#ifndef GAME_H
#define GAME_H

/**
 * @file game.h
 * @brief The main loop: the title, a game and the game-over screen, one
 *        after another; the frames every loop runs; and waiting for a key.
 *        See docs/game_loop.md.
 *
 * game_run() is plain code, for ever:
 *
 *     title_run();                 the title, then the player's name (game/title/title.h)
 *     reason = play_run();         one game, from the first level to game over
 *     over_run(reason);            why it ended, until a key
 *
 * Frames. Every loop that runs frames does the drawing in each one:
 * game_frame_start() first (wait for vblank, upload the sprites and the
 * overlay sprites, redraw the map if it changed, read the keyboard) and
 * game_frame_end() last (build the next frame's sprites). Waiting for the
 * player goes through game_wait_key(), which runs such frames until a key
 * event arrives: the screen stays live, and nothing else happens.
 *
 * The play loop is the only place the world is stepped. Each of its frames:
 *
 *     game_frame_start()
 *     the turn:   the world acts, or the player's command
 *     the step:   handle the frame's events, change level if asked to
 *                 (level_request(), game/level/level.h), release dying
 *                 entities, check the ECS guard
 *     game_frame_end()
 *
 * The turn is a turn engine with two phases. While it isn't the player's
 * turn, the world acts: the clock advances (nge_timer_advance(),
 * ecs/systems/nge_timer.h) from one tick at which timers are due to the
 * next, firing each entity's timer there (monsters' turns, effects'
 * ticks), at most CLOCK_ADVANCES_MAX (game.c) times a frame. It stops when
 * the player's timer fires (player_turn, game/player/player.h), when a
 * change of level is asked for, or when the game is over. Then the player
 * acts: no time passes, and one key event a frame goes to
 * player_command() until a command uses the turn.
 *
 * Choosers. A command that needs a choice (an item, a direction, a tile)
 * calls a chooser, which shows its screen or prompt, waits with
 * game_wait_key() until the player answers, puts back what it covered and
 * returns the answer; the command then does the action. No time passes
 * while a chooser waits. The choosers are ui_choose_item() and
 * ui_show_inventory() (game/ui/ui.h), player_ask_direction()
 * (game/player/player.h) and target_choose() (game/target/target.h).
 *
 * Events are handled in three places, each for a reason: after each
 * advance of the clock (consequences in time order), in the step (the
 * player's command), and after a change of level (the arrival, before the
 * clean-up releases what it names).
 *
 * A game ends only through game_over(): the clock stops, so nothing more
 * acts even in the frame's remaining advances, and the play loop ends at
 * the end of that frame, once its events (the death message) are handled.
 */

#include <stdint.h>

#include "input/nge_input.h"

/** Why the game ended (game_over()). */
#define GAME_OVER_DIED     0   /**< The player died. */
#define GAME_OVER_QUIT     1   /**< The player quit. */
#define GAME_OVER_ESCAPED  2   /**< The player climbed out of the dungeon (later: with the quest object, a win). */
#define GAME_OVER_COUNT    3   /**< Number of reasons. */

/** @brief Run the game, starting at the title. Never returns. */
void game_run(void);

/**
 * @brief End the game for reason (GAME_OVER_*): the clock stops, and the
 *        play loop ends at the end of this frame. Only during a game.
 */
void game_over(uint8_t reason);

/**
 * @brief Start a frame: wait for the blanking period, upload the sprites
 *        and the overlay sprites (game/screen.h), redraw the map if it
 *        changed, and read the keyboard. If the last frame wasn't ended
 *        (game_frame_end()), it is ended first, so the sprites uploaded are
 *        always built through the camera as it is now.
 */
void game_frame_start(void);

/** @brief End a frame: build the next frame's sprites (and advance their animations). */
void game_frame_end(void);

/**
 * @brief Wait for a key, running frames that only draw.
 * @param frames The most frames to wait, or 0 to wait for ever.
 * @return The next key event: at once if one is waiting; or NGE_KEY_NONE
 *         once frames have passed with none.
 */
nge_key_event_t game_wait_key(uint16_t frames);

/**
 * @brief Draw the level's screen afresh, for a new game or a new level:
 *        the map view uncovered (screen_show_map()), the level's sprites,
 *        the map, the stats, the turn, the depth, and play's help.
 */
void game_play_redraw(void);

/** @brief Show play's keys on the help bar (prompts call it when they end). */
void game_play_help(void);

#endif
