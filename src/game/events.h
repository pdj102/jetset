#ifndef EVENTS_H
#define EVENTS_H

/**
 * @file events.h
 * @brief The game's event IDs (nge/event/nge_event.h): things that have
 *        happened, in the past tense. An action that is refused posts
 *        nothing: its reason code says why (game/actions/act.h).
 *
 * | Event             | Target              | Source   | Payloads |
 * |-------------------|---------------------|----------|----------|
 * | EV_MOVED          | the entity moved    | -        | p1: dx, p2: dy (int8_t, -1 to 1) |
 * | EV_STOOD_ON       | what is stood on    | stander  | -        |
 * | EV_USED           | the item (dying)    | user     | -        |
 * | EV_EFFECT_STARTED | the entity affected | the effect; NGE_EVENT_NONE if instant | p1: EF_* |
 * | EV_PICKED_UP      | the item            | taker    | -        |
 * | EV_DROPPED        | the item            | dropper  | -        |
 * | EV_EQUIPPED       | the item            | wearer   | p1: slot |
 * | EV_UNEQUIPPED     | the item            | wearer   | -        |
 * | EV_DAMAGED        | the entity hurt     | causer   | p1: taken, p2: EV_DMG_PACK(outcome, type) |
 * | EV_HEALED         | the entity healed   | -        | p1: restored |
 * | EV_DIED           | the entity killed   | killer   | -        |
 * | EV_HIT            | the entity hit      | attacker | p1: d20 roll, p2: weapon or NGE_EVENT_NONE |
 * | EV_MISSED         | the entity missed   | attacker | p1: d20 roll, p2: weapon or NGE_EVENT_NONE |
 * | EV_EFFECT_ENDED   | the entity affected | the effect (dying) | p1: EF_*, p2: EFFECT_END_* |
 * | EV_ATTRS_CHANGED  | the entity          | -        | -        |
 * | EV_OPENED         | the feature         | opener   | -        |
 * | EV_CLOSED         | the feature         | closer   | -        |
 * | EV_ARRIVED        | the player          | -        | p1: depth, p2: LEVEL_ARRIVE_* |
 * | EV_LEFT_LEVEL     | the entity (maybe dying) | -   | p1: the depth it went to |
 * | EV_MANA_SPENT     | the entity          | -        | p1: MP spent |
 * | EV_MANA_RESTORED  | the entity          | -        | p1: MP restored |
 * | EV_XP_GAINED      | the player          | -        | p1, p2: XP gained, low and high byte |
 * | EV_LEVEL_GAINED   | the player          | -        | p1: the new level |
 * | EV_GOLD_CHANGED   | the player          | -        | -        |
 */

#include "event/nge_event.h"

#define EV_MOVED          (NGE_EVENT_USER + 0)     /**< Moved one tile by (p1, p2), as int8_t (act_move()). */
#define EV_STOOD_ON       (NGE_EVENT_USER + 1)     /**< Stood on by the source (after a move onto its tile). */
#define EV_USED           (NGE_EVENT_USER + 2)     /**< Used (drunk, eaten) by the source, and used up (act_use()). */
#define EV_EFFECT_STARTED (NGE_EVENT_USER + 3)     /**< Effect p1 started on the target, or was refreshed (game/effects/effects.h). */
#define EV_PICKED_UP      (NGE_EVENT_USER + 4)     /**< Picked up and stowed by the source (act_pickup()). */
#define EV_DROPPED        (NGE_EVENT_USER + 5)     /**< Dropped on the floor by the source (act_drop()). */
#define EV_EQUIPPED       (NGE_EVENT_USER + 6)     /**< Equipped by the source in slot p1 (act_equip()). */
#define EV_UNEQUIPPED     (NGE_EVENT_USER + 7)     /**< Taken off by the source, back into its inventory (act_unequip()). */
#define EV_DAMAGED        (NGE_EVENT_USER + 8)     /**< Took p1 damage, maybe 0 (game/components/health.h). */
#define EV_HEALED         (NGE_EVENT_USER + 9)     /**< Got p1 hit points back. */
#define EV_DIED           (NGE_EVENT_USER + 10)    /**< Hit points reached 0: destroyed unless persistent. */
#define EV_HIT            (NGE_EVENT_USER + 11)    /**< Hit by the source's attack (act_melee()); EV_DAMAGED follows. */
#define EV_MISSED         (NGE_EVENT_USER + 12)    /**< Missed by the source's attack. */
#define EV_EFFECT_ENDED   (NGE_EVENT_USER + 13)    /**< Effect p1 ended on the target: expired, cured or saved (p2). */
#define EV_ATTRS_CHANGED  (NGE_EVENT_USER + 14)    /**< Derived attributes or statuses changed (effects_recompute()). */
#define EV_OPENED         (NGE_EVENT_USER + 15)    /**< Opened by the source (act_open()). */
#define EV_CLOSED         (NGE_EVENT_USER + 16)    /**< Closed by the source (act_close()). */
#define EV_ARRIVED        (NGE_EVENT_USER + 17)    /**< Arrived on a new level, at depth p1, as p2 says (game/level/level.h). */
#define EV_LEFT_LEVEL     (NGE_EVENT_USER + 18)    /**< Left the current level for depth p1 (level_send()). */
#define EV_MANA_SPENT     (NGE_EVENT_USER + 19)    /**< Spent p1 MP (game/components/mana.h). */
#define EV_MANA_RESTORED  (NGE_EVENT_USER + 20)    /**< Got p1 MP back. */
#define EV_XP_GAINED      (NGE_EVENT_USER + 21)    /**< Gained p1 + 256 * p2 XP (game/components/pc.h). */
#define EV_LEVEL_GAINED   (NGE_EVENT_USER + 22)    /**< Went up to level p1. */
#define EV_GOLD_CHANGED   (NGE_EVENT_USER + 23)    /**< Gold gained or spent: pc_gold is the new amount. */

#endif
