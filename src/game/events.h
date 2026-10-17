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
 * | EV_EQUIPPED   | the item           | wearer  | p1: slot |
 * | EV_UNEQUIPPED | the item           | wearer  | -        |
 * | EV_DAMAGED    | the entity hurt    | causer  | p1: taken, p2: EV_DMG_PACK(outcome, type) |
 * | EV_HEALED     | the entity healed  | -       | p1: restored |
 * | EV_DIED       | the entity killed  | killer  | -        |
 */

#include "event/nge_event.h"

#define EV_MOVED       (NGE_EVENT_USER + 0)   /**< Moved one tile (act_move()). */
#define EV_STOOD_ON    (NGE_EVENT_USER + 1)   /**< Stood on by the source (after a move onto its tile). */
#define EV_GLOWED      (NGE_EVENT_USER + 2)   /**< The demo effect fired (game/timers.h). */
#define EV_GLOW_FADED  (NGE_EVENT_USER + 3)   /**< The demo effect's last firing: it is gone. */
#define EV_PICKED_UP   (NGE_EVENT_USER + 4)   /**< Picked up and stowed by the source (act_pickup()). */
#define EV_DROPPED     (NGE_EVENT_USER + 5)   /**< Dropped on the floor by the source (act_drop()). */
#define EV_EQUIPPED    (NGE_EVENT_USER + 6)   /**< Equipped by the source in slot p1 (act_equip()). */
#define EV_UNEQUIPPED  (NGE_EVENT_USER + 7)   /**< Taken off by the source, back into its inventory (act_unequip()). */
#define EV_DAMAGED     (NGE_EVENT_USER + 8)   /**< Took p1 damage, maybe 0 (game/components/health.h). */
#define EV_HEALED      (NGE_EVENT_USER + 9)   /**< Got p1 hit points back. */
#define EV_DIED        (NGE_EVENT_USER + 10)  /**< Hit points reached 0: destroyed unless persistent. */

#endif
