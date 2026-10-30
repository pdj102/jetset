/* nge-dataseg: PAGE_121 */
/*
 * The effects' page data, in page 121 (slot 7 while the effects' code in
 * page 120 runs): the definitions, constant data with their texts, the
 * game's side's data for the core (effects_core.h: the attributes' tables,
 * the timer kind), and the kind and power of each active effect
 * (uninitialised: set when one starts). Tables are here, not in page 120, whose addresses carry
 * the page in their high bits. See "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_121
#endif

#include "game/effects/effects_game.h"

#include "ecs/systems/nge_timer.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/timers.h"

#define TURNS(n)  ((n) * NGE_TIMER_TICKS_PER_TURN)

const effect_def_t effect_defs[EF_COUNT] = {
    [EF_NONE] = { 0 },
    [EF_HEALING] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel much better.",
        .on_start = { OP_HEAL, 0, 1 },
    },
    [EF_POISON] = {
        .name = "Poison",
        .start_text = "You feel sick.",
        .end_text = "The poison wears off.",
        .duration = EFFECT_TIMED,   /* refreshed, not doubled */
        .period = TURNS(3),
        .cycles = 4,
        .status = STATUS_POISONED,
        .on_tick = { OP_DAMAGE, DMG_POISON, 1 },
        .save = { ABIL_CON, 10 },   /* shaken off at a firing on 55% for an average creature */
    },
    [EF_CURE] = {
        .duration = EFFECT_INSTANT,
        .on_start = { OP_CURE, STATUS_POISONED, 0 },
    },
    [EF_VITALITY] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel more vital.",
        .on_start = { OP_RAISE_BASE, ATTR_HP_MAX, 1 },
    },
    [EF_RING_HEALTH] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_HP_MAX, 1 } },
    },
    [EF_STRENGTH] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel strong!",
        .on_start = { OP_RAISE_BASE, ATTR_STR, 1 },
    },
    [EF_REGEN] = {
        .name = "Regeneration",
        .duration = EFFECT_TIMED,
        .flags = EFFECT_STACK,   /* each source of regeneration counts */
        .period = TURNS(10),
        .cycles = 0,             /* for ever */
        .on_tick = { OP_HEAL, 0, 1 },
    },
};

/* ---- The game's side, read by the core ---- */

const effects_attr_t effects_game_attrs[ATTR_COUNT] = {
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
};

const uint8_t effects_game_timer_kind = TIMER_EFFECT;

uint8_t effect_kind[NGE_ENTITY_MAX];
int8_t effect_power[NGE_ENTITY_MAX];
