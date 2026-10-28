/*
 * The health component's functions: banked in page 110 with the other
 * component functions; its tables are in slot 0 (health_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/health.h"


#include <stddef.h>

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/pc.h"
#include "game/events.h"

typedef char health_types_check[(DMG_COUNT <= 8) ? 1 : -1];   /* the masks are one byte */

/* An entity in use with the health component */
static void check_health(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH));
}

void health_add(nge_entity_id_t id, const health_kind_t *k) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(k->hp_max != 0);

    health_hp[id] = k->hp_max;
    health_hp_max[id] = k->hp_max;
    health_immune[id] = k->immune;
    health_resist[id] = k->resist;
    health_vuln[id] = k->vuln;
    health_ac[id] = k->ac;
    health_hp_max_base[id] = k->hp_max;
    health_ac_base[id] = k->ac;
    health_immune_base[id] = k->immune;
    health_resist_base[id] = k->resist;
    health_vuln_base[id] = k->vuln;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_HEALTH);
}

uint8_t health_dealt(nge_entity_id_t id, uint8_t amount, uint8_t type, uint8_t *outcome) NGE_BANKED
{
    uint8_t bit;
    uint8_t result = DMG_NORMAL;
    uint16_t taken = amount;

    check_health(id);
    nge_util_assert(type < DMG_COUNT);

    bit = DMG_BIT(type);
    if (health_immune[id] & bit) {
        result = DMG_IMMUNE;
        taken = 0;
    } else if ((health_resist[id] & bit) && !(health_vuln[id] & bit)) {
        result = DMG_RESISTED;
        taken = (taken + 1) / 2;   /* rounded up: 1 stays 1 */
    } else if ((health_vuln[id] & bit) && !(health_resist[id] & bit)) {
        result = DMG_VULNERABLE;
        taken *= 2;
        if (taken > 255) {
            taken = 255;
        }
    }
    if (outcome != NULL) {
        *outcome = result;
    }
    return (uint8_t)taken;
}

uint8_t health_damage(nge_entity_id_t target, nge_entity_id_t source, uint8_t amount,
                      uint8_t type) NGE_BANKED
{
    uint8_t outcome;
    uint8_t taken;
    uint8_t hp;

    check_health(target);
    hp = health_hp[target];
    if (hp == 0) {
        return 0;   /* dead already */
    }
    taken = health_dealt(target, amount, type, &outcome);
    hp = (taken >= hp) ? 0 : (uint8_t)(hp - taken);
    health_hp[target] = hp;

    nge_event_post(target, EV_DAMAGED, source, taken, EV_DMG_PACK(outcome, type));
    if (hp == 0) {
        nge_event_post(target, EV_DIED, source, 0, 0);
        if (!PC_IS(target)) {
            /* A persistent creature (a boss) dies too; what it owns keeps its own flag */
            nge_entity_clear_flags(target, NGE_ENTITY_PERSISTENT);
        }
        nge_entity_destroy(target);   /* does nothing to the player: the game ends instead */
    }
    return taken;
}

uint8_t health_heal(nge_entity_id_t target, uint8_t amount) NGE_BANKED
{
    uint8_t hp;
    uint8_t room;

    check_health(target);
    hp = health_hp[target];
    if (hp == 0) {
        return 0;   /* the dead stay dead */
    }
    room = (uint8_t)(health_hp_max[target] - hp);
    if (amount > room) {
        amount = room;
    }
    if (amount != 0) {
        health_hp[target] = (uint8_t)(hp + amount);
        nge_event_post(target, EV_HEALED, NGE_EVENT_NONE, amount, 0);
    }
    return amount;
}
