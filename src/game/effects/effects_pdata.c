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
    [EF_POISON] = {
        .name = "Poison",
        .start_text = "You feel sick.",
        .end_text = "The poison wears off.",
        .duration = EFFECT_TIMED,
        .status = STATUS_POISONED,
        .on_tick = { OP_DAMAGE, DMG_POISON, 1 },
        .save = { ABIL_CON, 10 },   /* shaken off at a firing on 55% for an average creature */
        .period = TURNS(3),
        .cycles = 4,
    },
    [EF_REGEN] = {
        .name = "Regeneration",
        .duration = EFFECT_TIMED,
        .flags = EFFECT_STACK,   /* each source of regeneration counts */
        .period = TURNS(10),
        .cycles = 0,             /* for ever */
        .on_tick = { OP_HEAL, 0, 1 },
    },
    [EF_RENEW] = {
        .name = "Renewal",
        .start_text = "Your wounds begin to close.",
        .end_text = "The renewal fades.",
        .duration = EFFECT_TIMED,
        .on_tick = { OP_HEAL, 0, 1 },
        .period = TURNS(1),
        .cycles = 10,
    },
    [EF_HASTE] = {
        .name = "Haste",
        .start_text = "You speed up.",
        .end_text = "You slow down.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_SPEED, -1 } },
        .period = TURNS(5),
        .cycles = 4,
    },
    [EF_SLOW] = {
        .name = "Slow",
        .start_text = "You slow down.",
        .end_text = "You speed up.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_SPEED, 1 } },
        .save = { ABIL_WIS, 12 },
        .period = TURNS(5),
        .cycles = 4,
    },
    [EF_MIGHT] = {
        .name = "Might",
        .start_text = "You feel mighty!",
        .end_text = "Your might fades.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_STR, 1 } },
        .period = TURNS(5),
        .cycles = 4,
    },
    [EF_WEAKNESS] = {
        .name = "Weakness",
        .start_text = "You feel weak.",
        .end_text = "Your strength returns.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_STR, -1 } },
        .save = { ABIL_CON, 12 },
        .period = TURNS(5),
        .cycles = 4,
    },
    [EF_STONE_SKIN] = {
        .name = "Stoneskin",
        .start_text = "Your skin turns to stone.",
        .end_text = "Your skin softens.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_AC, 1 }, { ATTR_SPEED, 1 } },
        .period = TURNS(5),
        .cycles = 4,
    },
    [EF_RESIST_FIRE] = {
        .name = "Fire ward",
        .start_text = "You feel cool.",
        .end_text = "The fire ward fades.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_FIRE) } },
        .period = TURNS(10),
        .cycles = 3,
    },
    [EF_RESIST_COLD] = {
        .name = "Frost ward",
        .start_text = "You feel warm.",
        .end_text = "The frost ward fades.",
        .duration = EFFECT_TIMED,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_COLD) } },
        .period = TURNS(10),
        .cycles = 3,
    },
    [EF_BLIND] = {
        .name = "Blind",
        .start_text = "You can't see!",
        .end_text = "You can see again.",
        .duration = EFFECT_TIMED,
        .status = STATUS_BLINDED,
        .save = { ABIL_CON, 12 },
        .period = TURNS(2),
        .cycles = 5,
    },
    [EF_SLEEP] = {
        .name = "Asleep",
        .start_text = "You fall asleep.",
        .end_text = "You wake up.",
        .duration = EFFECT_TIMED,
        .status = STATUS_UNCONSCIOUS,
        .save = { ABIL_WIS, 12 },
        .period = TURNS(2),
        .cycles = 4,
    },
    [EF_HEALING] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel much better.",
        .on_start = { OP_HEAL, 0, 1 },
    },
    [EF_CURE] = {
        .duration = EFFECT_INSTANT,
        .on_start = { OP_CURE, STATUS_POISONED, 0 },
    },
    [EF_CURE_ALL] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel purified.",
        .on_start = { OP_CURE, STATUS_CONDITIONS, 0 },
    },
    [EF_MANA] = {
        .duration = EFFECT_INSTANT,
        .start_text = "Your mind clears.",
        .on_start = { OP_MANA, 0, 1 },
    },
    [EF_VITALITY] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel more vital.",
        .on_start = { OP_RAISE_BASE, ATTR_HP_MAX, 1 },
    },
    [EF_STRENGTH] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel strong!",
        .on_start = { OP_RAISE_BASE, ATTR_STR, 1 },
    },
    [EF_RAISE_DEX] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel nimble!",
        .on_start = { OP_RAISE_BASE, ATTR_DEX, 1 },
    },
    [EF_RAISE_CON] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel hardy!",
        .on_start = { OP_RAISE_BASE, ATTR_CON, 1 },
    },
    [EF_RAISE_INT] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel clever!",
        .on_start = { OP_RAISE_BASE, ATTR_INT, 1 },
    },
    [EF_RAISE_WIS] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel wise!",
        .on_start = { OP_RAISE_BASE, ATTR_WIS, 1 },
    },
    [EF_RAISE_CHA] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel charming!",
        .on_start = { OP_RAISE_BASE, ATTR_CHA, 1 },
    },
    [EF_NOURISH] = {
        .duration = EFFECT_INSTANT,
        .start_text = "That was good.",
        .on_start = { OP_HEAL, 0, 1 },
    },
    [EF_BOLT_MAGIC] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You are struck!",
        .on_start = { OP_DAMAGE, DMG_MAGIC, 1 },
    },
    [EF_BOLT_FIRE] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You are burned!",
        .on_start = { OP_DAMAGE, DMG_FIRE, 1 },
    },
    [EF_BOLT_COLD] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You are frozen!",
        .on_start = { OP_DAMAGE, DMG_COLD, 1 },
    },
    [EF_BOLT_ACID] = {
        .duration = EFFECT_INSTANT,
        .start_text = "The acid burns you!",
        .on_start = { OP_DAMAGE, DMG_ACID, 1 },
    },
    [EF_LIGHT] = {
        .duration = EFFECT_INSTANT,
        .start_text = "The room lights up.",
        .on_start = { OP_LIGHT, 0, 1 },
    },
    [EF_IDENTIFY] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel informed.",
        .on_start = { OP_IDENTIFY, 0, 1 },
    },
    [EF_MAP] = {
        .duration = EFFECT_INSTANT,
        .start_text = "A map forms in your mind.",
        .on_start = { OP_MAP, 0, 1 },
    },
    [EF_TELEPORT] = {
        .duration = EFFECT_INSTANT,
        .start_text = "Your body tingles.",
        .on_start = { OP_TELEPORT, 0, 1 },
    },
    [EF_UNCURSE] = {
        .duration = EFFECT_INSTANT,
        .start_text = "You feel cleansed.",
        .on_start = { OP_UNCURSE, 0, 1 },
    },
    [EF_ENCHANT] = {
        .duration = EFFECT_INSTANT,
        .start_text = "Your gear glows.",
        .on_start = { OP_ENCHANT, 0, 1 },
    },
    [EF_SUMMON] = {
        .duration = EFFECT_INSTANT,
        .start_text = "Something answers the call!",
        .on_start = { OP_SUMMON, 0, 1 },
    },
    [EF_WORN_HP] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_HP_MAX, 1 } },
    },
    [EF_WORN_AC] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_AC, 1 } },
    },
    [EF_WORN_SPEED] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_SPEED, -1 } },
    },
    [EF_WORN_STR] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_STR, 1 } },
    },
    [EF_WORN_DEX] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_DEX, 1 } },
    },
    [EF_WORN_CON] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_CON, 1 } },
    },
    [EF_WORN_INT] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_INT, 1 } },
    },
    [EF_WORN_WIS] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_WIS, 1 } },
    },
    [EF_WORN_RES_FIRE] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_FIRE) } },
    },
    [EF_WORN_RES_COLD] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_COLD) } },
    },
    [EF_WORN_RES_POISON] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_POISON) } },
    },
    [EF_WORN_RES_ACID] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_RESIST, DMG_BIT(DMG_ACID) } },
    },
    [EF_WORN_RES_MAGIC] = {
        .duration = EFFECT_WORN,
        .mod = { { ATTR_RESIST, (int8_t)DMG_BIT(DMG_MAGIC) } },
    },
    [EF_WORN_REGEN] = {
        .duration = EFFECT_WORN,
    },
    [EF_WORN_LIGHT] = {
        .duration = EFFECT_WORN,
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
