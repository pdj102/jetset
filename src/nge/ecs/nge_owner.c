/*
 * Ownership in common memory: the checked getter. The table is in slot 0
 * (nge_owner_cdata.c); the functions that change or search ownership are
 * banked, in nge_owner_set.c.
 */
#include "ecs/nge_owner.h"

#include "core/nge_util.h"

/* Owner values are uint8_t, with 0xFF left free like NGE_ENTITY_INVALID */
typedef char nge_owner_count_check[(NGE_OWNER_COUNT <= 0xFF) ? 1 : -1];

nge_owner_t nge_owner_get(nge_entity_id_t id)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    return nge_owner_of[id];
}
