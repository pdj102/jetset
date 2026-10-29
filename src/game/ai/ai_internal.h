#ifndef AI_INTERNAL_H
#define AI_INTERNAL_H

/*
 * Shared by the AI module's files; not for use outside it:
 * - ai.c (common memory): the event filter;
 * - ai_bank.c (page 130): the component, the stimuli, ai_can();
 * - ai_sense.c (page 130): the context, hearing and sight;
 * - ai_think.c (page 130): the turn, the states and their options;
 * - ai_act.c (page 130): ai_try() and the moves built on it, the states'
 *   way to act;
 * - ai_brains.c (page 130, const): the brains;
 * - ai_pdata.c (page 131): each entity's state, brain, target and goal.
 *
 * The files that act (ai_sense.c, ai_think.c, ai_act.c) are apart from
 * ai_bank.c so that code needing only the component (the generators and
 * their tests) doesn't need the actions.
 *
 * The functions here that aren't NGE_BANKED are called only from page 130,
 * with no bank switch (as the actions' helpers, act_internal.h).
 *
 * Page 131 is in slot 7 only while page 130's code runs. A call to any
 * other page (the actions, the components, nge) maps that page over slots
 * 6 and 7, so never pass a pointer into page 131 or to a page 130
 * constant: copy into a local on the stack first.
 */

#include "game/ai/ai.h"

#include "ecs/nge_owner.h"
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
extern nge_entity_id_t ai_target[NGE_ENTITY_MAX];   /* who it is after; NGE_ENTITY_INVALID: no one */
extern uint8_t ai_goal_x[NGE_ENTITY_MAX];           /* where it thinks the target is; NGE_MAP_X_INVALID: */
extern uint8_t ai_goal_y[NGE_ENTITY_MAX];           /* nowhere */
extern uint8_t ai_track[NGE_ENTITY_MAX];            /* turns the goal may follow the target unseen; */
                                                    /* below AI_TRACK: following started */

/* What an AI entity knows on this turn: ai_sense() fills it, on the stack */
typedef struct ai_ctx {
    nge_entity_id_t id;       /* the entity */
    ai_can_t can;             /* ai_can(): this turn's capabilities */
    nge_owner_t area;         /* its area, and its tile */
    uint8_t x;
    uint8_t y;
    nge_entity_id_t target;   /* ai_target[] */
    uint8_t gx;               /* ai_goal_x[], ai_goal_y[] */
    uint8_t gy;
    uint8_t sees;             /* 1: the target is in sight, at (gx, gy) */
    ai_stim_t stim;           /* what sensing, or then acting, noticed; AI_STIM_NONE */
} ai_ctx_t;

/*
 * Stimulus stim for AI entity id, caused by source (or NGE_EVENT_NONE).
 * Hurt by a source in its area: that is its target, its tile the goal,
 * tracked for AI_TRACK turns. Lost: no goal, no tracking. Then its brain's next state, and EV_AI_CHANGED if that
 * changes it
 */
void ai_stimulus(nge_entity_id_t id, ai_stim_t stim, nge_entity_id_t source) NGE_BANKED;

/*
 * Fill c for AI entity id at the start of its turn, and keep its target
 * and goal: a target no longer alive in its area is dropped. Asleep, it
 * may hear the player (AI_STIM_NOISE); awake, it looks for its target,
 * or the player if it has none (AI_STIM_SEEN: the target, and its tile
 * the goal, AI_TRACK turns of tracking). Not seen, the goal stays until
 * it is reached; then, while tracking is left, it is the target's tile,
 * one turn less each turn
 */
void ai_sense(nge_entity_id_t id, ai_ctx_t *c);

/* 1 if (x1, y1) is in sight from (x0, y0) in area: within AI_SIGHT, and no wall or closed door between */
uint8_t ai_in_sight(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/*
 * Do action (ACT_*) for a->actor if its check passes: a on the caller's
 * stack. Returns a->used: 1 if the turn was used. Refused, it changes
 * nothing and a->reason says why
 */
uint8_t ai_try(uint8_t action, act_t *a) NGE_BANKED;

/*
 * Step c->id one tile in direction d: 0 north, then clockwise to 7
 * north-west. With open, a closed door in the way is opened instead, if
 * it has AI_CAN_OPEN. 1 if the turn was used
 */
uint8_t ai_step(const ai_ctx_t *c, uint8_t d, uint8_t open);

/*
 * Step c->id one tile towards (x, y): the first tile of the line of sight
 * to it (nge_block_line()'s), then 45 degrees either side. A closed door in the way, with AI_CAN_OPEN, is
 * opened instead. 1 if the turn was used
 */
uint8_t ai_step_toward(const ai_ctx_t *c, uint8_t x, uint8_t y);

/* Wait a turn (ACT_WAIT): what an entity does when no option applies */
void ai_wait(const ai_ctx_t *c);

#endif
