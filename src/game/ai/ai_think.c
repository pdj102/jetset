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
 *        return opt_attack(c) || opt_drink(c) || opt_approach(c);
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

#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"

/* The first item id carries, not equipped, whose use effect is ef; NGE_ENTITY_INVALID if none */
static nge_entity_id_t carried_use(nge_entity_id_t id, uint8_t ef)
{
    uint8_t i;
    nge_entity_id_t item;

    NGE_OWNER_FOR_EACH_OWNED(i, item, id) {
        if (ITEM_IS(item) && item_slot[item] == SLOT_NONE && gen_hook(item, GEN_HOOK_USE) == ef) {
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

/* Drink a healing potion it carries, if it can and is at half its hit points or less */
static uint8_t opt_drink(const ai_ctx_t *c)
{
    nge_entity_id_t id = c->id;
    act_t a = { 0 };

    if (!(c->can & AI_CAN_USE) || !NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) ||
        (uint16_t)health_hp[id] * 2 > health_hp_max[id]) {
        return 0;
    }
    a.item = carried_use(id, EF_HEALING);
    if (a.item == NGE_ENTITY_INVALID) {
        return 0;
    }
    a.actor = id;
    return ai_try(ACT_USE, &a);
}

/* Step towards the goal; there and the target not seen, or no way at all: lost */
static uint8_t opt_approach(ai_ctx_t *c)
{
    uint8_t gx = c->gx;
    uint8_t gy = c->gy;
    uint8_t used;

    if (gx == NGE_MAP_X_INVALID || (gx == c->x && gy == c->y && !c->sees)) {
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

/* Wield the first weapon it carries, if it can equip and wields nothing */
static uint8_t opt_wield(const ai_ctx_t *c)
{
    nge_entity_id_t id = c->id;
    act_t a = { 0 };
    uint8_t i;
    nge_entity_id_t item;

    if (!(c->can & AI_CAN_EQUIP) || attack_weapon(id) != NGE_ENTITY_INVALID) {
        return 0;
    }
    NGE_OWNER_FOR_EACH_OWNED(i, item, id) {
        if (ITEM_IS(item) && item_slot[item] == SLOT_NONE) {
            a.actor = id;
            a.item = item;
            a.kind = SLOT_WEAPON;   /* the check refuses what isn't a weapon */
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
    return opt_attack(c) || opt_drink(c) || opt_approach(c);
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
