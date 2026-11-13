/*
 * The look action: banked in BANK_56 with the other actions (see act.h for
 * the pattern its steps follow, and docs/action_look.md).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "map/nge_map.h"
#include "game/ai/ai.h"
#include "game/components/stats.h"
#include "game/events.h"

uint8_t act_look(act_t *a, uint8_t mode) NGE_BANKED
{
    /* 1. Context: the fields in locals (T-ZSDCC-1) */
    nge_entity_id_t actor = a->actor;
    uint8_t x = a->x;
    uint8_t y = a->y;
    nge_owner_t area;
    uint8_t ax;
    uint8_t ay;

    /* No asserts on the actor: their text doesn't fit in BANK_56 in the debug build (act_look.md 4.5) */
    area = NGE_OWNER_OF(actor);
    ax = NGE_LOCATION_X(actor);
    ay = NGE_LOCATION_Y(actor);

    /* 2. Validate: sight as a monster's (ai_in_sight(), D3), blindness first (D5) */
    if (act_status(actor, STATUS_BLINDED)) {
        return act_fail(a, ACT_R_BLIND);
    }
    if (!nge_map_within(ax, ay, x, y, AI_SIGHT)) {
        return act_fail(a, ACT_R_OUT_OF_RANGE);
    }
    if (!nge_block_line(area, ax, ay, x, y, NGE_BLOCK_SOLID)) {
        return act_fail(a, ACT_R_NO_LINE);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll. 4. Apply: nothing changes; the messages say what is seen (D7) */
    nge_event_post(actor, EV_LOOKED, NGE_EVENT_NONE, x, y);
    return act_succeed_free(a);   /* looking uses no turn (D1) */
}
