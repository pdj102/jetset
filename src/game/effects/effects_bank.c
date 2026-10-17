/*
 * The effect system: banked in page 120, with the definitions and the
 * active effects' kinds in page 121 (effects_pdata.c), mapped into slot 7
 * while this code runs (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). See effects.h.
 */
#ifdef __SDCC
#pragma codeseg PAGE_120
#pragma constseg PAGE_120
#endif

#include "game/effects/effects_internal.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/timers.h"

/* Derived values being worked out: numbers can pass 255 or go below 0 until clamped */
typedef struct derived {
    int16_t value[ATTR_COUNT];
    uint8_t status;
} derived_t;

/* An entity effects can act on: alive, with health, not dead */
static uint8_t affectable(nge_entity_id_t id)
{
    return NGE_ENTITY_IS_ALIVE(id) && NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_HEALTH) && health_hp[id] != 0;
}

/* An alive active effect */
static void check_effect(nge_entity_id_t effect)
{
    (void)effect;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(effect < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_IS_ALIVE(effect));
    nge_util_assert(NGE_ENTITY_HAS_ANY_COMPONENTS(effect, COMPONENT_EFFECT));
    nge_util_assert(NGE_OWNER_IS_ENTITY(NGE_OWNER_OF(effect)));
}

/* ---- Derived values ---- */

/* Add effect ef's modifiers and statuses */
static void add_effect(derived_t *d, uint8_t ef)
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
        if (ATTR_IS_MASK(attr)) {
            d->value[attr] |= (uint8_t)amount;
        } else {
            d->value[attr] += amount;
        }
    }
}

/* id's bases, plus its equipped items' worn effects and its active effects */
static void derive(nge_entity_id_t id, derived_t *d)
{
    nge_entity_id_t owned;
    uint8_t ef;
    uint8_t a;
    uint8_t i;

    for (a = 0; a < ATTR_COUNT; a++) {
        d->value[a] = attr_base[a][id];
    }
    d->status = 0;
    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (ITEM_IS(owned)) {
            if (item_slot[owned] != SLOT_NONE) {
                ef = gen_hook(owned, GEN_HOOK_WORN);   /* maps the item table's page, then this one again */
                add_effect(d, ef);
            }
        } else if (NGE_ENTITY_HAS_ANY_COMPONENTS(owned, COMPONENT_EFFECT)) {
            add_effect(d, effect_kind[owned]);
        }
    }
}

/* v within lo to 255 */
static uint8_t clamp(int16_t v, uint8_t lo)
{
    if (v < lo) {
        return lo;
    }
    return (v > 255) ? 255 : (uint8_t)v;
}

/* Set *p to v: 1 if that changed it */
static uint8_t set(uint8_t *p, uint8_t v)
{
    if (*p == v) {
        return 0;
    }
    *p = v;
    return 1;
}

void effects_recompute(nge_entity_id_t id) NGE_BANKED
{
    derived_t d;
    uint8_t changed = 0;
    uint8_t max;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_HEALTH)) {
        return;
    }
    derive(id, &d);

    max = clamp(d.value[ATTR_HP_MAX], 1);
    changed |= set(&health_hp_max[id], max);
    if (health_hp[id] > max) {
        health_hp[id] = max;   /* a lower maximum: hit points kept within it; a higher one doesn't heal */
        changed = 1;
    }
    changed |= set(&health_ac[id], clamp(d.value[ATTR_AC], 0));
    changed |= set(&health_immune[id], (uint8_t)d.value[ATTR_IMMUNE]);
    changed |= set(&health_resist[id], (uint8_t)d.value[ATTR_RESIST]);
    changed |= set(&health_vuln[id], (uint8_t)d.value[ATTR_VULN]);
    if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_STATS) &&
        set(&stats_speed[id], clamp(d.value[ATTR_SPEED], 1))) {
        changed = 1;
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, NGE_COMPONENT_TIMER)) {
            nge_timer_set_period(id, stats_speed[id]);   /* its turns: every speed ticks */
        }
    }
    changed |= set(&status_mask[id], d.status);

    if (changed) {
        nge_event_post(id, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0, 0);
    }
}

uint8_t effects_check(nge_entity_id_t id) NGE_BANKED
{
    derived_t d;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_HEALTH)) {
        return 1;
    }
    derive(id, &d);
    if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_STATS) &&
        stats_speed[id] != clamp(d.value[ATTR_SPEED], 1)) {
        return 0;
    }
    return health_hp_max[id] == clamp(d.value[ATTR_HP_MAX], 1) && health_hp[id] <= health_hp_max[id] &&
           health_ac[id] == clamp(d.value[ATTR_AC], 0) && health_immune[id] == (uint8_t)d.value[ATTR_IMMUNE] &&
           health_resist[id] == (uint8_t)d.value[ATTR_RESIST] && health_vuln[id] == (uint8_t)d.value[ATTR_VULN] &&
           status_mask[id] == d.status;
}

void effects_set_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_HEALTH));
    nge_util_assert(attr < ATTR_COUNT);

    attr_base[attr][id] = value;
    effects_recompute(id);
}

/* ---- Ops ---- */

/* Do an op to target, caused by source */
static void run_op(const effect_op_t *op, nge_entity_id_t target, nge_entity_id_t source)
{
    /* The fields in locals: see messages_on_event() (game/messages) */
    uint8_t what = op->op;
    uint8_t arg = op->arg;
    int8_t amount = op->amount;

    switch (what) {
    case OP_NONE:
        break;
    case OP_DAMAGE:
        health_damage(target, source, (uint8_t)amount, arg);
        break;
    case OP_HEAL:
        health_heal(target, (uint8_t)amount);
        break;
    case OP_CURE:
        effect_cure(target, arg);
        break;
    case OP_RAISE_BASE:
        nge_util_assert(arg < ATTR_COUNT);
        effects_set_base(target, arg, clamp((int16_t)attr_base[arg][target] + amount, 0));
        break;
    default:
        nge_util_abort("bad effect op");
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
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_EFFECT) && effect_kind[id] == ef) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

uint8_t effect_apply(uint8_t ef, nge_entity_id_t target, nge_entity_id_t source) NGE_BANKED
{
    effect_def_t def;   /* a copy: the calls below map other pages */
    nge_entity_id_t e;

    nge_util_assert(ef != EF_NONE && ef < EF_COUNT);
    nge_util_assert(target < NGE_ENTITY_MAX);

    if (!affectable(target)) {
        return 0;
    }
    memcpy(&def, &effect_defs[ef], sizeof(def));

    if (!(def.flags & EFFECT_TIMED)) {
        run_op(&def.start, target, source);
        nge_event_post(target, EV_EFFECT_STARTED, NGE_EVENT_NONE, ef, 0);
        return 1;
    }

    if (!(def.flags & EFFECT_STACK)) {
        e = find(target, ef);
        if (e != NGE_ENTITY_INVALID) {
            nge_timer_add(e, TIMER_EFFECT, def.period, def.cycles);   /* refreshed: its full time again */
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
    nge_entity_components[e] |= COMPONENT_EFFECT;
    nge_timer_add(e, TIMER_EFFECT, def.period, def.cycles);
    run_op(&def.start, target, e);
    effects_recompute(target);
    nge_event_post(target, EV_EFFECT_STARTED, e, ef, 0);
    return 1;
}

void effect_end(nge_entity_id_t effect) NGE_BANKED
{
    effect_op_t end;
    nge_entity_id_t target;
    uint8_t ef;

    check_effect(effect);
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    ef = effect_kind[effect];
    end = effect_defs[ef].end;

    nge_timer_remove(effect);
    nge_entity_destroy(effect);   /* dying: no longer counted by effects_recompute() */
    if (affectable(target)) {
        run_op(&end, target, effect);
        effects_recompute(target);
        nge_event_post(target, EV_EFFECT_ENDED, effect, ef, 0);
    }
}

uint8_t effect_cure(nge_entity_id_t target, uint8_t statuses) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t i;
    uint8_t n = 0;

    nge_util_assert(target < NGE_ENTITY_MAX);

    /* Ending the current entity is allowed while walking the alive list */
    NGE_OWNER_FOR_EACH_OWNED(i, id, target) {
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_EFFECT) && (effect_defs[effect_kind[id]].status & statuses)) {
            effect_end(id);
            n++;
        }
    }
    return n;
}

void effects_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED
{
    effect_op_t tick;
    nge_entity_id_t target;

    check_effect(effect);
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    tick = effect_defs[effect_kind[effect]].tick;

    if (affectable(target)) {
        run_op(&tick, target, effect);
    }
    if (cycles == 1 && NGE_ENTITY_IS_ALIVE(effect)) {
        effect_end(effect);   /* its last firing (unless its op ended it) */
    }
}

uint8_t effect_kind_of(nge_entity_id_t effect) NGE_BANKED
{
    nge_util_assert(effect < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_HAS_ANY_COMPONENTS(effect, COMPONENT_EFFECT));

    return effect_kind[effect];
}

void effect_text(uint8_t ef, uint8_t which, char *buf, uint8_t size) NGE_BANKED
{
    const char *s;

    nge_util_assert(ef < EF_COUNT);
    nge_util_assert(size >= 1);

    s = (which == EFFECT_TEXT_START) ? effect_defs[ef].start_text : effect_defs[ef].end_text;
    buf[0] = '\0';
    if (s != NULL) {
        strncat(buf, s, size - 1);   /* common code: it may read this page's text */
    }
}
