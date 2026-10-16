#ifndef TIMERS_H
#define TIMERS_H

/**
 * @file timers.h
 * @brief The game's timer kinds (nge/ecs/systems/nge_timer.h) and the time
 *        actions take.
 *
 * Time is in ticks, NGE_TIMER_TICKS_PER_TURN (10) to a turn: an action
 * costing 10 ticks takes a turn at normal speed. The game's dispatch
 * function (game.c) calls the handler for each kind.
 *
 * | Kind          | Entity                  | Handler                                     |
 * |---------------|-------------------------|---------------------------------------------|
 * | TIMER_PLAYER  | the player              | waits for a command (NGE_TIMER_WAIT)        |
 * | TIMER_GLOW    | an effect, owned by the | actors_on_glow(): a demo effect, by turns   |
 * |               | player                  | (removed when there are real effects)       |
 * | TIMER_MONSTER | a monster               | none yet: waits for its speed (one move)    |
 */

#include "ecs/systems/nge_timer.h"

#define TIMER_PLAYER   0   /**< The player's turn: wait for a command. */
#define TIMER_GLOW     1   /**< Demo effect: a message every few turns, then it fades. */
#define TIMER_MONSTER  2   /**< A monster's turn (no AI yet: it waits a move). */

/* Action costs, in ticks */
#define COST_MOVE  10     /**< Moving one tile. */

#endif
