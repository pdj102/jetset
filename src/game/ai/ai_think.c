/*
 * The AI's turn, its states and their options: banked in page 130 with
 * the rest of the module (docs/ai_default_brain.md).
 *
 * Every turn has the same frame (ai_on_turn()):
 * 1. sense: ai_sense() fills the context, an ai_ctx_t on the stack, once
 *    (its capabilities, tile, target and goal, whether it sees the
 *    target). A stimulus from sensing goes to the brain at once, so the
 *    new state acts this turn;
 * 2. think: the state's handler, think_<state>(c), tries its options in
 *    order of priority and stops at the first that uses the turn:
 *
 *        return opt_cast(c) || opt_attack(c) || opt_drink(c) || opt_shoot(c) || opt_approach(c);
 *
 * 3. wait: if none did, the entity waits (ai_wait()), so every turn is
 *    exactly one action;
 * 4. a stimulus the options noticed (c->stim, AI_STIM_LOST) goes to the
 *    brain: the new state acts next turn.
 *
 * An option, opt_<name>(c), always has the same shape: does it apply
 * (capabilities, distance, hit points: from the context, with no banked
 * call if it can)? If so, fill an act_t and ai_try() it, returning 1 if
 * the turn was used; a refused action costs nothing, and the next option
 * is tried. Options only read the context and act: the state, target and
 * goal change through ai_sense() and ai_stimulus(). An option is written
 * once, and any state may list it; a new behaviour is a new option or a
 * new state, and the frame doesn't change.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "map/nge_map.h"

#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/spells/spells.h"

/* What opt_wield() readies: what it fights with, not what it wears */
#define WIELD_SLOTS  (SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO))

/* The first item id carries, not equipped, whose use effect is ef; NGE_ENTITY_INVALID if none */
static nge_entity_id_t carried_use(nge_entity_id_t id, uint8_t ef)
{
    uint8_t i;
    nge_entity_id_t item;

    NGE_OWNER_FOR_EACH_OWNED(i, item, id) {
        if (ITEM_IS(item) && item_slot[item] == SLOT_NONE && GEN_HOOK_EF(gen_hook_of(item, GEN_HOOK_USE)) == ef) {
            return item;
        }
    }
    return NGE_ENTITY_INVALID;
}

/* ---- The options ---- */

/* Attack the target, if it can and sees it: the action's check refuses it out of reach */
static uint8_t opt_attack(const ai_ctx_t *c)
{
    act_t a = { 0 };

    if (!(c->can & AI_CAN_MELEE) || !c->sees) {
        return 0;
    }
    a.actor = c->id;
    a.target = c->target;
    return ai_try(ACT_MELEE, &a);
}

/* At half its hit points or less: when it drinks a potion or casts a healing spell */
static uint8_t hurt(nge_entity_id_t id)
{
    return NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) && (uint16_t)health_hp[id] * 2 <= health_hp_max[id];
}

/*
 * Cast a spell (docs/spells.md, 4.9). Hurt, each spell it uses when hurt,
 * on itself. Seeing its target, each spell it casts at one, from a random
 * one: checked by hand, as opt_shoot() does, so it holds a spell that
 * would reach someone else. ACT_CAST refuses what it can't afford, so out
 * of MP it falls back on its other options. The random start is drawn out
 * of the action (actions.md, R5)
 */
static uint8_t opt_cast(const ai_ctx_t *c)
{
    /* The fields in locals (T-ZSDCC-1): a field read through c in a loop was miscompiled (screen_spell_bank.c) */
    nge_entity_id_t id = c->id;
    nge_entity_id_t target = c->target;
    uint8_t gx = c->gx;
    uint8_t gy = c->gy;
    uint8_t sees = c->sees;
    uint8_t first;
    uint8_t sp;
    act_t a = { 0 };

    if (!(c->can & AI_CAN_CAST)) {
        return 0;
    }
    a.actor = id;
    if (hurt(id)) {
        first = spell_next(id, SP_NONE, SPELL_AI_HURT);
        sp = first;
        while (sp != SP_NONE) {
            a.spell = sp;
            a.x = c->x;
            a.y = c->y;
            if (ai_try(ACT_CAST, &a)) {
                return 1;
            }
            sp = spell_next(id, sp, SPELL_AI_HURT);
            if (sp == first) {
                break;
            }
        }
    }
    if (!sees) {
        return 0;
    }
    sp = (uint8_t)(rand() & 15);   /* a mask, not %: 11 to 15 fold back to 1 to 5 */
    if (sp >= SPELL_COUNT) {
        sp = (uint8_t)(sp - (SPELL_COUNT - 1));
    }
    first = spell_next(id, sp, SPELL_AI_TARGET);
    sp = first;
    while (sp != SP_NONE) {
        a.spell = sp;
        a.x = gx;
        a.y = gy;
        if (act_do(ACT_CAST, &a, ACT_CHECK) == ACT_SUCCESS && a.target == target) {
            act_do(ACT_CAST, &a, ACT_DO);
            return a.used;
        }
        sp = spell_next(id, sp, SPELL_AI_TARGET);
        if (sp == first) {
            break;
        }
    }
    return 0;
}

/* Drink a healing potion it carries, if it can and is at half its hit points or less */
static uint8_t opt_drink(const ai_ctx_t *c)
{
    nge_entity_id_t id = c->id;
    act_t a = { 0 };

    if (!(c->can & AI_CAN_USE) || !hurt(id)) {
        return 0;
    }
    a.item = carried_use(id, EF_HEALING);
    if (a.item == NGE_ENTITY_INVALID) {
        return 0;
    }
    a.actor = id;
    return ai_try(ACT_USE, &a);
}

/*
 * Shoot the target, if it can and sees it. Sight first, as the check walks
 * the line. By hand, not ai_try(): the check names who a shot would hit,
 * and a creature in the way isn't the target, so it holds its fire
 */
static uint8_t opt_shoot(const ai_ctx_t *c)
{
    nge_entity_id_t target = c->target;
    act_t a = { 0 };

    if (!(c->can & AI_CAN_RANGED) || !c->sees) {
        return 0;
    }
    a.actor = c->id;
    a.x = c->gx;
    a.y = c->gy;
    if (act_do(ACT_FIRE, &a, ACT_CHECK) != ACT_SUCCESS || a.target != target) {
        return 0;
    }
    act_do(ACT_FIRE, &a, ACT_DO);
    return a.used;
}

/* Step towards the goal; there and the target not seen, or no way at all: lost. Rooted, it watches what it sees */
static uint8_t opt_approach(ai_ctx_t *c)
{
    uint8_t gx = c->gx;
    uint8_t gy = c->gy;
    uint8_t sees = c->sees;
    ai_can_t can = c->can;
    uint8_t used;

    if (sees && !(can & AI_CAN_MOVE)) {
        return 0;
    }
    if (gx == NGE_MAP_X_INVALID || (gx == c->x && gy == c->y && !sees)) {
        c->stim = AI_STIM_LOST;
        return 0;
    }
    used = ai_step_toward(c, gx, gy);
    if (!used) {
        c->stim = AI_STIM_LOST;
    }
    return used;
}

/* Pick up the first item here, if it can carry things and one is here */
static uint8_t opt_pickup(const ai_ctx_t *c)
{
    act_t a = { 0 };

    if (!(c->can & AI_CAN_PICKUP) || nge_location_find(c->area, c->x, c->y, COMPONENT_ITEM, 0) == NGE_ENTITY_INVALID) {
        return 0;   /* no banked call for nothing to pick up */
    }
    a.actor = c->id;
    a.target = NGE_ENTITY_INVALID;
    return ai_try(ACT_PICKUP, &a);
}

/* Ready the first weapon, launcher or ammo it carries that has a free slot, if it can equip */
static uint8_t opt_wield(const ai_ctx_t *c)
{
    nge_entity_id_t id = c->id;
    act_t a = { 0 };
    uint8_t i;
    nge_entity_id_t item;

    if (!(c->can & AI_CAN_EQUIP)) {
        return 0;
    }
    NGE_OWNER_FOR_EACH_OWNED(i, item, id) {
        if (ITEM_IS(item) && item_slot[item] == SLOT_NONE && (gen_item_fits(kind_index[item]) & WIELD_SLOTS)) {
            a.actor = id;
            a.item = item;
            a.slot = SLOT_NONE;   /* the slot it fits; the check refuses one it lacks or has filled */
            if (ai_try(ACT_EQUIP, &a)) {
                return 1;
            }
        }
    }
    return 0;
}

/* On half its turns, a step in a random direction: no doors opened */
static uint8_t opt_wander(const ai_ctx_t *c)
{
    if (((unsigned int)rand() & 1) == 0) {
        return 0;
    }
    return ai_step(c, (uint8_t)((unsigned int)rand() % 8), 0);
}

/* ---- The states ---- */

static uint8_t think_idle(ai_ctx_t *c)
{
    return opt_pickup(c) || opt_wield(c) || opt_wander(c);
}

static uint8_t think_hunt(ai_ctx_t *c)
{
    return opt_cast(c) || opt_attack(c) || opt_drink(c) || opt_shoot(c) || opt_approach(c);
}

/* ---- The turn ---- */

void ai_on_turn(nge_entity_id_t id) NGE_BANKED
{
    ai_ctx_t c;
    uint8_t used = 0;
    ai_stim_t stim;

    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id) && AI_IS(id));

    /* 1. Sense: what it noticed changes its state now */
    ai_sense(id, &c);
    stim = c.stim;
    if (stim != AI_STIM_NONE) {
        ai_stimulus(id, stim, c.target);
        c.stim = AI_STIM_NONE;
    }

    /* 2. Think: its state's options, in order */
    switch (ai_state[id]) {
    case AI_STATE_ASLEEP:
        break;   /* sleeps until it hears the player or is hurt */
    case AI_STATE_IDLE:
        used = think_idle(&c);
        break;
    case AI_STATE_HUNT:
        used = think_hunt(&c);
        break;
    default:
        nge_util_abort("bad ai state");
        break;
    }

    /* 3. Nothing applied: wait */
    if (!used) {
        ai_wait(&c);
    }

    /* 4. What acting noticed: the new state acts next turn */
    stim = c.stim;
    if (stim != AI_STIM_NONE) {
        ai_stimulus(id, stim, NGE_EVENT_NONE);
    }
}
