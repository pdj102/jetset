#ifndef NGE_TIMER_H
#define NGE_TIMER_H

/**
 * @file nge_timer.h
 * @brief Timer system: one game clock for everything that acts or happens
 *        on a schedule (actors, effects).
 *
 * Time is counted in ticks, NGE_TIMER_TICKS_PER_TURN to a turn. Ticks are
 * simulated, not frames: nge_timer_advance() jumps straight to the next
 * tick at which a timer is due and fires it, and the clock stops while the
 * game waits for the player.
 *
 * Each entity with the timer component fires after `count` ticks: the game's
 * dispatch function is called with the entity, its timer's `kind` (which
 * handler) and its `cycles` left, and the handler's return value says what
 * happens next:
 *
 *     1 to NGE_TIMER_DELAY_MAX   fire again after that many ticks (an
 *                                actor: the cost of what it did)
 *     NGE_TIMER_REPEAT           fire again after `period` ticks (an effect)
 *     NGE_TIMER_WAIT             stop the clock until nge_timer_set_delay()
 *                                for this entity (the player: wait for a key)
 *     NGE_TIMER_STOP             remove the timer
 *
 * `cycles` limits how often a timer fires: after each firing it goes down
 * by one, and the timer is removed when it reaches 0. A timer with cycles 0
 * fires until it is removed. The handler gets the cycles left counting this
 * firing: 1 is the last (time to say "the poison wears off"), 0 means no limit.
 *
 * Effects (poison, regeneration, sleep) are entities with a timer owned by
 * their target (ecs/nge_owner.h): destroying the target destroys or moves
 * what it owns, so its effects go with it.
 *
 * The dispatch function is the game's, in common memory (a pointer to a
 * banked function doesn't switch banks); it switches on kind and calls the
 * handlers directly, which may be banked:
 *
 *     static uint8_t game_on_timer(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
 *     {
 *         switch (kind) {
 *         case TIMER_PLAYER:
 *             return NGE_TIMER_WAIT;
 *         case TIMER_MONSTER:
 *             return monster_act(id);      // returns its action's cost
 *         ...
 *         }
 *     }
 *
 *     while (nge_timer_waiting() == NGE_ENTITY_INVALID && nge_timer_advance(game_on_timer)) {
 *         nge_event_process(game_on_event);
 *     }
 *     ...the player's command...
 *     nge_timer_set_delay(player, cost);   // the clock runs again
 *
 * Handlers may add, change and remove other entities' timers, create and
 * destroy entities (their own too) and post events. They change their own
 * timer only through the return value (or remove it, or destroy their
 * entity, after which the return value is ignored).
 *
 * The tables and the clock are in slot 0 (the ECS data page, ecs/nge_ecs.h),
 * so saving that page saves every timer. Banking: the functions are banked
 * (NGE_BANKED, page 54): don't call them while game code has its own page
 * mapped in slot 6 or 7. nge_timer_waiting() reads slot 0 directly.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

#ifndef NGE_TIMER_TICKS_PER_TURN
/** Ticks in a turn, 1 to 255: the turn counter's unit. */
#define NGE_TIMER_TICKS_PER_TURN  10
#endif

/** Component bit: component number 3, reserved for nge (see nge_entity.h). */
#define NGE_COMPONENT_TIMER  NGE_COMPONENT(3)

/* The handler's return value (and the delay and period arguments: 1 to NGE_TIMER_DELAY_MAX) */
#define NGE_TIMER_STOP       0      /**< Remove the timer. */
#define NGE_TIMER_DELAY_MAX  252    /**< Longest delay or period, in ticks. */
#define NGE_TIMER_WAIT       0xFE   /**< Stop the clock until nge_timer_set_delay() for this entity. */
#define NGE_TIMER_REPEAT     0xFF   /**< Fire again after the timer's period. */

/**
 * The game's dispatch function: in common memory; calls the handler for
 * kind and returns its result.
 * @param cycles Firings left counting this one: 1 for the last, 0 for no limit.
 */
typedef uint8_t (*nge_timer_dispatch_fn)(nge_entity_id_t id, uint8_t kind, uint8_t cycles);

/* Tables in slot 0 (nge_timer_cdata.c): read-only for game code; meaningful only with NGE_COMPONENT_TIMER */
extern uint8_t nge_timer_count[NGE_ENTITY_MAX];    /**< Ticks until it fires (0 only while waiting). */
extern uint8_t nge_timer_period[NGE_ENTITY_MAX];   /**< Ticks between firings for NGE_TIMER_REPEAT; 0: none. */
extern uint8_t nge_timer_cycles[NGE_ENTITY_MAX];   /**< Firings left; 0: no limit. */
extern uint8_t nge_timer_kind[NGE_ENTITY_MAX];     /**< The game's handler number. */

/* The clock, in slot 0: read-only for game code */
extern uint16_t        nge_timer_turns;   /**< Turns since nge_timer_init() (wraps after 65535). */
extern uint8_t         nge_timer_tick;    /**< Ticks into the current turn: 0 to NGE_TIMER_TICKS_PER_TURN-1. */
extern nge_entity_id_t nge_timer_wait;    /**< The entity the clock waits for, or NGE_ENTITY_INVALID. */

/** The entity the clock is waiting for (it returned NGE_TIMER_WAIT), or NGE_ENTITY_INVALID if it runs. */
#define nge_timer_waiting()  (nge_timer_wait)

/** @brief Set the clock to turn 0, tick 0, running. Call after nge_ecs_init(). */
void nge_timer_init(void) NGE_BANKED;

/**
 * @brief Give an entity in use a timer, or replace the one it has.
 * @param kind The game's handler number, passed to dispatch.
 * @param delay Ticks until it first fires: 1 to NGE_TIMER_DELAY_MAX.
 * @param period Ticks between firings for NGE_TIMER_REPEAT (1 to
 *               NGE_TIMER_DELAY_MAX), or 0 if the handler never returns it.
 * @param cycles Firings before the timer is removed, or 0 for no limit.
 */
void nge_timer_add(nge_entity_id_t id, uint8_t kind, uint8_t delay, uint8_t period,
                   uint8_t cycles) NGE_BANKED;

/**
 * @brief Fire an entity's timer after delay ticks (1 to NGE_TIMER_DELAY_MAX)
 *        from now instead: after the player's command, or to wake a
 *        sleeper early. If the clock is waiting for this entity, it runs again.
 */
void nge_timer_set_delay(nge_entity_id_t id, uint8_t delay) NGE_BANKED;

/** @brief Remove the timer (if the clock waits for this entity, it runs again). */
void nge_timer_remove(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief Move the clock on to the next tick at which a timer is due, and
 *        fire the timers due then.
 *
 * Fires them in entity ID order, each through dispatch (see above). A timer
 * removed, or an entity destroyed, by an earlier handler in the same tick is
 * not fired. If a handler returns NGE_TIMER_WAIT, the rest of the tick's
 * timers still fire, then the clock stops.
 * @return Ticks the clock moved on (1 or more), or 0 if it didn't: it is
 *         waiting, or no alive entity has a timer.
 */
uint8_t nge_timer_advance(nge_timer_dispatch_fn dispatch) NGE_BANKED;

#endif
