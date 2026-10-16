/*
 * Helpers shared by the actions: in page 112 with them, not banked, so
 * only code in page 112 may call them (directly, with no bank switch).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"

uint8_t act_fail(act_t *a, uint8_t reason, uint8_t full_cost)
{
    nge_util_assert(reason != ACT_R_NONE && reason < ACT_R_COUNT);

    a->reason = reason;
    a->cost = (act_reason_flags[reason] & ACT_RF_ATTEMPTED) ? full_cost : 0;
    return ACT_FAIL;
}

uint8_t act_succeed(act_t *a, uint8_t cost)
{
    a->reason = ACT_R_NONE;
    a->cost = cost;
    return ACT_SUCCESS;
}
