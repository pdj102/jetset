/*
 * The brains: constant data in page 130, read by the AI's code there.
 * Each table is written out in full; an entry left out is AI_STATE_NONE,
 * no change.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

const ai_brain_def_t ai_brains[AI_BRAIN_COUNT] = {
    [AI_BRAIN_NONE] = {                 /* never an entity's: ai_add() refuses it */
        .next = {
            [AI_STATE_ASLEEP] = { [AI_STIM_NONE] = AI_STATE_NONE, [AI_STIM_HURT] = AI_STATE_NONE },
            [AI_STATE_IDLE]   = { [AI_STIM_NONE] = AI_STATE_NONE, [AI_STIM_HURT] = AI_STATE_NONE },
        },
    },
    [AI_BRAIN_DEFAULT] = {
        .next = {
            [AI_STATE_ASLEEP] = { [AI_STIM_NONE] = AI_STATE_NONE, [AI_STIM_HURT] = AI_STATE_IDLE },
            [AI_STATE_IDLE]   = { [AI_STIM_NONE] = AI_STATE_NONE, [AI_STIM_HURT] = AI_STATE_NONE },
        },
    },
};
