#ifndef EVENTS_H
#define EVENTS_H

/**
 * @file events.h
 * @brief The game's event IDs (nge/event/nge_event.h): things that have
 *        happened, in the past tense.
 *
 * | Event       | Target            | Source  | Payloads |
 * |-------------|-------------------|---------|----------|
 * | EV_MOVED    | the entity moved  | -       | -        |
 * | EV_BUMPED   | the entity        | -       | -        |
 * | EV_STOOD_ON | what is stood on  | stander | -        |
 * | EV_BLOCKED  | the blocker       | mover   | -        |
 */

#include "event/nge_event.h"

#define EV_MOVED     (NGE_EVENT_USER + 0)   /**< Moved one tile. */
#define EV_BUMPED    (NGE_EVENT_USER + 1)   /**< Walked into a wall and didn't move. */
#define EV_STOOD_ON  (NGE_EVENT_USER + 2)   /**< Stood on by the source (after a move onto its tile). */
#define EV_BLOCKED   (NGE_EVENT_USER + 3)   /**< Stopped the source moving onto its tile. */

#endif
