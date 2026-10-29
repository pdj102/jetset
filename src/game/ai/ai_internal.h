#ifndef AI_INTERNAL_H
#define AI_INTERNAL_H

/*
 * Shared by the AI module's files; not for use outside it:
 * - ai.c (common memory): the event filter;
 * - ai_bank.c (page 130): the component, the turn, the stimuli, ai_can();
 * - ai_act.c (page 130): ai_try(), the states' way to act;
 * - ai_brains.c (page 130, const): the brains;
 * - ai_pdata.c (page 131): each entity's state and brain.
 *
 * Page 131 is in slot 7 only while page 130's code runs. A call to any
 * other page (the actions, the components, nge) maps that page over slots
 * 6 and 7, so never pass a pointer into page 131 or to a page 130
 * constant: copy into a local on the stack first.
 */

#include "game/ai/ai.h"

#include "game/actions/act.h"

/* A brain: the next state for each state and stimulus; AI_STATE_NO_CHANGE: stay */
typedef struct ai_brain_def {
    ai_state_t next[AI_STATE_COUNT][AI_STIM_COUNT];
} ai_brain_def_t;

/* The brains, by AI_BRAIN_* (ai_brains.c, page 130) */
extern const ai_brain_def_t ai_brains[AI_BRAIN_COUNT];

/* Page data (ai_pdata.c, page 131): meaningful only with COMPONENT_AI */
extern ai_state_t ai_state[NGE_ENTITY_MAX];
extern ai_brain_t ai_brain[NGE_ENTITY_MAX];

/*
 * Stimulus stim for AI entity id, caused by source (or NGE_EVENT_NONE):
 * its brain's next state, and EV_AI_CHANGED if that changes it
 */
void ai_stimulus(nge_entity_id_t id, ai_stim_t stim, nge_entity_id_t source) NGE_BANKED;

/*
 * Do action (ACT_*) for a->actor if its check passes: a on the caller's
 * stack. Returns a->used: 1 if the turn was used. Refused, it changes
 * nothing and a->reason says why
 */
uint8_t ai_try(uint8_t action, act_t *a) NGE_BANKED;

#endif
