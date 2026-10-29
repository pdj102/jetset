/*
 * The AI module: banked in page 130, with each entity's state and brain in
 * page 131 (ai_pdata.c). The component, the turn, the stimuli and the
 * capabilities; acting is in ai_act.c.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

#include "core/nge_util.h"

#include "game/components/attack.h"
#include "game/components/inventory.h"
#include "game/components/stats.h"
#include "game/events.h"

static void check_entity(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
}

/* An entity in use with the AI component */
static void check_ai(nge_entity_id_t id)
{
    check_entity(id);
    nge_util_assert(AI_IS(id));
}

/* ---- The component ---- */

void ai_add(nge_entity_id_t id, const ai_kind_t *k) NGE_BANKED
{
    ai_brain_t brain = k->brain;
    ai_state_t state = k->state;

    check_entity(id);
    nge_util_assert(brain != AI_BRAIN_NONE && brain < AI_BRAIN_COUNT);
    nge_util_assert(state != AI_STATE_NONE && state < AI_STATE_COUNT);

    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_AI);
    ai_brain[id] = brain;
    ai_state[id] = state;
}

void ai_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id);

    NGE_ENTITY_REMOVE_COMPONENT(id, COMPONENT_AI);
}

ai_state_t ai_get_state(nge_entity_id_t id) NGE_BANKED
{
    check_ai(id);

    return ai_state[id];
}

/* ---- Capabilities ---- */

ai_can_t ai_can(nge_entity_id_t id) NGE_BANKED
{
    ai_can_t can = 0;
    uint8_t intel = 0;   /* no stats: none of the AI's rules that need a mind */

    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));

    if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS)) {
        intel = stats_ability[ABIL_INT][id];   /* the value: effects count */
    }
    if (ATTACK_HAS(id, ATTACK_MELEE) || attack_weapon(id) != NGE_ENTITY_INVALID) {
        can |= AI_CAN_MELEE;
    }
    if (INV_HAS(id)) {
        can |= AI_CAN_PICKUP;
        if (inv_slots[id] != 0) {
            can |= AI_CAN_EQUIP;
        }
        if (intel >= AI_INT_USE) {
            can |= AI_CAN_USE;
        }
    }
    if (intel >= AI_INT_OPEN) {
        can |= AI_CAN_OPEN;
    }
    return can;
}

/* ---- The state machine ---- */

void ai_stimulus(nge_entity_id_t id, ai_stim_t stim, nge_entity_id_t source) NGE_BANKED
{
    ai_state_t old;
    ai_state_t next;

    check_ai(id);
    nge_util_assert(stim < AI_STIM_COUNT);

    old = ai_state[id];
    next = ai_brains[ai_brain[id]].next[old][stim];
    if (next == AI_STATE_NO_CHANGE || next == old) {
        return;
    }
    ai_state[id] = next;
    nge_event_post(id, EV_AI_CHANGED, source, old, next);
}

/*
 * A state that acts gets a handler, think_<state>(id, can), returning a
 * stimulus (what it noticed: AI_STIM_NONE for nothing), which is passed to
 * ai_stimulus(id, stim, NGE_EVENT_NONE): the new state acts next turn.
 * Neither state does yet
 */
void ai_on_turn(nge_entity_id_t id) NGE_BANKED
{
    check_ai(id);

    switch (ai_state[id]) {
    case AI_STATE_ASLEEP:
        break;   /* sleeps until something wakes it (ai_on_event()) */
    case AI_STATE_IDLE:
        break;   /* waits (later: wanders, looks for the player) */
    default:
        nge_util_abort("bad ai state");
        break;
    }
}
