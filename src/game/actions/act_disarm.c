/*
 * The search and disarm actions: banked in BANK_56 with the other actions
 * (see act.h for the pattern their steps follow, and docs/features.md 4.6).
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
#include "game/events.h"

/* A d20 roll */
static uint8_t d20(void)
{
    return (uint8_t)(1 + (unsigned int)rand() % 20);
}

uint8_t act_search(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t id;
    nge_owner_t area;
    uint8_t x0;
    uint8_t y0;
    uint8_t x;
    uint8_t y;
    uint8_t found = 0;

    /* 1, 2. Context and validation: nothing can stop a search */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3, 4. Resolve and apply: a roll for each hidden feature on the 3 x 3 tiles around the actor */
    area = NGE_OWNER_OF(actor);
    x0 = (uint8_t)(NGE_LOCATION_X(actor) - 1);
    y0 = (uint8_t)(NGE_LOCATION_Y(actor) - 1);
    for (y = y0; y != (uint8_t)(y0 + 3); y++) {
        for (x = x0; x != (uint8_t)(x0 + 3); x++) {
            NGE_LOCATION_FOR_EACH_AT(id, area, x, y, COMPONENT_FEATURE) {
                if (FEATURE_IS_HIDDEN(id) && feature_found_by(actor, id, d20())) {
                    feature_find(id);
                    nge_event_post(id, EV_FOUND, actor, 0, 0);
                    found++;
                }
            }
        }
    }
    a->amount = found;
    nge_event_post(actor, EV_SEARCHED, NGE_EVENT_NONE, found, 0);
    return act_succeed(a);   /* finding nothing isn't a failure: the turn is used */
}

uint8_t act_disarm(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t target;
    int8_t margin;
    uint8_t reason;

    /* 1, 2. Context and validation: a known trap that can be disarmed, next to the actor or on its tile */
    reason = act_find_feature(a, FCAN_DISARM, ACT_R_CANT_DISARM);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    target = a->target;
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: d20 and Dexterity against its DC */
    margin = feature_disarm_margin(actor, target, d20());

    /* 4. Apply: disarmed and gone; or held; or, failed badly, it goes off at the disarmer */
    if (margin >= 0) {
        nge_event_post(target, EV_DISARMED, actor, 0, 0);
        nge_entity_destroy(target);   /* the clean-up draws its cell's terrain again */
        return act_succeed(a);
    }
    if (margin <= FEAT_DISARM_SET_OFF) {
        act_spring(target, actor);
        return act_fail(a, ACT_R_SET_OFF);
    }
    return act_fail(a, ACT_R_DISARM_FAILED);
}
