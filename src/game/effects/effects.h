#ifndef EFFECTS_H
#define EFFECTS_H

/**
 * @file effects.h
 * @brief Effects: what potions, food, poison, rings and the like do to an
 *        entity, and for how long.
 *
 * An effect is a definition (EF_*): constant data in a table
 * (effects_pdata.c) that bundles
 * - modifiers: amounts added to attributes (game/attrs/attrs.h), or
 *   bits ORed into mask attributes, while the effect lasts;
 * - statuses granted while it lasts (STATUS_POISONED);
 * - ops: what it does on_start (when applied), on_tick (each period) and
 *   on_end (when it ends): damage, heal, cure, raise a base for good,
 *   through the components' rules;
 * - a power: what a use of the effect multiplies the amounts of its ops and
 *   number modifiers by, from the hook that names it (game/gen/gen.h), so
 *   one definition serves every strength of an item. Masks, statuses,
 *   durations and saving throws don't scale. It may be negative;
 * - a duration:
 *   instant  its on_start op runs once; nothing is left behind (a healing
 *            potion; raising a base is how an effect is permanent);
 *   timed    an active effect (ecs/systems/nge_effect.h): an entity owned
 *            by the target, with NGE_COMPONENT_EFFECT and a TIMER_EFFECT
 *            timer (game/timers.h), which runs its on_tick op every
 *            period for its cycles (0: for ever), then ends it, its kind
 *            and power in the timer's payload. It keeps its power, as what
 *            started it may be gone. Its definition may name a saving throw
 *            the target tries at each firing, after the tick op: passed,
 *            it ends early. It goes when its target goes. Applied again,
 *            it is refreshed (its timer restarted, the stronger power
 *            kept) unless its definition stacks (another instance);
 *   worn     an item's worn hook (game/gen/gen.h): its modifiers and
 *            statuses count while the item is equipped, with no instance,
 *            at the power the item's hook names.
 *
 * An attribute's value (game/attrs/attrs.h: kind value, base, value) is
 * always its base plus the modifiers of the worn effects of equipped items
 * and of the target's active effects. effects_recompute() (the attributes
 * module) works the values out from scratch, so a modifier can't be left
 * behind; the code that changes any of these calls it (equipping, an
 * effect starting or ending, a base raised). A pool's current amount (hit
 * points) is kept within its value; a higher maximum doesn't heal.
 *
 * Lifecycle: nge creates an active effect, with its kind and power in its
 * timer's payload, destroys it, and finds a target's (NGE_EFFECT_FOR_EACH);
 * everything else is here: refreshing and stacking, firings
 * (effects_on_timer(), from the game's dispatch, which skips effects off
 * the level), expiry, saving throws, cures, recomputing, events. An active
 * effect's kind and power: EFFECT_KIND_OF(), EFFECT_POWER_OF().
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
 * definitions in page 121). Don't call these while game code has its own
 * page mapped in slot 6 or 7. EFFECT_KIND_OF() and EFFECT_POWER_OF() read
 * slot 0 directly.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "ecs/systems/nge_effect.h"
#include "ecs/systems/nge_timer.h"
#include "game/components/components.h"

/*
 * Effects: rows of the definitions table; EF_NONE (0) is an empty hook. Amounts are
 * per power. Timed effects come first: only they can be active, and what counts
 * active effects by kind needs EF_TIMED_COUNT entries, not EF_COUNT. "Pending" rows
 * do nothing until their mechanic is built (docs/effects.md, "Pending effects")
 */
#define EF_NONE               0
#define EF_POISON             1    /**< Timed: 1 poison damage per power every 3 turns, 4 times; poisoned. */
#define EF_REGEN              2    /**< Timed, for ever, stacking: heal 1 per power every 10 turns (the player's innate effect). */
#define EF_RENEW              3    /**< Timed: heal 1 per power each turn, for 10 turns. */
#define EF_HASTE              4    /**< Timed: speed -1 per power for 20 turns (lower is faster). */
#define EF_SLOW               5    /**< Timed: speed +1 per power for 20 turns. */
#define EF_MIGHT              6    /**< Timed: Str +1 per power for 20 turns. */
#define EF_WEAKNESS           7    /**< Timed: Str -1 per power for 20 turns. */
#define EF_STONE_SKIN         8    /**< Timed: AC +1 and speed +1 per power for 20 turns. */
#define EF_RESIST_FIRE        9    /**< Timed: resist fire for 30 turns. */
#define EF_RESIST_COLD        10   /**< Timed: resist cold for 30 turns. */
#define EF_BLIND              11   /**< Timed: blinded, and -4 to hit (ATTR_TO_HIT), for up to 10 turns. */
#define EF_SLEEP              12   /**< Timed: unconscious for 8 turns; pending: no rule reads it. */
#define EF_ROOT               13   /**< Timed, for ever, innate: rooted. */
#define EF_HOLD               14   /**< Timed: rooted for 6 turns. */
#define EF_MANA_REGEN         15   /**< Timed, for ever, innate: 1 MP every 20 turns (every kind with mana). */
#define EF_DESCENT            16   /**< Timed: one firing after 15 turns, which teleports a level down per power (on_end). */
#define EF_RECALL             17   /**< Timed: one firing after 15 turns, which teleports a level up per power (on_end); power 10 reaches depth 1. */
#define EF_GRACE              18   /**< Timed: Dex +1 per power for 20 turns. */
#define EF_ENDURANCE          19   /**< Timed: Con +1 per power for 20 turns. */
#define EF_INSIGHT            20   /**< Timed: Int +1 per power for 20 turns. */
#define EF_INTUITION          21   /**< Timed: Wis +1 per power for 20 turns. */
#define EF_RESIST_POISON      22   /**< Timed: resist poison for 30 turns. */
#define EF_RESIST_ACID        23   /**< Timed: resist acid for 30 turns. */
#define EF_RESIST_MAGIC       24   /**< Timed: resist magic for 30 turns. */
#define EF_AID                25   /**< Timed: HP max +2 per power for 30 turns. */
#define EF_SHIELD             26   /**< Timed: AC +1 per power for 20 turns. */
#define EF_HEX                27   /**< Timed: vulnerable to magic for 20 turns; Wis save. */
#define EF_DRAIN_STR          28   /**< Timed: Str -1 per power for 100 turns; Con save. */
#define EF_DRAIN_DEX          29   /**< Timed: Dex -1 per power for 100 turns; Con save. */
#define EF_DRAIN_CON          30   /**< Timed: Con -1 per power for 100 turns; Con save. */
#define EF_DRAIN_INT          31   /**< Timed: Int -1 per power for 100 turns; Con save. */
#define EF_DRAIN_WIS          32   /**< Timed: Wis -1 per power for 100 turns; Con save. */
#define EF_DRAIN_CHA          33   /**< Timed: Cha -1 per power for 100 turns; Con save. */
#define EF_BURNING            34   /**< Timed: 1 fire damage per power each turn, 4 times; Dex save. */
#define EF_BLEEDING           35   /**< Timed: 1 slash damage per power every 2 turns, 5 times; Con save. */
#define EF_CORROSION          36   /**< Timed: 1 acid damage per power every 2 turns, 4 times. */
#define EF_BLESS              37   /**< Timed: +1 to hit per power for 30 turns. */
#define EF_BANE               38   /**< Timed: -1 to hit per power for up to 20 turns; Cha save. */
#define EF_HEROISM            39   /**< Timed: +1 to hit and HP max +2 per power for 30 turns. */
#define EF_TIMED_COUNT        40   /**< Timed effects are 1 to this - 1. */
#define EF_HEALING            40   /**< Instant: heal 1 per power. */
#define EF_CURE               41   /**< Instant: ends poisoning. */
#define EF_CURE_ALL           42   /**< Instant: ends every condition. */
#define EF_CURE_BLIND         43   /**< Instant: ends blindness. */
#define EF_MANA               44   /**< Instant: restore 1 MP per power. */
#define EF_VITALITY           45   /**< Instant: +1 per power base maximum hit points, for good. */
#define EF_RAISE_STR          46   /**< Instant: +1 per power base strength, for good. */
#define EF_RAISE_DEX          47   /**< Instant: +1 per power base dexterity, for good. */
#define EF_RAISE_CON          48   /**< Instant: +1 per power base constitution, for good. */
#define EF_RAISE_INT          49   /**< Instant: +1 per power base intelligence, for good. */
#define EF_RAISE_WIS          50   /**< Instant: +1 per power base wisdom, for good. */
#define EF_RAISE_CHA          51   /**< Instant: +1 per power base charisma, for good. */
#define EF_NOURISH            52   /**< Instant: food heals 1 per power, until there is hunger (docs/items.md D11). */
#define EF_BOLT_MAGIC         53   /**< Instant: 1 magic damage per power. */
#define EF_BOLT_FIRE          54   /**< Instant: 1 fire damage per power. */
#define EF_BOLT_COLD          55   /**< Instant: 1 cold damage per power. */
#define EF_BOLT_ACID          56   /**< Instant: 1 acid damage per power. */
#define EF_LIGHT              57   /**< Instant, pending. */
#define EF_IDENTIFY           58   /**< Instant, pending. */
#define EF_MAP                59   /**< Instant, pending. */
#define EF_TELEPORT           60   /**< Instant: to random free floor on the level. */
#define EF_UNCURSE            61   /**< Instant: lifts the curse of everything the target has equipped. */
#define EF_ENCHANT            62   /**< Instant, pending. */
#define EF_SUMMON             63   /**< Instant: 1 random creature for the depth per power (at most 3) around the target. */
#define EF_LEVEL_TELEPORT     64   /**< Instant: teleport a level up or down, at random, per power. */
#define EF_WORN_HP            65   /**< Worn: +1 per power maximum hit points. */
#define EF_WORN_AC            66   /**< Worn: AC +1 per power. */
#define EF_WORN_SPEED         67   /**< Worn: speed -1 per power (lower is faster). */
#define EF_WORN_STR           68   /**< Worn: Str +1 per power. */
#define EF_WORN_DEX           69   /**< Worn: Dex +1 per power. */
#define EF_WORN_CON           70   /**< Worn: Con +1 per power. */
#define EF_WORN_INT           71   /**< Worn: Int +1 per power. */
#define EF_WORN_WIS           72   /**< Worn: Wis +1 per power. */
#define EF_WORN_CHA           73   /**< Worn: Cha +1 per power. */
#define EF_WORN_RES_FIRE      74   /**< Worn: resist fire. */
#define EF_WORN_RES_COLD      75   /**< Worn: resist cold. */
#define EF_WORN_RES_POISON    76   /**< Worn: resist poison. */
#define EF_WORN_RES_ACID      77   /**< Worn: resist acid. */
#define EF_WORN_RES_MAGIC     78   /**< Worn: resist magic. */
#define EF_WORN_IMMUNE_FIRE   79   /**< Worn: immune to fire. */
#define EF_WORN_IMMUNE_COLD   80   /**< Worn: immune to cold. */
#define EF_WORN_IMMUNE_POISON 81   /**< Worn: immune to poison. */
#define EF_WORN_REGEN         82   /**< Worn, pending: a worn effect has no tick. */
#define EF_WORN_LIGHT         83   /**< Worn, pending: no light radius yet; power is the radius. */
#define EF_COUNT              84

/* Why an effect ended: EV_EFFECT_ENDED's p2 */
#define EFFECT_END_EXPIRED  0   /**< Its time ran out, or effect_end(). */
#define EFFECT_END_CURED    1   /**< effect_cure() (or a cure op). */
#define EFFECT_END_SAVED    2   /**< Its target passed its saving throw. */
#define EFFECT_END_COUNT    3

/* An active effect's kind (EF_*, timed) and power: its timer's payload (slot 0), while it is in use */
#define EFFECT_KIND_OF(e)   (nge_timer_payload[0][(e)])
#define EFFECT_POWER_OF(e)  ((int8_t)nge_timer_payload[1][(e)])

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
 *        EV_EFFECT_STARTED, except for an instant effect whose op
 *        fizzled (EV_FIZZLED instead). A worn effect can't be applied: its
 *        modifiers count through equipment (asserts).
 * @param power  Multiplies its amounts: not 0.
 * @param source What caused it, for its on_start op's events (damage), or
 *               NGE_ENTITY_INVALID.
 * @return 1, or 0 if refused: the target isn't alive, has no health or is
 *         at 0 hp, or no entity is free for an active effect.
 */
uint8_t effect_apply(uint8_t ef, int8_t power, nge_entity_id_t target, nge_entity_id_t source) NGE_BANKED;

/**
 * @brief End an active effect: destroy it, run its on_end op, recompute its
 *        target and post EV_EFFECT_ENDED, p2 EFFECT_END_EXPIRED (unless the
 *        target is dead).
 */
void effect_end(nge_entity_id_t effect) NGE_BANKED;

/**
 * @brief An active effect's timer fired (TIMER_EFFECT, from the game's
 *        dispatch): its on_tick op, then its saving throw (passed, it ends:
 *        EFFECT_END_SAVED), then, on its last cycle (cycles 1), its end
 *        (EFFECT_END_EXPIRED). An ending: the on_end op, recompute,
 *        EV_EFFECT_ENDED with the reason, unless the target is dead.
 * @param cycles Firings left counting this one, as the dispatch got it.
 */
void effects_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED;

/**
 * @brief End target's active effects that grant any of statuses
 *        (EV_EFFECT_ENDED, p2 EFFECT_END_CURED), but not innate ones.
 * @return How many were ended.
 */
uint8_t effect_cure(nge_entity_id_t target, uint8_t statuses) NGE_BANKED;

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
