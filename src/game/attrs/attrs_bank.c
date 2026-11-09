/*
 * The attributes module, banked in page 138 (see attrs.h): deriving each
 * entity's attribute values and statuses from its bases (page 139,
 * attrs_pdata.c, in slot 7 while this code runs) and the modifiers of its
 * sources, clamped and stored in its components' tables (slot 0).
 *
 * A source's statuses and modifiers are in the effect definitions in page
 * 121 (game/effects/effects_defs.h). mods_of() maps that page into slot 7
 * by hand and copies a row's to the stack: nothing stays mapped across a
 * banked call (N-BANK-4), and this page's constants are in slot 6.
 */
#ifdef __SDCC
#pragma codeseg PAGE_138
#pragma constseg PAGE_138
#endif

#include "game/attrs/attrs_internal.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_effect.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/stats.h"
#include "game/effects/effects_defs.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "hw/nge_mmu.h"

const attrs_attr_t attrs_table[ATTR_COUNT] = {
    [ATTR_HP_MAX] = { health_hp_max_base,               COMPONENT_HEALTH, 0 },
    [ATTR_AC]     = { health_ac_base,                   COMPONENT_HEALTH, 0 },
    [ATTR_SPEED]  = { stats_speed_base,                 COMPONENT_STATS,  0 },
    [ATTR_IMMUNE] = { health_immune_base,               COMPONENT_HEALTH, 1 },
    [ATTR_RESIST] = { health_resist_base,               COMPONENT_HEALTH, 1 },
    [ATTR_VULN]   = { health_vuln_base,                 COMPONENT_HEALTH, 1 },
    [ATTR_STR]    = { &stats_ability_base[ABIL_STR][0], COMPONENT_STATS,  0 },
    [ATTR_DEX]    = { &stats_ability_base[ABIL_DEX][0], COMPONENT_STATS,  0 },
    [ATTR_CON]    = { &stats_ability_base[ABIL_CON][0], COMPONENT_STATS,  0 },
    [ATTR_INT]    = { &stats_ability_base[ABIL_INT][0], COMPONENT_STATS,  0 },
    [ATTR_WIS]    = { &stats_ability_base[ABIL_WIS][0], COMPONENT_STATS,  0 },
    [ATTR_CHA]    = { &stats_ability_base[ABIL_CHA][0], COMPONENT_STATS,  0 },
    [ATTR_MP_MAX] = { mana_mp_max_base,                 COMPONENT_MANA,   0 },
};

/* Values worked out, before clamping */
typedef struct derived {
    int16_t value[ATTR_COUNT];
    uint8_t status;
} derived_t;

/* Attributes are kept for entities with health (and, for speed, stats; for MP, mana) */
static uint8_t has_attrs(nge_entity_id_t id)
{
    return NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH);
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

/*
 * Effect ef's statuses, and its modifiers into mod: copied from page 121,
 * mapped in for the copy only. Page 121 goes into slot 7 over this
 * module's page 139, the bases: read no base between the two maps.
 */
static uint8_t mods_of(uint8_t ef, effect_mod_t *mod)
{
    uint8_t page;
    uint8_t status;

    nge_util_assert(ef < EF_COUNT);
    page = nge_mmu_map(NGE_MMU_DATA_SLOT, EFFECTS_PDATA_PAGE);
    status = effect_defs[ef].status;
    memcpy(mod, effect_defs[ef].mod, sizeof(effect_defs[ef].mod));
    nge_mmu_map(NGE_MMU_DATA_SLOT, page);
    return status;
}

/* Add effect ef's statuses and modifiers at power to d */
static void add(derived_t *d, uint8_t ef, int8_t power)
{
    effect_mod_t mod[EFFECT_MODS];
    uint8_t i;
    uint8_t attr;
    int8_t amount;

    d->status |= mods_of(ef, mod);
    for (i = 0; i < EFFECT_MODS; i++) {
        attr = mod[i].attr;
        amount = mod[i].amount;
        nge_util_assert(attr < ATTR_COUNT);
        if (attrs_table[attr].is_mask) {
            d->value[attr] |= (uint8_t)amount;
        } else {
            d->value[attr] += times(amount, power);
        }
    }
}

/* id's base table of attribute attr, which id has (asserted) */
static uint8_t *base_of(nge_entity_id_t id, uint8_t attr)
{
    const attrs_attr_t *at;

    nge_util_assert(attr < ATTR_COUNT);
    at = &attrs_table[attr];
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(id, at->component));
    return at->base;
}

/* id's bases (0 for an attribute it hasn't), plus the statuses and modifiers of its equipped items' worn effects and of its active effects */
static void derive(nge_entity_id_t id, derived_t *d)
{
    nge_entity_id_t owned;
    nge_component_t component;
    uint16_t hook;
    uint8_t *base;
    uint8_t a;
    uint8_t i;

    for (a = 0; a < ATTR_COUNT; a++) {
        /* The fields in locals (T-ZSDCC-1) */
        base = attrs_table[a].base;
        component = attrs_table[a].component;
        d->value[a] = NGE_ENTITY_HAS_COMPONENT(id, component) ? base[id] : 0;
    }
    d->status = 0;
    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (ITEM_IS(owned) && item_slot[owned] != SLOT_NONE) {
            hook = gen_hook_of(owned, GEN_HOOK_WORN);
            add(d, GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook));
        } else if (NGE_ENTITY_HAS_COMPONENT(owned, NGE_COMPONENT_EFFECT)) {
            add(d, EFFECT_KIND_OF(owned), EFFECT_POWER_OF(owned));
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

/* An ability score within its range */
static uint8_t clamp_ability(int16_t v)
{
    if (v < STATS_ABILITY_MIN) {
        return STATS_ABILITY_MIN;
    }
    return (v > STATS_ABILITY_MAX) ? STATS_ABILITY_MAX : (uint8_t)v;
}

/* Set *p to v if write: 1 if it differs */
static uint8_t set(uint8_t *p, uint8_t v, uint8_t write)
{
    if (*p == v) {
        return 0;
    }
    if (write) {
        *p = v;
    }
    return 1;
}

/* Store d's values, clamped, in id's components if write: 1 if any differs */
static uint8_t store(nge_entity_id_t id, const derived_t *d, uint8_t write)
{
    uint8_t changed = 0;
    uint8_t max = clamp(d->value[ATTR_HP_MAX], 1);
    uint8_t a;

    changed |= set(&health_hp_max[id], max, write);
    if (health_hp[id] > max) {
        if (write) {
            health_hp[id] = max;   /* a lower maximum: hit points kept within it; a higher one doesn't heal */
        }
        changed = 1;
    }
    changed |= set(&health_ac[id], clamp(d->value[ATTR_AC], 0), write);
    changed |= set(&health_immune[id], (uint8_t)d->value[ATTR_IMMUNE], write);
    changed |= set(&health_resist[id], (uint8_t)d->value[ATTR_RESIST], write);
    changed |= set(&health_vuln[id], (uint8_t)d->value[ATTR_VULN], write);
    if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) &&
        set(&stats_speed[id], clamp(d->value[ATTR_SPEED], 1), write)) {
        changed = 1;
        if (write && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_TIMER)) {
            nge_timer_set_period(id, stats_speed[id]);   /* its turns: every speed ticks */
        }
    }
    if (MANA_IS(id)) {
        max = clamp(d->value[ATTR_MP_MAX], 1);
        changed |= set(&mana_mp_max[id], max, write);
        if (mana_mp[id] > max) {
            if (write) {
                mana_mp[id] = max;   /* as hit points: kept within a lower maximum */
            }
            changed = 1;
        }
    }
    if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS)) {
        changed |= set(&stats_status[id], d->status, write);   /* only creatures have statuses */
        for (a = 0; a < ABIL_COUNT; a++) {
            changed |= set(&stats_ability[a][id], clamp_ability(d->value[ATTR_ABILITY(a)]), write);
        }
    }
    return changed;
}

/* See effects_recompute(): a plain call for this page, as a banked one would nest a level deeper */
static void recompute(nge_entity_id_t id)
{
    derived_t d;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !has_attrs(id)) {
        return;
    }
    derive(id, &d);
    if (store(id, &d, 1)) {
        nge_event_post(id, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0, 0);
    }
}

void effects_recompute(nge_entity_id_t id) NGE_BANKED
{
    recompute(id);
}

uint8_t effects_check(nge_entity_id_t id) NGE_BANKED
{
    derived_t d;

    nge_util_assert(id < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(id) || !has_attrs(id)) {
        return 1;
    }
    derive(id, &d);
    return !store(id, &d, 0);
}

void effects_set_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(has_attrs(id));

    base_of(id, attr)[id] = value;
    recompute(id);
}

void effects_raise_base(nge_entity_id_t id, uint8_t attr, int8_t amount) NGE_BANKED
{
    uint8_t *base;
    int16_t v;

    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(has_attrs(id));

    base = base_of(id, attr);
    v = (int16_t)base[id] + amount;   /* a base stays within 0-255 */
    base[id] = (v < 0) ? 0 : (v > 255) ? 255 : (uint8_t)v;
    recompute(id);
}

uint8_t effects_base(nge_entity_id_t id, uint8_t attr) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);

    return base_of(id, attr)[id];
}

void effects_put_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    base_of(id, attr)[id] = value;
}
