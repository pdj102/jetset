/* nge-dataseg: PAGE_121 */
/*
 * The effects' page data, in page 121 (slot 7 while the effects' code in
 * page 120 runs): the definitions, constant data with their texts, the
 * game's side's data for the core (effects_core.h: the attributes' kinds,
 * the timer kind), and the kind of each active effect (uninitialised: set
 * when one starts). Tables are here, not in page 120, whose addresses carry
 * the page in their high bits. See "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_121
#endif

#include "game/effects/effects_game.h"

#include "ecs/systems/nge_timer.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/timers.h"

#define TURNS(n)  ((n) * NGE_TIMER_TICKS_PER_TURN)

const effect_def_t effect_defs[EF_COUNT] = {
    [EF_NONE] = { 0 },
    [EF_HEALING] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel much better.",
        .on_start = { OP_HEAL, 0, 6 },
    },
    [EF_POISON] = {
        .start_text = "You feel sick.",
        .end_text = "The poison wears off.",
        .duration = EFFECT_TIMED,   /* refreshed, not doubled */
        .period = TURNS(3),
        .cycles = 4,
        .status = STATUS_POISONED,
        .on_tick = { OP_DAMAGE, DMG_POISON, 1 },
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
        .mod = { { ATTR_HP_MAX, 2 } },
    },
    [EF_STRENGTH] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel strong!",
        .on_start = { OP_RAISE_BASE, ATTR_STR, 1 },
    },
    [EF_REGEN] = {
        .duration = EFFECT_TIMED,
        .flags = EFFECT_STACK,   /* each source of regeneration counts */
        .period = TURNS(10),
        .cycles = 0,             /* for ever */
        .on_tick = { OP_HEAL, 0, 1 },
    },
};

/* ---- The game's side, read by the core ---- */

const uint8_t effects_game_attr_is_mask[ATTR_COUNT] = {
    [ATTR_HP_MAX] = 0,
    [ATTR_AC]     = 0,
    [ATTR_SPEED]  = 0,
    [ATTR_IMMUNE] = 1,
    [ATTR_RESIST] = 1,
    [ATTR_VULN]   = 1,
    [ATTR_STR]    = 0,
    [ATTR_DEX]    = 0,
    [ATTR_CON]    = 0,
    [ATTR_INT]    = 0,
    [ATTR_WIS]    = 0,
    [ATTR_CHA]    = 0,
};

const uint8_t effects_game_timer_kind = TIMER_EFFECT;

uint8_t effect_kind[NGE_ENTITY_MAX];
