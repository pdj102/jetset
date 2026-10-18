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
    [ACT_R_BLOCKED_ENTITY] = 0,
    [ACT_R_NO_INVENTORY]     = 0,
    [ACT_R_NOTHING_HERE]     = 0,
    [ACT_R_NOT_ITEM]         = 0,
    [ACT_R_FULL]             = 0,
    [ACT_R_NOT_CARRIED]      = 0,
    [ACT_R_EQUIPPED]         = 0,
    [ACT_R_ALREADY_EQUIPPED] = 0,
    [ACT_R_CANT_EQUIP]       = 0,
    [ACT_R_NO_SLOT]          = 0,
    [ACT_R_SLOT_TAKEN]       = 0,
    [ACT_R_NOT_EQUIPPED]     = 0,
    [ACT_R_NO_ATTACK]        = 0,
    [ACT_R_NO_TARGET]        = 0,
    [ACT_R_CANT_DAMAGE]      = 0,
    [ACT_R_OUT_OF_RANGE]     = 0,
    [ACT_R_MISSED]           = ACT_RF_ATTEMPTED,   /* the attack was made: the turn is used */
    [ACT_R_CANT_USE]         = 0,
    [ACT_R_NOTHING_THERE]    = 0,
    [ACT_R_CANT_OPEN]        = 0,
    [ACT_R_ALREADY_OPEN]     = 0,
    [ACT_R_ALREADY_CLOSED]   = 0,
    [ACT_R_OBSTRUCTED]       = 0,
    [ACT_R_NO_STAIRS]        = 0,
};

uint8_t act_do(uint8_t action, act_t *a, uint8_t mode)
{
    /* A switch, not a table of pointers: a pointer to a banked function doesn't switch banks */
    switch (action) {
    case ACT_MOVE:
        return act_move(a, mode);
    case ACT_PICKUP:
        return act_pickup(a, mode);
    case ACT_DROP:
        return act_drop(a, mode);
    case ACT_EQUIP:
        return act_equip(a, mode);
    case ACT_UNEQUIP:
        return act_unequip(a, mode);
    case ACT_MELEE:
        return act_melee(a, mode);
    case ACT_USE:
        return act_use(a, mode);
    case ACT_OPEN:
        return act_open(a, mode);
    case ACT_CLOSE:
        return act_close(a, mode);
    case ACT_ASCEND:
        return act_ascend(a, mode);
    case ACT_DESCEND:
        return act_descend(a, mode);
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
