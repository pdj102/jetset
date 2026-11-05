/*
 * Effect system: banked in page 58, with the effect component and the
 * game's hook as page data in page 59 (nge_effect_pdata.c), which
 * banked_call maps into slot 7 for each call. It reads the entity and owner
 * tables in slot 0 and calls the game's hook, in common memory, which may
 * call banked game code: the hook's banked calls map other pages, and
 * banked_call maps these back when they return.
 */
#ifdef __SDCC
#pragma codeseg PAGE_58
#pragma constseg PAGE_58
#endif

#include "ecs/systems/nge_effect_internal.h"

#include "core/nge_util.h"
#include "ecs/systems/nge_timer.h"

/* An active effect in use (alive or dying, or alive only) */
static void check_effect(nge_entity_id_t effect, uint8_t alive)
{
    (void)effect;   /* unused when asserts are compiled out (NDEBUG) */
    (void)alive;
    nge_util_assert(effect < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(effect) != NGE_ENTITY_FREE);
    nge_util_assert(!alive || NGE_ENTITY_IS_ALIVE(effect));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT));
}

/* Tell the game what happened: the values by copy, as the hook may end the effect and map other pages */
static void hook(uint8_t what, nge_entity_id_t effect, nge_entity_id_t target, uint8_t kind, int8_t power,
                 uint8_t arg)
{
    nge_effect_hook_t h;
    nge_effect_hook_fn fn = nge_effect_game_hook;

    nge_util_assert(fn != 0);   /* nge_effect_init() */
    h.what = what;
    h.effect = effect;
    h.target = target;
    h.kind = kind;
    h.power = power;
    h.arg = arg;
    fn(&h);
}

void nge_effect_init(nge_effect_hook_fn hook_fn) NGE_BANKED
{
    nge_util_assert(hook_fn != 0);

    nge_effect_game_hook = hook_fn;
    nge_timer_set_handler(NGE_TIMER_EFFECT, nge_effect_timer);
}

nge_entity_id_t nge_effect_start(uint8_t kind, int8_t power, nge_entity_id_t target,
                                 uint8_t period, uint8_t cycles, uint8_t flags) NGE_BANKED
{
    nge_entity_id_t e;
    uint8_t i;
    uint8_t rose;

    nge_util_assert(power != 0);
    nge_util_assert(period != 0);
    nge_util_assert((flags & ~NGE_EFFECT_STACK) == 0);
    nge_util_assert(target < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(target));

    if (!(flags & NGE_EFFECT_STACK)) {
        NGE_EFFECT_FOR_EACH(i, e, target) {
            if (nge_effect_kinds[e] == kind) {
                nge_timer_add(e, NGE_TIMER_EFFECT, period, cycles);   /* refreshed: its full time again */
                rose = power > nge_effect_powers[e];
                if (rose) {
                    nge_effect_powers[e] = power;   /* a weaker one can't dilute it */
                }
                hook(NGE_EFFECT_REFRESHED, e, target, kind, nge_effect_powers[e], rose);
                return e;
            }
        }
    }
    e = nge_entity_create();
    if (e == NGE_ENTITY_INVALID) {
        return NGE_ENTITY_INVALID;
    }
    nge_owner_set(e, target);   /* it goes when its target goes */
    nge_effect_kinds[e] = kind;
    nge_effect_powers[e] = power;
    NGE_ENTITY_ADD_COMPONENT(e, NGE_COMPONENT_EFFECT);
    nge_timer_add(e, NGE_TIMER_EFFECT, period, cycles);
    hook(NGE_EFFECT_STARTED, e, target, kind, power, 0);
    return e;
}

/* See nge_effect_end(): a plain call for this page, as a banked one would nest a level deeper */
static void end(nge_entity_id_t effect, uint8_t reason)
{
    nge_entity_id_t target;
    uint8_t kind;
    int8_t power;

    check_effect(effect, 1);
    nge_util_assert(NGE_OWNER_IS_ENTITY(NGE_OWNER_OF(effect)));
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    kind = nge_effect_kinds[effect];
    power = nge_effect_powers[effect];

    nge_timer_remove(effect);
    nge_entity_destroy(effect);   /* dying: no longer an alive effect of target's */
    hook(NGE_EFFECT_ENDED, effect, target, kind, power, reason);
}

void nge_effect_end(nge_entity_id_t effect, uint8_t reason) NGE_BANKED
{
    end(effect, reason);
}

void nge_effect_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED
{
    check_effect(effect, 1);
    hook(NGE_EFFECT_TICK, effect, (nge_entity_id_t)NGE_OWNER_OF(effect), nge_effect_kinds[effect],
         nge_effect_powers[effect], cycles);
    /* The hook may have ended it, or destroyed its target (and so it) */
    if (cycles == 1 && NGE_ENTITY_IS_ALIVE(effect)) {
        end(effect, NGE_EFFECT_END_EXPIRED);   /* its last firing */
    }
}

uint8_t nge_effect_kind(nge_entity_id_t effect) NGE_BANKED
{
    check_effect(effect, 0);

    return nge_effect_kinds[effect];
}

int8_t nge_effect_power(nge_entity_id_t effect) NGE_BANKED
{
    check_effect(effect, 0);

    return nge_effect_powers[effect];
}
