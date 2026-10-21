/*
 * The move action: banked in page 112 with the other actions (see act.h
 * for the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg PAGE_112
#pragma constseg PAGE_112
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "event/nge_event.h"
#include "game/events.h"

uint8_t act_move(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_owner_t area;
    nge_block_t pass;
    nge_entity_id_t blocker;
    nge_entity_id_t id;
    uint8_t x;
    uint8_t y;

    /* 1. Context: the actor's area, the tile it moves to, what it can enter */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENTS(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(a->dx >= -1 && a->dx <= 1 && a->dy >= -1 && a->dy <= 1 && (a->dx | a->dy) != 0);

    area = NGE_OWNER_OF(actor);
    x = (uint8_t)(NGE_LOCATION_X(actor) + a->dx);
    y = (uint8_t)(NGE_LOCATION_Y(actor) + a->dy);
    pass = NGE_ENTITY_HAS_COMPONENTS(actor, NGE_COMPONENT_BLOCK) ? NGE_BLOCK_PASS(actor) : NGE_BLOCK_NONE;

    /* 2. Validate: the tile can be entered */
    a->target = NGE_ENTITY_INVALID;
    if (!nge_block_can_enter(area, x, y, pass, actor, &blocker)) {
        if (blocker != NGE_ENTITY_INVALID) {
            a->target = blocker;
            return act_fail(a, ACT_R_BLOCKED_ENTITY);
        }
        return act_fail(a, ACT_R_BLOCKED);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll for a move */

    /* 4. Apply, then the events: what happened, for the systems to react to */
    nge_location_set(actor, x, y);
    nge_event_post(actor, EV_MOVED, NGE_EVENT_NONE, (uint8_t)a->dx, (uint8_t)a->dy);
    NGE_LOCATION_FOR_EACH_AT(id, area, x, y, 0) {
        if (id != actor) {
            nge_event_post(id, EV_STOOD_ON, actor, 0, 0);
        }
    }
    return act_succeed(a);
}
