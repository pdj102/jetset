/*
 * Ownership functions that change or search the owner table: banked in
 * page 48 with the entity lifecycle. The table is in common memory
 * (nge_owner.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_48
#pragma constseg PAGE_48
#endif

#include "ecs/nge_owner.h"

#include "core/nge_util.h"

/*
 * A valid owner that can take entities: the world, an area or an entity in
 * use. A dying entity may take entities (clean-up moves a dying entity's
 * contents to its owner, which may be dying too); nge_entity_release()
 * checks that it has passed them on.
 */
static uint8_t can_own(nge_owner_t o)
{
    if (NGE_OWNER_IS_ENTITY(o)) {
        return NGE_ENTITY_STATE(o) != NGE_ENTITY_FREE;
    }
    return o < NGE_OWNER_COUNT;
}

/*
 * 1 if o is ancestor or is owned by ancestor, directly or indirectly. The
 * chain of owners ends at the world or an area; no chain is longer than
 * NGE_ENTITY_MAX entities, since ownership has no cycles.
 */
static uint8_t is_or_under(nge_owner_t ancestor, nge_owner_t o)
{
    uint8_t n;

    for (n = 0; n <= NGE_ENTITY_MAX; n++) {
        if (o == ancestor) {
            return 1;
        }
        if (!NGE_OWNER_IS_ENTITY(o)) {
            return 0;
        }
        o = nge_owner_of[o];
    }
    nge_util_assert(0);   /* a cycle: the table is corrupt */
    return 0;
}

void nge_owner_set(nge_entity_id_t id, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(can_own(owner));
    nge_util_assert(!is_or_under(id, owner));

    nge_owner_of[id] = owner;
}

uint8_t nge_owner_owns(nge_owner_t owner, nge_entity_id_t id) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    return is_or_under(owner, nge_owner_of[id]);
}

uint8_t nge_owner_count(nge_owner_t owner) NGE_BANKED
{
    uint8_t id;
    uint8_t n = 0;

    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (nge_owner_of[id] == owner && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE) {
            n++;
        }
    }
    return n;
}

uint8_t nge_owner_move_all(nge_owner_t from, nge_owner_t to) NGE_BANKED
{
    uint8_t id;
    uint8_t n = 0;

    nge_util_assert(from < NGE_OWNER_COUNT);
    nge_util_assert(can_own(to));
    nge_util_assert(!is_or_under(from, to));

    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (nge_owner_of[id] == from && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE) {
            nge_owner_of[id] = to;
            n++;
        }
    }
    return n;
}
