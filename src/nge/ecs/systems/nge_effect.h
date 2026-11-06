#ifndef NGE_EFFECT_H
#define NGE_EFFECT_H

/**
 * @file nge_effect.h
 * @brief Effect system: setting up active effects (poison, regeneration,
 *        haste) and finding them again, and nothing of what they do.
 *
 * An active effect is an alive entity owned by its target, with the effect
 * component (a tag: no data) and a timer (ecs/systems/nge_timer.h) whose
 * payload holds what the game needs to know what it is (its kind and
 * power, say). It goes when its target goes (ecs/nge_owner.h).
 *
 * nge_effect_create() makes one; nge_effect_destroy() destroys it;
 * NGE_EFFECT_FOR_EACH walks a target's. The rest is the game's: its timer
 * is one of the game's kinds, fired through the game's dispatch, and the
 * game decides when an effect starts, is refreshed (nge_timer_add() again:
 * the payload is kept) or stacks, what its firings do, and when it ends
 * (on its last cycle, its handler's cycles is 1). nge_effect calls nothing
 * of the game's.
 *
 *     e = nge_effect_create(target, TIMER_EFFECT, 30, 4, EF_POISON, 1);
 *     ...
 *     NGE_EFFECT_FOR_EACH(i, e, target) {
 *         if (nge_timer_payload[0][e] == EF_POISON) {
 *             nge_effect_destroy(e);   // cured
 *         }
 *     }
 *
 * Banking: the functions are banked (NGE_BANKED, page 58), with no data of
 * their own: they read and write slot 0 only. Don't call them while game
 * code has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

/**
 * Loop over target's alive active effects, last to first: i is a uint8_t
 * loop counter, effect receives each ID. Destroying effects in the loop is
 * allowed, as for NGE_OWNER_FOR_EACH_OWNED; a destroyed one is dying, so it
 * isn't visited.
 */
#define NGE_EFFECT_FOR_EACH(i, effect, target) \
    NGE_OWNER_FOR_EACH_OWNED(i, effect, target) \
        if (!NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT)) {} else

/**
 * @brief Create an active effect: an entity owned by target, with the
 *        effect component, payload p0 and p1, and a timer of kind, period
 *        and cycles (nge_timer_add(): its first firing one period from now).
 * @param target An alive entity.
 * @param kind The timer's kind: the game's, passed to its dispatch.
 * @param period Ticks between firings, 1 to 255.
 * @param cycles Firings, or 0 for ever.
 * @return The effect, or NGE_ENTITY_INVALID if no entity is free: nothing
 *         was created.
 */
nge_entity_id_t nge_effect_create(nge_entity_id_t target, uint8_t kind, uint8_t period, uint8_t cycles,
                                  uint8_t p0, uint8_t p1) NGE_BANKED;

/**
 * @brief Destroy an alive active effect: it is dying, so NGE_EFFECT_FOR_EACH
 *        no longer finds it and its timer no longer fires. Its owner and
 *        payload can be read until it is released.
 */
void nge_effect_destroy(nge_entity_id_t effect) NGE_BANKED;

#endif
