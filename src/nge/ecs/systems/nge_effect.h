#ifndef NGE_EFFECT_H
#define NGE_EFFECT_H

/**
 * @file nge_effect.h
 * @brief Effect system: the lifecycle of active effects (poison,
 *        regeneration, haste), and nothing of what they do.
 *
 * An active effect is an entity owned by its target, with the effect
 * component (its kind and power) and a timer of the reserved kind
 * NGE_TIMER_EFFECT (ecs/systems/nge_timer.h). It goes when its target goes
 * (ecs/nge_owner.h). nge_effect_start() creates one, or refreshes the one
 * of that kind already there (its timer restarted, the higher power kept)
 * unless the kind stacks (NGE_EFFECT_STACK: another instance). It fires
 * every period for its cycles (0: for ever), then ends; it may be ended
 * before, for any reason the game names, with nge_effect_end().
 *
 * What an effect does is the game's. The system holds which effects an
 * entity has, at what power and for how long, and tells the game when
 * something happens to one through one hook, the game's dispatch function
 * (NGE_EFFECT_STARTED, REFRESHED, TICK, ENDED). The hooks are direct
 * calls, so the game can recompute what effects change before its caller
 * goes on. A hook may start and end effects (its own too), and destroy
 * entities (the target too): the system reads only its own data afterwards.
 *
 * A kind is any number the game gives it: the system keeps no table of
 * kinds. Each start passes the kind's period, cycles and flags, from the
 * game's own definition of it.
 *
 * Endings all go one way: the effect is destroyed, then the ENDED hook
 * gets its target, kind and power by value, so a recompute it runs no
 * longer counts it. The system ends an effect itself only when its time
 * runs out (NGE_EFFECT_END_EXPIRED); cures, saving throws and the like are
 * the game's, with the iterator and nge_effect_end().
 *
 *     static void game_effect_hook(const nge_effect_hook_t *h)   // common
 *     {
 *         effects_on_hook(h);    // banked: ops, recompute, events
 *     }
 *
 *     nge_timer_init();
 *     nge_effect_init(game_effect_hook);
 *     nge_effect_start(EF_POISON, 1, target, 30, 4, 0);
 *
 * The component's data is page data in page 59; the timer's tables are in
 * slot 0. Banking: the functions are banked (NGE_BANKED, page 58): don't
 * call them while game code has its own page mapped in slot 6 or 7. The
 * hook is in common memory (a pointer to a banked function doesn't switch
 * banks); it may call banked game code. Firings come through a trampoline
 * in common memory (nge_effect_timer.c), so the game's timer dispatch never
 * sees them.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

/** nge_effect_start()'s flags: started again, another instance; without it the one there is refreshed. */
#define NGE_EFFECT_STACK  0x01

/* What happened: nge_effect_hook_t.what */
#define NGE_EFFECT_STARTED    0   /**< A new instance; arg unused. */
#define NGE_EFFECT_REFRESHED  1   /**< The one there restarted; arg 1 if its power rose. */
#define NGE_EFFECT_TICK       2   /**< A firing; arg: cycles left, counting this one (1: the last, 0: for ever). */
#define NGE_EFFECT_ENDED      3   /**< Destroyed already; arg: the reason. */
#define NGE_EFFECT_HOOKS      4

/** Why an effect ended: the only reason the system sets. The game numbers its own from 1. */
#define NGE_EFFECT_END_EXPIRED  0

/** What the hook is told. */
typedef struct nge_effect_hook {
    uint8_t         what;     /**< NGE_EFFECT_STARTED... */
    nge_entity_id_t effect;   /**< Dying when ENDED: only its ID is passed on. */
    nge_entity_id_t target;   /**< What it affects: its owner. */
    uint8_t         kind;
    int8_t          power;
    uint8_t         arg;      /**< See each what. */
} nge_effect_hook_t;

/** The game's dispatch function: in common memory. */
typedef void (*nge_effect_hook_fn)(const nge_effect_hook_t *h);

/**
 * Loop over target's active effects, last to first: i is a uint8_t loop
 * counter, effect receives each ID. Ending effects in the loop is allowed,
 * as for NGE_OWNER_FOR_EACH_OWNED.
 */
#define NGE_EFFECT_FOR_EACH(i, effect, target) \
    NGE_OWNER_FOR_EACH_OWNED(i, effect, target) \
        if (!NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT)) {} else

/**
 * @brief Set up the system with the game's hook, and register its timer
 *        handler for NGE_TIMER_EFFECT. Call after nge_timer_init(), which
 *        clears the handlers; call again after each nge_timer_init().
 */
void nge_effect_init(nge_effect_hook_fn hook) NGE_BANKED;

/**
 * @brief Start kind at power on target, or refresh the one there.
 *
 * Not stacking, with one of kind there: its timer starts again with period
 * and cycles, it keeps the higher power, and REFRESHED. Otherwise a new
 * active effect owned by target, then STARTED.
 * @param power Not 0.
 * @param target An alive entity.
 * @param period Ticks between firings, 1 to 255.
 * @param cycles Firings, or 0 for ever.
 * @param flags NGE_EFFECT_STACK, or 0.
 * @return The effect (the hook may have ended it), or NGE_ENTITY_INVALID if
 *         no entity is free: nothing was started.
 */
nge_entity_id_t nge_effect_start(uint8_t kind, int8_t power, nge_entity_id_t target,
                                 uint8_t period, uint8_t cycles, uint8_t flags) NGE_BANKED;

/**
 * @brief End an alive active effect for reason: remove its timer, destroy
 *        it, then the ENDED hook with reason.
 * @param reason NGE_EFFECT_END_EXPIRED, or one of the game's.
 */
void nge_effect_end(nge_entity_id_t effect, uint8_t reason) NGE_BANKED;

/** @brief An active effect's kind (one in use: alive or dying). */
uint8_t nge_effect_kind(nge_entity_id_t effect) NGE_BANKED;

/** @brief An active effect's power (one in use: alive or dying). */
int8_t nge_effect_power(nge_entity_id_t effect) NGE_BANKED;

#endif
