#ifndef NGE_EVENT_H
#define NGE_EVENT_H

/**
 * @file nge_event.h
 * @brief Events: a queue of things that have happened, for systems to react to.
 *
 * An event records a fact, after it has happened: "Player damaged by Orc
 * (slashing, 4 hp)", "Player died", "Bread consumed by Player", "Trap stood
 * on by Player". Code that makes a change posts an event about it; systems
 * react to it in their on_event() functions (messages, stats, follow-on
 * changes) but can't cancel or change it. Name events in the past tense.
 *
 * Each event has a target (an entity ID, or NGE_EVENT_NONE), an event ID,
 * and optionally a source (the entity that caused it) and two 8-bit
 * payloads (0 if unused).
 *
 * Events wait in a short queue in common memory and are handled in the
 * order they were posted. The game loop calls nge_event_process() with its
 * dispatch function, which calls each system's on_event() in a fixed order:
 *
 *     static void game_on_event(const nge_event_t *e)   // in common memory
 *     {
 *         messages_on_event(e);
 *         stats_on_event(e);       // may be banked: called directly
 *     }
 *     ...
 *     nge_event_process(game_on_event);
 *
 * Rules:
 * - Post an event after the change it describes, never to ask whether
 *   something may happen.
 * - The dispatch function must be in common memory (a pointer to a banked
 *   function doesn't switch banks); the on_event() functions it calls may
 *   be banked.
 * - Handlers get a const pointer to a copy of the event. They may post new
 *   events, which are handled in the same nge_event_process() call, after
 *   those already waiting ("damaged" leading to "died").
 * - Entity IDs in events stay valid until the events are processed: call
 *   nge_event_process() before releasing dying entities, so a "died" target
 *   is still dying, not freed and reused. Check NGE_ENTITY_IS_ALIVE() in a
 *   handler that needs an alive entity.
 * - A full queue is a programming error: nge_event_post() asserts (with
 *   NDEBUG, the new event is dropped).
 *
 * Every function returns at once (non-blocking), and all are in common
 * memory.
 */

#include <stdint.h>

#ifndef NGE_EVENT_QUEUE
/** Events waiting at most: a power of 2. */
#define NGE_EVENT_QUEUE  16
#endif

#ifndef NGE_EVENT_PROCESS_MAX
/** Events handled by one nge_event_process() call at most: catches handlers that keep posting. */
#define NGE_EVENT_PROCESS_MAX  64
#endif

/** No entity, for a target or source: the same value as NGE_ENTITY_INVALID. */
#define NGE_EVENT_NONE  0xFF

/** Event IDs 0 to NGE_EVENT_USER-1 are reserved for nge; the game's start here. */
#define NGE_EVENT_USER  16

typedef uint8_t nge_event_id_t;

/** An event: 5 bytes. */
typedef struct nge_event {
    uint8_t target;        /**< Entity it happened to, or NGE_EVENT_NONE. */
    nge_event_id_t id;     /**< What happened. */
    uint8_t source;        /**< Entity that caused it, or NGE_EVENT_NONE. */
    uint8_t p1;            /**< Payload 1, e.g. a damage type; 0 if unused. */
    uint8_t p2;            /**< Payload 2, e.g. an amount; 0 if unused. */
} nge_event_t;

/** The game's dispatch function: in common memory; calls each system's on_event(). */
typedef void (*nge_event_dispatch_fn)(const nge_event_t *e);

/** @brief Empty the queue. */
void nge_event_init(void);

/**
 * @brief Post an event: add it to the end of the queue.
 *
 * Asserts if the queue is full (with NDEBUG the event is dropped).
 */
void nge_event_post(uint8_t target, nge_event_id_t id, uint8_t source, uint8_t p1, uint8_t p2);

/** Post an event with only a target: no source, payloads 0. */
#define nge_event_post0(target, id)  nge_event_post((target), (id), NGE_EVENT_NONE, 0, 0)

/** @brief Number of events waiting. */
uint8_t nge_event_count(void);

/**
 * @brief Handle the waiting events, oldest first: each is taken off the
 *        queue and passed, as a const copy, to dispatch.
 *
 * Events posted by the handlers are handled in the same call. At most
 * NGE_EVENT_PROCESS_MAX events are handled: more asserts (with NDEBUG, the
 * rest wait for the next call).
 * @param dispatch The game's dispatch function, in common memory.
 * @return The number of events handled.
 */
uint8_t nge_event_process(nge_event_dispatch_fn dispatch);

/** @brief Throw away the waiting events (for example on a level change). */
void nge_event_flush(void);

#endif
