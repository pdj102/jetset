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
 * | EV_FIRED          | the entity aimed at | shooter  | p1: launcher, thrown item or NGE_EVENT_NONE |
 * | EV_AIMED          | the entity aimed at | aimer    | p1: the item |
 * | EV_UNLOCKED       | the feature         | unlocker | -        |
 * | EV_CAST           | the entity aimed at, or the caster | the caster | p1: SP_* |
 * | EV_SPELL_LEARNED  | the entity          | -        | p1: SP_* |
 * | EV_OPERATED       | the feature         | operator | p1: EF_* |
 * | EV_TRIGGERED      | the trap            | the creature it fires at | - |
 * | EV_FOUND          | the hidden feature  | finder, or NGE_EVENT_NONE if it went off | - |
 * | EV_DISARMED       | the trap (dying)    | disarmer | -        |
 * | EV_SEARCHED       | the searcher        | -        | p1: how many found |
 * | EV_FIZZLED        | the entity affected | -        | p1: the op that did nothing (OP_*, game/effects/effects_defs.h) |
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
 * | EV_QUEST_FOUND    | the quest item      | the player | -      |
 * | EV_AI_CHANGED     | the AI entity       | the cause, or NGE_EVENT_NONE | p1: old state, p2: new state (AI_STATE_*) |
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
#define EV_HIT            (NGE_EVENT_USER + 11)    /**< Hit by the source's attack (act_strike()); EV_DAMAGED follows. */
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
#define EV_QUEST_FOUND    (NGE_EVENT_USER + 24)    /**< Picked up by the player for the first time this game (game/quest/quest.h). */
#define EV_AI_CHANGED     (NGE_EVENT_USER + 25)    /**< Its AI state went from p1 to p2 (game/ai/ai.h). */
#define EV_FIRED          (NGE_EVENT_USER + 26)    /**< A shot was fired at it by the source (act_fire()); EV_HIT or EV_MISSED follows. */
#define EV_AIMED          (NGE_EVENT_USER + 27)    /**< An item's effect was sent at it by the source (act_aim()); the effect's events follow. */
#define EV_UNLOCKED       (NGE_EVENT_USER + 28)    /**< Unlocked by the source (act_unlock()). */
#define EV_CAST           (NGE_EVENT_USER + 29)    /**< Spell p1 was cast at it by the source, or by itself (act_cast()); the effect's events follow. */
#define EV_SPELL_LEARNED  (NGE_EVENT_USER + 30)    /**< Learned spell p1 (spell_learn(), game/spells/spells.h). */
#define EV_OPERATED       (NGE_EVENT_USER + 31)    /**< Operated by the source (act_operate()): its effect p1 follows. */
#define EV_TRIGGERED      (NGE_EVENT_USER + 32)    /**< A trap went off at the source (act_spring()): its attack's or effect's events follow. */
#define EV_FOUND          (NGE_EVENT_USER + 33)    /**< A hidden feature was found by the source, or went off (NGE_EVENT_NONE). */
#define EV_DISARMED       (NGE_EVENT_USER + 34)    /**< Disarmed by the source (act_disarm()): destroyed. */
#define EV_SEARCHED       (NGE_EVENT_USER + 35)    /**< Searched around itself, finding p1 hidden features (act_search()). */
#define EV_FIZZLED        (NGE_EVENT_USER + 36)    /**< An effect's op p1 could do nothing to the target (game/effects). */

#endif
