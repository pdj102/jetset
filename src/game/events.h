#ifndef EVENTS_H
#define EVENTS_H

/**
 * @file events.h
 * @brief The game's event IDs (nge/event/nge_event.h): things that have
 *        happened, in the past tense. An action that is refused posts
 *        nothing: its reason code says why (game/actions/act.h).
 *
 * | Event         | Target             | Source  | Payloads |
 * |---------------|--------------------|---------|----------|
 * | EV_MOVED      | the entity moved   | -       | -        |
 * | EV_STOOD_ON   | what is stood on   | stander | -        |
 * | EV_GLOWED     | the effect's owner | effect  | -        |
 * | EV_GLOW_FADED | the effect's owner | effect  | -        |
 * | EV_PICKED_UP  | the item           | taker   | -        |
 * | EV_DROPPED    | the item           | dropper | -        |
 */

#include "event/nge_event.h"

#define EV_MOVED       (NGE_EVENT_USER + 0)   /**< Moved one tile (act_move()). */
#define EV_STOOD_ON    (NGE_EVENT_USER + 1)   /**< Stood on by the source (after a move onto its tile). */
#define EV_GLOWED      (NGE_EVENT_USER + 2)   /**< The demo effect fired (game/timers.h). */
#define EV_GLOW_FADED  (NGE_EVENT_USER + 3)   /**< The demo effect's last firing: it is gone. */
#define EV_PICKED_UP   (NGE_EVENT_USER + 4)   /**< Picked up and stowed by the source (act_pickup()). */
#define EV_DROPPED     (NGE_EVENT_USER + 5)   /**< Dropped on the floor by the source (act_drop()). */

#endif
