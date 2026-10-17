/* nge-dataseg: PAGE_121 */
/*
 * The effects' page data, in page 121 (slot 7 while the effects' code in
 * page 120 runs): the definitions, constant data with their texts, and the
 * kind of each active effect (uninitialised: set when one starts). See
 * effects_internal.h for the record and "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_121
#endif

#include "game/effects/effects_internal.h"

#include "ecs/systems/nge_timer.h"
#include "game/components/attrs.h"
#include "game/components/health.h"

#define TURNS(n)  ((n) * NGE_TIMER_TICKS_PER_TURN)

const effect_def_t effect_defs[EF_COUNT] = {
    [EF_NONE] = { 0 },
    [EF_HEALING] = {
        .start_text = "You feel much better.",
        .start = { OP_HEAL, 0, 6 },
    },
    [EF_POISON] = {
        .start_text = "You feel sick.",
        .end_text = "The poison wears off.",
        .flags = EFFECT_TIMED,   /* refreshed, not doubled */
        .period = TURNS(3),
        .cycles = 4,
        .status = STATUS_POISONED,
        .tick = { OP_DAMAGE, DMG_POISON, 1 },
    },
    [EF_CURE] = {
        .start = { OP_CURE, STATUS_POISONED, 0 },
    },
    [EF_VITALITY] = {
        .start_text = "You feel more vital.",
        .start = { OP_RAISE_BASE, ATTR_HP_MAX, 1 },
    },
    [EF_RING_HEALTH] = {
        .mod = { { ATTR_HP_MAX, 2 } },
    },
    [EF_REGEN] = {
        .flags = EFFECT_TIMED | EFFECT_STACK,   /* each source of regeneration counts */
        .period = TURNS(10),
        .cycles = 0,             /* for ever */
        .tick = { OP_HEAL, 0, 1 },
    },
};

uint8_t effect_kind[NGE_ENTITY_MAX];
