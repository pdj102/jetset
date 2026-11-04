/*
 * The open, close and unlock actions: banked in BANK_56 with the other
 * actions (see act.h for the pattern their steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/feature.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/events.h"
#include "game/gen/gen.h"

/* Steps 1 and 2 for all three: a feature that opens (act_find_feature()) */
static uint8_t find_openable(act_t *a)
{
    return act_find_feature(a, FCAN_OPEN, ACT_R_CANT_OPEN);
}

uint8_t act_open(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t target;
    uint8_t reason;

    /* 1, 2. Context and validation: an openable feature next to the actor, closed, not locked */
    reason = find_openable(a);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    target = a->target;
    if (FEATURE_IS_OPEN(target)) {
        return act_fail(a, ACT_R_ALREADY_OPEN);
    }
    if (FEATURE_IS_LOCKED(target)) {
        return act_fail(a, ACT_R_LOCKED);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    feature_open(target);
    nge_event_post(target, EV_OPENED, a->actor, 0, 0);
    return act_succeed(a);
}

uint8_t act_close(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t target;
    nge_entity_id_t id;
    uint8_t reason;

    /* 1, 2. Context and validation: an openable feature next to the actor, open, nothing in the way */
    reason = find_openable(a);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    target = a->target;
    if (!FEATURE_IS_OPEN(target)) {
        return act_fail(a, ACT_R_ALREADY_CLOSED);
    }
    NGE_LOCATION_FOR_EACH_AT(id, NGE_OWNER_OF(target), NGE_LOCATION_X(target), NGE_LOCATION_Y(target), 0) {
        if (id != target) {
            return act_fail(a, ACT_R_OBSTRUCTED);   /* an item or a creature in the doorway */
        }
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply, then the event */
    feature_close(target);
    nge_event_post(target, EV_CLOSED, a->actor, 0, 0);
    return act_succeed(a);
}

uint8_t act_unlock(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    nge_entity_id_t target;
    uint8_t pick;
    uint8_t works;
    uint8_t reason;

    /* 1, 2. Context and validation: the item carried, a locked feature, a key that fits or a pick */
    nge_util_assert(item < NGE_ENTITY_MAX);
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    reason = find_openable(a);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    target = a->target;
    if (!FEATURE_IS_LOCKED(target)) {
        return act_fail(a, ACT_R_NOT_LOCKED);
    }
    pick = gen_item_subclass(kind_index[item]) == ITEM_TOOL;
    if (!pick && !feature_key_fits(target, item)) {
        return act_fail(a, ACT_R_WRONG_KEY);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: a key turns; a pick rolls, and each try uses it up */
    if (pick) {
        works = feature_pick_works(actor, item, (uint8_t)(1 + (unsigned int)rand() % 20));
        item_count[item]--;
        if (item_count[item] == 0) {
            nge_entity_destroy(item);
        }
        if (!works) {
            return act_fail(a, ACT_R_PICK_FAILED);
        }
    }

    /* 4. Apply, then the event */
    feature_unlock(target);
    nge_event_post(target, EV_UNLOCKED, actor, 0, 0);
    return act_succeed(a);
}
