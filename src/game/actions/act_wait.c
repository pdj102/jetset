/*
 * The wait action: banked in page 112 with the other actions (see act.h
 * for the pattern its steps follow; a wait has only the first two).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"

uint8_t act_wait(act_t *a, uint8_t mode) NGE_BANKED
{
    /* 1. Context, and 2. validate: an actor, which can always wait */
    nge_util_assert(a->actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(a->actor));

    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve, 4. apply: nothing changes, and nothing to tell */
    return act_succeed(a);
}
