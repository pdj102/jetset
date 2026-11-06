/*
 * The effects module, banked in page 120, with the definitions in page 121
 * (effects_pdata.c), mapped into slot 7 while this code runs (a banked game
 * module, see "Banked game modules" in docs/memory_map.md). See effects.h.
 *
 * nge sets up an active effect (ecs/systems/nge_effect.h: an entity owned
 * by its target, with a timer whose payload holds its kind and power) and
 * finds them; this does the rest: refreshing and stacking, firings,
 * expiry, endings. Derived values are the attributes module's
 * (game/attrs/attrs.h).
 *
 * Banked calls nest deepest from an AI's hit that starts a timed effect:
 * nge_timer_advance(), ai_on_turn(), act_melee(), effect_apply(), then
 * nge_effect_create() and its calls, or effects_recompute() and
 * gen_hook_of(): 6 of the 8 allowed (T-STACK-2).
 */
#ifdef __SDCC
#pragma codeseg PAGE_120
#pragma constseg PAGE_120
#endif

#include "game/effects/effects_internal.h"

#include <stdlib.h>
#include <string.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "event/nge_event.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/timers.h"

typedef char effects_flags_check[!(EFFECT_STACK & EFFECT_INNATE) ? 1 : -1];

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

/* An op's amount at power, within what an int8_t amount holds */
static int8_t scaled(int8_t amount, int8_t power)
{
    int16_t v = times(amount, power);

    return (v < -127) ? -127 : (v > 127) ? 127 : (int8_t)v;
}

/* Alive, with health, not dead: what effects act on */
static uint8_t affectable(nge_entity_id_t id)
{
    return NGE_ENTITY_IS_ALIVE(id) && NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) && health_hp[id] != 0;
}

uint8_t effects_save(nge_entity_id_t target, uint8_t arg, uint8_t dc)
{
    int16_t roll;

    nge_util_assert(arg < ABIL_COUNT);
    roll = (int16_t)(1 + (unsigned int)rand() % 20);
    if (NGE_ENTITY_HAS_COMPONENT(target, COMPONENT_STATS)) {
        roll += STATS_MOD(stats_ability[arg][target]);
    }
    return roll >= (int16_t)dc;
}

/* The curse of everything id has equipped: a flag on any that isn't cursed means nothing */
static void uncurse(nge_entity_id_t id)
{
    nge_entity_id_t owned;
    uint8_t i;

    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (ITEM_IS(owned) && item_slot[owned] != SLOT_NONE) {
            nge_entity_set_flags(owned, ITEM_UNCURSED);
        }
    }
}

static void end(nge_entity_id_t effect, uint8_t reason);

/* See effect_cure(): a plain call for this page's ops */
static uint8_t cure(nge_entity_id_t target, uint8_t statuses)
{
    nge_entity_id_t e;
    uint8_t i;
    uint8_t ef;
    uint8_t n = 0;

    nge_util_assert(target < NGE_ENTITY_MAX);

    NGE_EFFECT_FOR_EACH(i, e, target) {
        ef = EFFECT_KIND_OF(e);
        if ((effect_defs[ef].status & statuses) && !(effect_defs[ef].flags & EFFECT_INNATE)) {
            end(e, EFFECT_END_CURED);
            n++;
        }
    }
    return n;
}

/* Do an op at power to target, caused by source */
static void run_op(const effect_op_t *op, int8_t power, nge_entity_id_t target, nge_entity_id_t source)
{
    /* The fields in locals: see messages_on_event() (game/messages) */
    uint8_t what = op->op;
    uint8_t arg = op->arg;
    int8_t amount = scaled(op->amount, power);

    switch (what) {
    case OP_NONE:
        break;
    case OP_CURE:
        cure(target, arg);
        break;
    case OP_RAISE_BASE:
        effects_raise_base(target, arg, amount);
        break;
    case OP_DAMAGE:
        health_damage(target, source, (uint8_t)amount, arg);
        break;
    case OP_HEAL:
        health_heal(target, (uint8_t)amount);
        break;
    case OP_MANA:
        if (MANA_IS(target)) {
            mana_restore(target, (uint8_t)amount);
        }
        break;
    case OP_UNCURSE:
        uncurse(target);
        break;
    case OP_LIGHT:
    case OP_IDENTIFY:
    case OP_MAP:
    case OP_TELEPORT:
    case OP_ENCHANT:
    case OP_SUMMON:
        break;   /* pending: their mechanics come later */
    default:
        nge_util_abort("bad effect op");
        break;
    }
}

/* ---- Starting, refreshing, firing and ending: nge only creates, finds and destroys ---- */

/* A new instance: its on_start op now (a burst; its on_tick op comes a period from now), then its modifiers count */
static void started(nge_entity_id_t effect, nge_entity_id_t target, uint8_t ef, int8_t power)
{
    effect_op_t on_start = effect_defs[ef].on_start;

    run_op(&on_start, power, target, effect);
    effects_recompute(target);
    nge_event_post(target, EV_EFFECT_STARTED, effect, ef, 0);
}

/* Applied again, not stacking: its full time again, and the stronger power kept so a weaker one can't dilute it */
static void refresh(nge_entity_id_t effect, nge_entity_id_t target, uint8_t ef, int8_t power, uint8_t period,
                    uint8_t cycles)
{
    nge_timer_add(effect, TIMER_EFFECT, period, cycles);   /* the payload is kept */
    if (power > EFFECT_POWER_OF(effect)) {
        nge_timer_payload[1][effect] = (uint8_t)power;
        effects_recompute(target);
    }
    nge_event_post(target, EV_EFFECT_STARTED, effect, ef, 0);
}

/*
 * The one way an effect ends, whatever the reason: destroyed first, so the
 * recompute no longer counts it, then its on_end op, unless its target is
 * dead
 */
static void end(nge_entity_id_t effect, uint8_t reason)
{
    /* Copied before it is destroyed: the target, kind and power */
    nge_entity_id_t target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    uint8_t ef = EFFECT_KIND_OF(effect);
    int8_t power = EFFECT_POWER_OF(effect);
    effect_op_t on_end;

    nge_util_assert(ef != EF_NONE && ef < EF_TIMED_COUNT);
    on_end = effect_defs[ef].on_end;
    nge_effect_destroy(effect);
    if (affectable(target)) {
        run_op(&on_end, power, target, effect);
        effects_recompute(target);
        nge_event_post(target, EV_EFFECT_ENDED, effect, ef, reason);
    }
}

void effects_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED
{
    nge_entity_id_t target;
    uint8_t ef;
    int8_t power;
    effect_op_t on_tick;
    uint8_t save_arg;
    uint8_t save_dc;

    nge_util_assert(effect < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(effect));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT));
    target = (nge_entity_id_t)NGE_OWNER_OF(effect);
    ef = EFFECT_KIND_OF(effect);
    power = EFFECT_POWER_OF(effect);
    nge_util_assert(ef != EF_NONE && ef < EF_TIMED_COUNT);
    on_tick = effect_defs[ef].on_tick;
    /* The fields in locals (T-ZSDCC-1) */
    save_arg = effect_defs[ef].save.arg;
    save_dc = effect_defs[ef].save.dc;

    if (affectable(target)) {
        run_op(&on_tick, power, target, effect);
    }
    /* Its op may have ended it, or killed its target */
    if (save_dc != 0 && NGE_ENTITY_IS_ALIVE(effect) && affectable(target) && effects_save(target, save_arg, save_dc)) {
        end(effect, EFFECT_END_SAVED);
    }
    if (cycles == 1 && NGE_ENTITY_IS_ALIVE(effect)) {
        end(effect, EFFECT_END_EXPIRED);   /* its last firing */
    }
}

uint8_t effect_apply(uint8_t ef, int8_t power, nge_entity_id_t target, nge_entity_id_t source) NGE_BANKED
{
    effect_def_t def;   /* a copy: the calls below map other pages */
    nge_entity_id_t e;
    uint8_t i;

    nge_util_assert(ef != EF_NONE && ef < EF_COUNT);
    nge_util_assert(power != 0);
    nge_util_assert(target < NGE_ENTITY_MAX);

    memcpy(&def, &effect_defs[ef], sizeof(def));
    nge_util_assert(def.duration != EFFECT_WORN);   /* worn effects count through equipment */
    nge_util_assert(def.save.dc == 0 || def.duration == EFFECT_TIMED);   /* a saving throw is tried at firings */
    if (!affectable(target)) {
        return 0;
    }

    if (def.duration == EFFECT_INSTANT) {
        run_op(&def.on_start, power, target, source);
        nge_event_post(target, EV_EFFECT_STARTED, NGE_EVENT_NONE, ef, 0);
        return 1;
    }
    if (!(def.flags & EFFECT_STACK)) {
        NGE_EFFECT_FOR_EACH(i, e, target) {
            if (EFFECT_KIND_OF(e) == ef) {
                refresh(e, target, ef, power, def.period, def.cycles);
                return 1;
            }
        }
    }
    e = nge_effect_create(target, TIMER_EFFECT, def.period, def.cycles, ef, (uint8_t)power);
    if (e == NGE_ENTITY_INVALID) {
        return 0;
    }
    started(e, target, ef, power);
    return 1;
}

void effect_end(nge_entity_id_t effect) NGE_BANKED
{
    end(effect, EFFECT_END_EXPIRED);
}

uint8_t effect_cure(nge_entity_id_t target, uint8_t statuses) NGE_BANKED
{
    return cure(target, statuses);
}

/* ---- Texts and saving throws ---- */

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
