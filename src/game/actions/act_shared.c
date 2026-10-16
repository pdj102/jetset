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

uint8_t act_fail(act_t *a, uint8_t reason)
{
    nge_util_assert(reason != ACT_R_NONE && reason < ACT_R_COUNT);

    a->reason = reason;
    a->used = (act_reason_flags[reason] & ACT_RF_ATTEMPTED) != 0;
    return ACT_FAIL;
}

uint8_t act_succeed(act_t *a)
{
    a->reason = ACT_R_NONE;
    a->used = 1;
    return ACT_SUCCESS;
}

uint8_t act_checked(act_t *a)
{
    a->reason = ACT_R_NONE;
    a->used = 0;
    return ACT_SUCCESS;
}
