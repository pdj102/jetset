/*
 * The game's side of the effects module (effects_core.h): what the
 * attributes mean and where their derived values live, the game's ops,
 * the sources that aren't active effects (equipped items), and which
 * entities effects act on. Banked in page 120 with the core, which calls
 * these with plain calls.
 */
#ifdef __SDCC
#pragma codeseg PAGE_120
#pragma constseg PAGE_120
#endif

#include "game/effects/effects_game.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/stats.h"
#include "game/gen/gen.h"

/* Its data (the attributes' tables, the timer kind) is in page 121 with the definitions: effects_pdata.c */

/* Attributes are kept for entities with health (and, for speed, stats) */
uint8_t effects_game_has_attrs(nge_entity_id_t id)
{
    return NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH);
}

/* 1d20 + the ability's modifier (0 without stats) >= dc */
uint8_t effects_game_save(nge_entity_id_t target, uint8_t arg, uint8_t dc)
{
    int16_t roll;

    nge_util_assert(arg < ABIL_COUNT);
    roll = (int16_t)(1 + (unsigned int)rand() % 20);
    if (NGE_ENTITY_HAS_COMPONENT(target, COMPONENT_STATS)) {
        roll += STATS_MOD(stats_ability[arg][target]);
    }
    return roll >= (int16_t)dc;
}

/* Alive, with health, not dead */
uint8_t effects_game_affectable(nge_entity_id_t id)
{
    return NGE_ENTITY_IS_ALIVE(id) && effects_game_has_attrs(id) && health_hp[id] != 0;
}

void effects_game_op(const effect_op_t *op, nge_entity_id_t target, nge_entity_id_t source)
{
    /* The fields in locals: see messages_on_event() (game/messages) */
    uint8_t what = op->op;
    uint8_t arg = op->arg;
    uint8_t amount = (uint8_t)op->amount;

    switch (what) {
    case OP_DAMAGE:
        health_damage(target, source, amount, arg);
        break;
    case OP_HEAL:
        health_heal(target, amount);
        break;
    case OP_MANA:
        if (MANA_IS(target)) {
            mana_restore(target, amount);
        }
        break;
    case OP_LIGHT:
    case OP_IDENTIFY:
    case OP_MAP:
    case OP_TELEPORT:
    case OP_UNCURSE:
    case OP_ENCHANT:
    case OP_SUMMON:
        break;   /* pending: their mechanics come later */
    default:
        nge_util_abort("bad effect op");
        break;
    }
}

/* The worn effects of the items id has equipped */
void effects_game_sources(nge_entity_id_t id, effects_derived_t *d)
{
    nge_entity_id_t owned;
    uint16_t hook;
    uint8_t i;

    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (ITEM_IS(owned) && item_slot[owned] != SLOT_NONE) {
            hook = gen_hook_of(owned, GEN_HOOK_WORN);   /* maps the item table's page, then this one again */
            effects_add(d, GEN_HOOK_EF(hook), GEN_HOOK_POWER(hook));
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

uint8_t effects_game_store(nge_entity_id_t id, const effects_derived_t *d, uint8_t write)
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
    if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS)) {
        changed |= set(&stats_status[id], d->status, write);   /* only creatures have statuses */
        for (a = 0; a < ABIL_COUNT; a++) {
            changed |= set(&stats_ability[a][id], clamp_ability(d->value[ATTR_ABILITY(a)]), write);
        }
    }
    return changed;
}
