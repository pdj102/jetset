/*
 * Actions in common memory: the dispatcher and the reason table. The
 * actions themselves are banked in page 112.
 */
#include "game/actions/act_internal.h"

#include "core/nge_util.h"

/* Each reason's kind: refused (0) or attempted (ACT_RF_ATTEMPTED). Read by the actions too */
const uint8_t act_reason_flags[ACT_R_COUNT] = {
    [ACT_R_NONE]             = 0,
    [ACT_R_BLOCKED]          = 0,   /* refused: nothing tried */
    [ACT_R_BLOCKED_CREATURE] = 0,
};

uint8_t act_do(uint8_t action, act_t *a, uint8_t mode)
{
    /* A switch, not a table of pointers: a pointer to a banked function doesn't switch banks */
    switch (action) {
    case ACT_MOVE:
        return act_move(a, mode);
    default:
        nge_util_abort("bad action");
        return ACT_FAIL;
    }
}

uint8_t act_attempted(uint8_t reason)
{
    nge_util_assert(reason < ACT_R_COUNT);

    return (act_reason_flags[reason] & ACT_RF_ATTEMPTED) != 0;
}
