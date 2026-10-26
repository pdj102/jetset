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
 *     screen_title_run();          the title, then the player's name (game/ui/screen_title.h)
 *     reason = play_run();         one game, from the first level to game over
 *     over_run(reason);            why it ended, until a key
 *
 * Frames. Every loop that runs frames does the drawing in each one:
 * game_frame_start() first (wait for vblank, upload the sprites and the
 * overlay sprites, redraw the map if it changed, read the keyboard) and
 * game_frame_end() last (build the next frame's sprites). Waiting for the
 * player goes through game_wait_key(), which runs such frames until a key
 * event arrives: the screen stays live, and nothing else happens. The one
 * other kind of frame is the paused frame of an overlay screen (below).
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
 * change of level is asked for, when the game is over, or while an
 * animation plays in view. Then the player acts: no time passes, and one
 * key event a frame goes to player_command() until a command uses the
 * turn; keys wait, queued, while an animation plays.
 *
 * Animations (game/anim/anim.h). Actions are instant; the events they post
 * start the animations (a slide after a move, a lunge after an attack),
 * which play over the following frames while the turn engine waits
 * (anim_holding()). The camera follows the player's slide a pixel at a
 * time (nge_render_follow()).
 *
 * Paused frames. An overlay screen (game/ui/screen.h) pauses the game: its
 * frames are game_pause_frame(), which only waits for the blanking period
 * and reads the keyboard, so nothing on the level moves, not even the
 * animations, until play's frames start again.
 *
 * Choosers. A command that needs a choice (an item, a direction, a tile)
 * calls a chooser, which shows its prompt or screen and waits until the
 * player answers: a prompt over the map with game_wait_key(), puts back
 * the help line (game_play_help()); a screen with screen_wait_key(),
 * paused, and screen_close() shows the main screen again. It returns the
 * answer, and the command then does the action. No time passes while a
 * chooser waits. The choosers are screen_inv_run()
 * (game/ui/screen_inv.h), player_ask_direction()
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
 *        and the overlay sprites (game/ui/screen_main.h), redraw the map if it
 *        changed, and read the keyboard. If the last frame wasn't ended
 *        (game_frame_end()), it is ended first, so the sprites uploaded are
 *        always built through the camera as it is now.
 */
void game_frame_start(void);

/**
 * @brief End a frame: advance the animations and motions, move the camera
 *        with the player's slide, and build the next frame's sprites.
 */
void game_frame_end(void);

/**
 * @brief Run a paused frame, for an overlay screen (game/ui/screen.h):
 *        wait for the blanking period and read the keyboard. Nothing is
 *        uploaded or advanced: the sprites, the map and the animations stay
 *        as they were. If the last frame wasn't ended, it is ended first.
 */
void game_pause_frame(void);

/**
 * @brief Wait for a key, running frames that only draw.
 * @param frames The most frames to wait, or 0 to wait for ever.
 * @return The next key event: at once if one is waiting; or NGE_KEY_NONE
 *         once frames have passed with none.
 */
nge_key_event_t game_wait_key(uint16_t frames);

/**
 * @brief Draw the level's screen afresh, for a new game or a new level:
 *        the level's sprites,
 *        the map, the stats, the turn, the depth, and play's help.
 */
void game_play_redraw(void);

/** @brief Show play's keys on the help bar (prompts call it when they end). */
void game_play_help(void);

#endif
