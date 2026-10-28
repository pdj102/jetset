/*
 * The player character component's functions: banked in page 110 with
 * the other component functions, and the level table with them; its
 * variables are in slot 0 (pc_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/pc.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"

/* A new character's name, until pc_set_name() */
static const char default_name[] = "Rogue";

/*
 * The XP each level needs, by level - PC_LEVEL_MIN: classic in shape
 * (doubling, then slower), scaled so a player who clears most of each
 * depth is about level 10 at the bottom (docs/player_character.md, D6)
 */
static const uint16_t level_xp[PC_LEVEL_MAX - PC_LEVEL_MIN + 1] = {
    0, 20, 40, 80, 160, 320, 640, 1200, 2000, 3000,
};

/* The player, in use with the PC component */
static void check_pc(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(PC_IS(id));
}

void pc_add(nge_entity_id_t id) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(NGE_ENTITY_HAS_COMPONENTS(id, COMPONENT_HEALTH | COMPONENT_STATS));   /* a new level needs them */

    nge_entity_components[id] |= COMPONENT_PC;
    pc_set_name(id, default_name);
    pc_class = PC_CLASS_FIGHTER;
    pc_level = PC_LEVEL_MIN;
    pc_xp = 0;
    pc_gold = 0;
    pc_depth_max = 1;
}

void pc_set_name(nge_entity_id_t id, const char *name) NGE_BANKED
{
    uint8_t i;

    check_pc(id);
    nge_util_assert(name[0] != '\0');

    for (i = 0; i < PC_NAME_MAX && name[i] != '\0'; i++) {
        pc_name[i] = name[i];
    }
    pc_name[i] = '\0';
}

uint16_t pc_xp_needed(uint8_t level) NGE_BANKED
{
    nge_util_assert(level >= PC_LEVEL_MIN && level <= PC_LEVEL_MAX);

    return level_xp[level - PC_LEVEL_MIN];
}

/*
 * Go up a level: maximum hit points up by a roll of the hit die plus the
 * Constitution modifier, at least 1 (the base, through the effects: the
 * maximum stops at 255), and current hit points by the same
 */
static void level_up(nge_entity_id_t id)
{
    int16_t gain;
    uint16_t hp_max;

    gain = (int16_t)(1 + (unsigned int)rand() % PC_HIT_DIE) + STATS_MOD(stats_ability[ABIL_CON][id]);
    if (gain < 1) {
        gain = 1;
    }
    hp_max = (uint16_t)(health_hp_max_base[id] + gain);
    effects_set_base(id, ATTR_HP_MAX, hp_max > 255 ? 255 : (uint8_t)hp_max);
    health_heal(id, (uint8_t)gain);   /* up to the new maximum: posts EV_HEALED */
    pc_level++;
    nge_event_post(id, EV_LEVEL_GAINED, NGE_EVENT_NONE, pc_level, 0);
}

void pc_gain_xp(nge_entity_id_t id, uint16_t amount) NGE_BANKED
{
    uint16_t room;

    check_pc(id);
    room = (uint16_t)(PC_XP_MAX - pc_xp);
    if (amount > room) {
        amount = room;
    }
    if (amount == 0) {
        return;
    }
    pc_xp += amount;
    nge_event_post(id, EV_XP_GAINED, NGE_EVENT_NONE, (uint8_t)amount, (uint8_t)(amount >> 8));
    while (pc_level < PC_LEVEL_MAX && pc_xp >= level_xp[pc_level + 1 - PC_LEVEL_MIN]) {
        level_up(id);
    }
}

void pc_gain_gold(nge_entity_id_t id, uint16_t amount) NGE_BANKED
{
    uint16_t room;

    check_pc(id);
    room = (uint16_t)(PC_GOLD_MAX - pc_gold);
    if (amount > room) {
        amount = room;
    }
    if (amount == 0) {
        return;
    }
    pc_gold += amount;
    nge_event_post(id, EV_GOLD_CHANGED, NGE_EVENT_NONE, 0, 0);
    pc_gain_xp(id, amount);   /* classic D&D: an XP a gold piece */
}

void pc_spend_gold(nge_entity_id_t id, uint16_t amount) NGE_BANKED
{
    uint16_t gold;

    check_pc(id);
    gold = pc_gold;
    nge_util_assert(amount <= gold);   /* the caller checks PC_HAS_GOLD() */
    if (amount == 0) {
        return;
    }
    pc_gold = (amount >= gold) ? 0 : (uint16_t)(gold - amount);   /* never below 0, asserts or not */
    nge_event_post(id, EV_GOLD_CHANGED, NGE_EVENT_NONE, 0, 0);
}

void pc_reach_depth(nge_entity_id_t id, uint8_t depth) NGE_BANKED
{
    check_pc(id);

    if (depth > pc_depth_max) {
        pc_depth_max = depth;
    }
}
