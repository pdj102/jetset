#ifndef NGE_TIMER_H
#define NGE_TIMER_H

/**
 * @file nge_timer.h
 * @brief Timer system: one game clock for everything that acts or happens
 *        on a schedule (actors, effects).
 *
 * Time is counted in ticks, NGE_TIMER_TICKS_PER_TURN to a turn. Ticks are
 * simulated, not frames: nge_timer_advance() jumps straight to the next
 * tick at which a timer is due and fires it.
 *
 * Each entity with the timer component fires every `period` ticks (the
 * first time one period after nge_timer_add()): the game's dispatch
 * function is called with the entity, its timer's `kind` (which handler)
 * and its `cycles` left. After each firing the timer starts its next period
 * by itself; `cycles` limits how often it fires: it goes down by one each
 * time and the timer is removed when it reaches 0. A timer with cycles 0
 * fires until it is removed. The handler gets the cycles left counting this
 * firing: 1 is the last (time to say "the poison wears off"), 0 means no
 * limit.
 *
 * Actors: an actor's period is its speed, so every action takes one
 * period; change it with nge_timer_set_period() when the speed changes.
 * Effects (poison, regeneration, sleep) are entities with a timer owned by
 * their target (ecs/nge_owner.h): destroying the target destroys or moves
 * what it owns, so its effects go with it.
 *
 * The clock can be stopped and started: the player's handler stops it,
 * and the game starts it again once the player has used their turn.
 * nge_timer_stop() lets the rest of the current tick fire first, so
 * everything due at the same time acts.
 *
 *     static void game_on_timer(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
 *     {
 *         switch (kind) {
 *         case TIMER_PLAYER:
 *             nge_timer_stop();            // wait for a command
 *             break;
 *         case TIMER_MONSTER:
 *             monster_act(id);             // may be banked
 *             break;
 *         ...
 *         }
 *     }
 *
 *     while (nge_timer_advance(game_on_timer)) {   // 0 once stopped
 *         nge_event_process(game_on_event);
 *     }
 *     ...the player's command uses their turn...
 *     nge_timer_start();
 *
 * The dispatch function is the game's, in common memory (a pointer to a
 * banked function doesn't switch banks); it switches on kind and calls the
 * handlers directly, which may be banked. Handlers may add, change and
 * remove timers (their own too), create and destroy entities (their own
 * too) and post events.
 *
 * The tables and the clock are in slot 0 (the ECS data page, ecs/nge_ecs.h),
 * so saving that page saves every timer. Banking: the functions are banked
 * (NGE_BANKED, page 54): don't call them while game code has its own page
 * mapped in slot 6 or 7. nge_timer_stopped() reads slot 0 directly.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"

#ifndef NGE_TIMER_TICKS_PER_TURN
/** Ticks in a turn, 1 to 255: the turn counter's unit. */
#define NGE_TIMER_TICKS_PER_TURN  10
#endif

/**
 * The game's dispatch function: in common memory; calls the handler for kind.
 * @param cycles Firings left counting this one: 1 for the last, 0 for no limit.
 */
typedef void (*nge_timer_dispatch_fn)(nge_entity_id_t id, uint8_t kind, uint8_t cycles);

/* Tables in slot 0 (nge_timer_cdata.c): read-only for game code; meaningful only with NGE_COMPONENT_TIMER */
extern uint8_t nge_timer_count[NGE_ENTITY_MAX];    /**< Ticks until it fires. */
extern uint8_t nge_timer_period[NGE_ENTITY_MAX];   /**< Ticks between firings, 1 to 255. */
extern uint8_t nge_timer_cycles[NGE_ENTITY_MAX];   /**< Firings left; 0: no limit. */
extern uint8_t nge_timer_kind[NGE_ENTITY_MAX];     /**< The game's handler number. */

/* The clock, in slot 0: read-only for game code */
extern uint16_t nge_timer_turns;     /**< Turns since nge_timer_init() (wraps after 65535). */
extern uint8_t  nge_timer_tick;      /**< Ticks into the current turn: 0 to NGE_TIMER_TICKS_PER_TURN-1. */
extern uint8_t  nge_timer_halted;    /**< 1 while the clock is stopped. */

/** 1 while the clock is stopped (nge_timer_stop()), 0 while it runs. */
#define nge_timer_stopped()  (nge_timer_halted)

/** @brief Set the clock to turn 0, tick 0, running. Call after nge_ecs_init(). */
void nge_timer_init(void) NGE_BANKED;

/**
 * @brief Give an entity in use a timer, or replace the one it has: it
 *        fires every period ticks, the first time one period from now.
 * @param kind The game's handler number, passed to dispatch.
 * @param period Ticks between firings, 1 to 255 (an actor: its speed).
 * @param cycles Firings before the timer is removed, or 0 for no limit.
 */
void nge_timer_add(nge_entity_id_t id, uint8_t kind, uint8_t period, uint8_t cycles) NGE_BANKED;

/** @brief Change a timer's period (1 to 255), from its next firing on. */
void nge_timer_set_period(nge_entity_id_t id, uint8_t period) NGE_BANKED;

/** @brief Remove the timer. */
void nge_timer_remove(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief Stop the clock: nge_timer_advance() does nothing until
 *        nge_timer_start(). Called from a handler, the rest of the current
 *        tick's timers still fire first.
 */
void nge_timer_stop(void) NGE_BANKED;

/** @brief Start the clock again. */
void nge_timer_start(void) NGE_BANKED;

/**
 * @brief Move the clock on to the next tick at which a timer is due, and
 *        fire the timers due then.
 *
 * Fires them in entity ID order, each through dispatch, then starts its
 * next period (with the period as it is after the handler) and counts its
 * cycle. A timer removed, or an entity destroyed, by an earlier handler in
 * the same tick is not fired.
 * @return Ticks the clock moved on (1 or more), or 0 if it didn't: it is
 *         stopped, or no alive entity has a timer.
 */
uint8_t nge_timer_advance(nge_timer_dispatch_fn dispatch) NGE_BANKED;

#endif
