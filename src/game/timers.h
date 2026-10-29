#ifndef TIMERS_H
#define TIMERS_H

/**
 * @file timers.h
 * @brief The game's timer kinds (nge/ecs/systems/nge_timer.h).
 *
 * Time is in ticks, NGE_TIMER_TICKS_PER_TURN (10) to a turn. An actor's
 * timer fires every `speed` ticks (game/components/stats.h): at speed 10
 * it acts once a turn, whatever the action. The game's dispatch function
 * (game.c) calls the handler for each kind.
 *
 * | Kind          | Entity                  | Handler                                     |
 * |---------------|-------------------------|---------------------------------------------|
 * | TIMER_PLAYER  | the player              | player_on_turn(): their turn: no time       |
 * |               |                         | passes until a command uses it (player.h)   |
 * | TIMER_EFFECT  | an active effect, owned | effects_on_timer(): its op each period, and |
 * |               | by what it affects      | its end after its cycles (game/effects)     |
 * | TIMER_AI      | an entity with the AI   | ai_on_turn(): its state's handler           |
 * |               | component: a monster    | (game/ai/ai.h)                              |
 */

#include "ecs/systems/nge_timer.h"

#define TIMER_PLAYER   0   /**< The player's turn: wait for a command. */
#define TIMER_EFFECT   1   /**< An active effect: its period's op; it ends on its last cycle. */
#define TIMER_AI       2   /**< An AI entity's turn: the AI decides what it does. */
#define TIMER_COUNT    3   /**< The number of timer kinds. */

#endif
