/*
 * The effects core: the mechanism (see effects_core.h), banked in page
 * 120 with the game's side (effects_game.c), the definitions and the
 * active effects' kinds in page 121 (effects_pdata.c), mapped into slot 7
 * while this code runs (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). See effects.h.
 *
 * Only the core's own includes here: nothing of the game's components
 * (tests/Makefile checks it). The game is reached through the seam.
 */
#ifdef __SDCC
#pragma codeseg PAGE_120
#pragma constseg PAGE_120
#endif

#include "game/effects/effects_core.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "game/events.h"

/* An alive active effect */
static void check_effect(nge_entity_id_t effect)
{
    (void)effect;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(effect < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_IS_ALIVE(effect));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(effect, COMPONENT_EFFECT));
    nge_util_assert(NGE_OWNER_IS_ENTITY(NGE_OWNER_OF(effect)));
}

/* amount x power, by adding: a multiply would link the library's routine into common memory, which has no room */
static int16_t times(int8_t amount, int8_t power)
{
    int16_t v = 0;

    while (power > 0) {
        v += amount;
        power--;
    }
    while (power < 0) {
        v -= amount;
        power++;
    }
    return v;
}

/* ---- Derived values ---- */

void effects_add(effects_derived_t *d, uint8_t ef, int8_t power)
{
    const effect_def_t *def;
    uint8_t i;
    uint8_t attr;
    int8_t amount;

    nge_util_assert(ef < EF_COUNT);
    def = &effect_defs[ef];
    d->status |= def->status;
    for (i = 0; i < EFFECT_MODS; i++) {
        attr = def->mod[i].attr;
        amount = def->mod[i].amount;
        nge_util_assert(attr < ATTR_COUNT);
        if (effects_game_attrs[attr].is_mask) {
            d->value[attr] |= (uint8_t)amount;
        } else {
            d->value[attr] += times(amount, power);
        }
    }
}

/* id's base table of attribute attr, which id has (asserted) */
static uint8_t *base_of(nge_entity_id_t id, uint8_t attr)
{
    const effects_attr_t *at;

    nge_util_assert(attr < ATTR_COUNT);
    at = &effects_game_attrs[attr];
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(id, at->component));
    return at->base;
}

/* id's bases (0 for an attribute it hasn't), plus the modifiers and statuses of the game's sources and of its active effects */
static void derive(nge_entity_id_t id, effects_derived_t *d)
{
    nge_entity_id_t owned;
    nge_component_t component;
    uint8_t *base;
    uint8_t a;
    uint8_t i;

    for (a = 0; a < ATTR_COUNT; a++) {
        /* The fields in locals (T-ZSDCC-1) */
        base = effects_game_attrs[a].base;
        component = effects_game_attrs[a].component;
        d->value[a] = NGE_ENTITY_HAS_COMPONENT(id, component) ? base[id] : 0;
    }
    d->status = 0;
    effects_game_sources(id, d);
    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (NGE_ENTITY_HAS_COMPONENT(owned, COMPONENT_EFFECT)) {
            effects_add(d, effect_kind[owned], effect_power[owned]);
        }
    }
}

void effects_recompute(nge_entity_id_t id) NGE_BANKED
{
    effects_derived_t d;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !effects_game_has_attrs(id)) {
        return;
    }
    derive(id, &d);
    if (effects_game_store(id, &d, 1)) {
        nge_event_post(id, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0, 0);
    }
}

uint8_t effects_check(nge_entity_id_t id) NGE_BANKED
{
    effects_derived_t d;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !effects_game_has_attrs(id)) {
        return 1;
    }
    derive(id, &d);
    return !effects_game_store(id, &d, 0);
}

void effects_set_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(effects_game_has_attrs(id));

    base_of(id, attr)[id] = value;
    effects_recompute(id);
}

/* ---- Ops ---- */

/* An op's amount at power, within what an int8_t amount holds */
static int8_t scaled(int8_t amount, int8_t power)
{
    int16_t v = times(amount, power);

    return (v < -127) ? -127 : (v > 127) ? 127 : (int8_t)v;
}

/* Do an op at power to target, caused by source: the core's own, or the game's */
static void run_op(const effect_op_t *op, int8_t power, nge_entity_id_t target, nge_entity_id_t source)
{
    /* The fields in locals: see messages_on_event() (game/messages) */
    uint8_t what = op->op;
    uint8_t arg = op->arg;
    int8_t amount = scaled(op->amount, power);
    effect_op_t game_op;
    int16_t v;

    switch (what) {
    case OP_NONE:
        break;
    case OP_CURE:
        effect_cure(target, arg);
        break;
    case OP_RAISE_BASE:
        v = (int16_t)base_of(target, arg)[target] + amount;   /* a base stays within 0-255 */
        effects_set_base(target, arg, (v < 0) ? 0 : (v > 255) ? 255 : (uint8_t)v);
        break;
    default:
        game_op.op = what;
        game_op.arg = arg;
        game_op.amount = amount;
        effects_game_op(&game_op, target, source);
        break;
    }
}

/* ---- Active effects ---- */

/* target's active effect of kind ef, or NGE_ENTITY_INVALID */
static nge_entity_id_t find(nge_entity_id_t target, uint8_t ef)
{
    nge_entity_id_t id;
    uint8_t i;

    NGE_OWNER_FOR_EACH_OWNED(i, id, target) {
        if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_EFFECT) && effect_kind[id] == ef) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

uint8_t effect_apply(uint8_t ef, int8_t power, nge_entity_id_t target, nge_entity_id_t source) NGE_BANKED
{
    effect_def_t def;   /* a copy: the calls below map other pages */
    nge_entity_id_t e;

    nge_util_assert(ef != EF_NONE && ef < EF_COUNT);
    nge_util_assert(power != 0);
    nge_util_assert(target < NGE_ENTITY_MAX);

    memcpy(&def, &effect_defs[ef], sizeof(def));
    nge_util_assert(def.duration != EFFECT_WORN);   /* worn effects count through equipment */
    nge_util_assert(def.save.dc == 0 || def.duration == EFFECT_TIMED);   /* a saving throw is tried at firings */
    if (!effects_game_affectable(target)) {
        return 0;
    }

    if (def.duration == EFFECT_INSTANT) {
        run_op(&def.on_start, power, target, source);
        nge_event_post(target, EV_EFFECT_STARTED, NGE_EVENT_NONE, ef, 0);
        return 1;
    }

    /* Timed: refreshed, or a new instance */
    if (!(def.flags & EFFECT_STACK)) {
        e = find(target, ef);
        if (e != NGE_ENTITY_INVALID) {
            nge_timer_add(e, effects_game_timer_kind, def.period, def.cycles);   /* refreshed: its full time again */
            if (power > effect_power[e]) {
                effect_power[e] = power;   /* a weaker one can't dilute it */
                effects_recompute(target);
            }
            nge_event_post(target, EV_EFFECT_STARTED, e, ef, 0);
            return 1;
        }
    }
    e = nge_entity_create();
    if (e == NGE_ENTITY_INVALID) {
        return 0;
    }
    nge_owner_set(e, target);   /* it goes when its target goes */
    effect_kind[e] = ef;
    effect_power[e] = power;
    NGE_ENTITY_ADD_COMPONENT(e, COMPONENT_EFFECT);
    nge_timer_add(e, effects_game_timer_kind, def.period, def.cycles);
    /*
     * Its on_start op now (a burst); its on_tick op comes from the timer, a
     * period from now. Its modifiers and statuses count from now on
     */
    run_op(&def.on_start, power, target, e);
    effects_recompute(target);
    nge_event_post(target, EV_EFFECT_STARTED, e, ef, 0);
    return 1;
}

/* End an active effect for reason (EFFECT_END_*): see effect_end() */
static void end(nge_entity_id_t effect, uint8_t reason)
{
    effect_op_t on_end;
    nge_entity_id_t target;
    uint8_t ef;
    int8_t power;

    check_effect(effect);
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    ef = effect_kind[effect];
    power = effect_power[effect];
    on_end = effect_defs[ef].on_end;

    nge_timer_remove(effect);
    nge_entity_destroy(effect);   /* dying: no longer counted by effects_recompute() */
    if (effects_game_affectable(target)) {
        run_op(&on_end, power, target, effect);
        effects_recompute(target);
        nge_event_post(target, EV_EFFECT_ENDED, effect, ef, reason);
    }
}

void effect_end(nge_entity_id_t effect) NGE_BANKED
{
    end(effect, EFFECT_END_EXPIRED);
}

uint8_t effect_cure(nge_entity_id_t target, uint8_t statuses) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t i;
    uint8_t n = 0;

    nge_util_assert(target < NGE_ENTITY_MAX);

    /* Ending the current entity is allowed while walking the alive list */
    NGE_OWNER_FOR_EACH_OWNED(i, id, target) {
        if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_EFFECT) && (effect_defs[effect_kind[id]].status & statuses)) {
            end(id, EFFECT_END_CURED);
            n++;
        }
    }
    return n;
}

void effects_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED
{
    effect_op_t on_tick;
    nge_entity_id_t target;
    uint8_t ef;
    uint8_t save_arg;
    uint8_t save_dc;
    int8_t power;

    check_effect(effect);
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    ef = effect_kind[effect];
    power = effect_power[effect];
    on_tick = effect_defs[ef].on_tick;
    /* The fields in locals (T-ZSDCC-1) */
    save_arg = effect_defs[ef].save.arg;
    save_dc = effect_defs[ef].save.dc;

    if (effects_game_affectable(target)) {
        run_op(&on_tick, power, target, effect);
    }
    /* Its op may have ended it, or killed its target */
    if (save_dc != 0 && NGE_ENTITY_IS_ALIVE(effect) && effects_game_affectable(target) &&
        effects_game_save(target, save_arg, save_dc)) {
        end(effect, EFFECT_END_SAVED);
    }
    if (cycles == 1 && NGE_ENTITY_IS_ALIVE(effect)) {
        end(effect, EFFECT_END_EXPIRED);   /* its last firing (unless its op or its saving throw ended it) */
    }
}

uint8_t effect_kind_of(nge_entity_id_t effect) NGE_BANKED
{
    nge_util_assert(effect < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(effect, COMPONENT_EFFECT));

    return effect_kind[effect];
}

void effect_text(uint8_t ef, uint8_t which, char *buf, uint8_t size) NGE_BANKED
{
    const char *s;

    nge_util_assert(ef < EF_COUNT);
    nge_util_assert(size >= 1);
    nge_util_assert(which < EFFECT_TEXT_COUNT);

    switch (which) {
    case EFFECT_TEXT_START:
        s = effect_defs[ef].start_text;
        break;
    case EFFECT_TEXT_END:
        s = effect_defs[ef].end_text;
        break;
    default:
        s = effect_defs[ef].name;
        break;
    }
    buf[0] = '\0';
    if (s != NULL) {
        strncat(buf, s, size - 1);   /* common code: it may read this page's text */
    }
}

uint8_t effect_save_of(uint8_t ef, uint8_t *ability) NGE_BANKED
{
    nge_util_assert(ef < EF_COUNT);

    *ability = effect_defs[ef].save.arg;
    return effect_defs[ef].save.dc;
}
