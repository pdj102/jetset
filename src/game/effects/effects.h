#ifndef EFFECTS_H
#define EFFECTS_H

/**
 * @file effects.h
 * @brief Effects: what potions, food, poison, rings and the like do to an
 *        entity, and for how long.
 *
 * An effect is a definition (EF_*): constant data in a table
 * (effects_pdata.c) that bundles
 * - modifiers: amounts added to attributes (game/components/attrs.h), or
 *   bits ORed into mask attributes, while the effect lasts;
 * - statuses granted while it lasts (STATUS_POISONED);
 * - ops: what it does on_start (when applied), on_tick (each period) and
 *   on_end (when it ends): damage, heal, cure, raise a base for good,
 *   through the components' rules;
 * - a duration:
 *   instant  its on_start op runs once; nothing is left behind (a healing
 *            potion; raising a base is how an effect is permanent);
 *   timed    an active effect: an entity owned by the target, with
 *            COMPONENT_EFFECT and a timer (TIMER_EFFECT, game/timers.h)
 *            that runs its on_tick op every period for its cycles (0: for
 *            ever), then ends it. Its definition may name a saving throw
 *            the target tries at each firing, after the tick op: passed,
 *            it ends early. It goes when its target goes. Applied again,
 *            it is refreshed (its timer restarted) unless its definition
 *            stacks (another instance);
 *   worn     an item's worn hook (game/gen/gen.h): its modifiers and
 *            statuses count while the item is equipped, with no instance.
 *
 * An attribute's value (game/components/attrs.h: kind value, base,
 * value) is always its base plus the modifiers of the worn effects of
 * equipped items and of the target's active effects. effects_recompute()
 * works the values out from scratch, so a modifier can't be left behind;
 * the code that changes any of these calls it (equipping, an effect
 * starting or ending, a base raised). A pool's current amount (hit
 * points) is kept within its value; a higher maximum doesn't heal.
 *
 * Effects are applied by direct calls, not by events: by actions (using an
 * item, a hit's hook), by generators (a creature's innate hook) and by ops
 * (a cure). Events report what happened (EV_EFFECT_STARTED,
 * EV_EFFECT_ENDED, EV_ATTRS_CHANGED), for messages and the screen.
 *
 * Sources: an active effect records no source entity, so it can't hold an
 * ID that is later reused; its ops name the effect as the cause.
 *
 * Banking: everything here is banked (NGE_BANKED, page 120, with the
 * definitions and the active effects' kinds in page 121). Don't call
 * these while game code has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* Effects: rows of the definitions table; EF_NONE (0) is an empty hook */
#define EF_NONE         0
#define EF_HEALING      1   /**< Instant: heal 6. */
#define EF_POISON       2   /**< Timed: 1 poison damage every 3 turns, 4 times; poisoned. */
#define EF_CURE         3   /**< Instant: ends poisoning. */
#define EF_VITALITY     4   /**< Instant: +1 base maximum hit points, for good. */
#define EF_RING_HEALTH  5   /**< Worn: +2 maximum hit points. */
#define EF_REGEN        6   /**< Timed, for ever, stacking: heal 1 every 10 turns (the player's innate effect). */
#define EF_STRENGTH     7   /**< Instant: +1 base strength, for good. */
#define EF_COUNT        8

/* Why an effect ended: EV_EFFECT_ENDED's p2 */
#define EFFECT_END_EXPIRED  0   /**< Its time ran out, or effect_end(). */
#define EFFECT_END_CURED    1   /**< effect_cure() (or a cure op). */
#define EFFECT_END_SAVED    2   /**< Its target passed its saving throw. */
#define EFFECT_END_COUNT    3

/* effect_text(): which text */
#define EFFECT_TEXT_START  0   /**< When it starts: "You feel sick." */
#define EFFECT_TEXT_END    1   /**< When it ends: "The poison wears off." */
#define EFFECT_TEXT_NAME   2   /**< While it lasts (a timed effect): "Poison". */
#define EFFECT_TEXT_COUNT  3

#define EFFECT_NAME_MAX  14   /**< The longest name (EFFECT_TEXT_NAME). */

/**
 * @brief Apply effect ef (not EF_NONE) to target: an instant effect runs
 *        its on_start op; a timed one starts an active effect (its on_start op,
 *        recomputing the target) or refreshes the one there. Posts
 *        EV_EFFECT_STARTED. A worn effect can't be applied: its
 *        modifiers count through equipment (asserts).
 * @param source What caused it, for its on_start op's events (damage), or
 *               NGE_ENTITY_INVALID.
 * @return 1, or 0 if refused: the target isn't alive, has no health or is
 *         at 0 hp, or no entity is free for an active effect.
 */
uint8_t effect_apply(uint8_t ef, nge_entity_id_t target, nge_entity_id_t source) NGE_BANKED;

/**
 * @brief End an active effect: destroy it, run its on_end op, recompute its
 *        target and post EV_EFFECT_ENDED, p2 EFFECT_END_EXPIRED (unless the
 *        target is dead).
 */
void effect_end(nge_entity_id_t effect) NGE_BANKED;

/**
 * @brief End target's active effects that grant any of statuses
 *        (EV_EFFECT_ENDED, p2 EFFECT_END_CURED).
 * @return How many were ended.
 */
uint8_t effect_cure(nge_entity_id_t target, uint8_t statuses) NGE_BANKED;

/**
 * @brief An active effect's timer handler (TIMER_EFFECT): its on_tick op on
 *        its target, then its saving throw, if it has one (passed, it ends:
 *        EFFECT_END_SAVED); on its last cycle (cycles 1) it ends.
 */
void effects_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED;

/**
 * @brief Work out an entity's derived attributes and statuses from its
 *        bases, its equipped items' worn effects and its active effects,
 *        clamped to their ranges; keep hp within the maximum and an
 *        actor's timer period equal to its speed. Posts EV_ATTRS_CHANGED
 *        if anything changed. Does nothing to an entity that isn't alive
 *        or has no health.
 */
void effects_recompute(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief 1 if an entity's derived attributes and statuses are what
 *        effects_recompute() would make them (for asserts and tests).
 */
uint8_t effects_check(nge_entity_id_t id) NGE_BANKED;

/** @brief Set attribute attr's base for good, then recompute (an alive entity with health and the attribute's component). */
void effects_set_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED;

/** @brief The kind (EF_*) of an active effect. */
uint8_t effect_kind_of(nge_entity_id_t effect) NGE_BANKED;

/**
 * @brief Copy effect ef's start or end text, or its name (EFFECT_TEXT_*),
 *        into buf: at most size-1 characters and a NUL; empty if it has
 *        none.
 */
void effect_text(uint8_t ef, uint8_t which, char *buf, uint8_t size) NGE_BANKED;

/**
 * @brief Effect ef's saving throw: its DC, 0 if it has none, and in
 *        *ability the ability rolled (ABIL_*, game/components/stats.h).
 */
uint8_t effect_save_of(uint8_t ef, uint8_t *ability) NGE_BANKED;

#endif
