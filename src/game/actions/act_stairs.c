/*
 * The ascend and descend actions: banked in BANK_56 with the other
 * actions (see act.h for the pattern their steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "game/components/feature.h"
#include "game/components/kind.h"
#include "game/components/pc.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/quest/quest.h"

/*
 * Both: stairs that can (FCAN_ASCEND or FCAN_DESCEND) on the actor's tile,
 * then the level step levels down (-1: up). A known trapdoor leads down
 * too, arriving anywhere (docs/features.md, step 7)
 */
static uint8_t take_stairs(act_t *a, uint8_t mode, uint8_t can, int8_t step)
{
    gen_feature_info_t info;
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t id;
    nge_entity_id_t stairs = NGE_ENTITY_INVALID;
    uint8_t fall = 0;
    uint8_t depth;

    /* 1. Context: the stairs here */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(NGE_OWNER_OF(actor) == LEVEL_AREA);

    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, NGE_LOCATION_X(actor), NGE_LOCATION_Y(actor), COMPONENT_FEATURE) {
        gen_feature_info(kind_index[id], &info);
        if ((info.can & can) && !FEATURE_IS_HIDDEN(id)) {   /* a hidden trapdoor isn't known to be there */
            stairs = id;
            fall = (info.can & FCAN_TRAP) != 0;   /* a trapdoor: no stairs at the bottom, so anywhere */
            break;
        }
    }

    /* 2. Validate */
    if (stairs == NGE_ENTITY_INVALID) {
        return act_fail(a, ACT_R_NO_STAIRS);
    }
    a->target = stairs;
    depth = (uint8_t)(level_depth + step);
    if (depth == LEVEL_DEPTH_SURFACE && (!PC_IS(actor) || !quest_carried(actor))) {
        /* Only the player leaves, and only with it (docs/winning.md): described in the help screen (game/ui/screen_guide_pdata.c) */
        return act_fail(a, ACT_R_NO_QUEST);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll, but a rooted actor only struggles */
    if (act_rooted(actor)) {
        return act_fail(a, ACT_R_ROOTED);
    }

    /* 4. Apply: the player at the end of the frame, anyone else now */
    if (PC_IS(actor)) {
        level_request(depth, fall ? LEVEL_ARRIVE_ANY : (step > 0 ? LEVEL_ARRIVE_UP_STAIRS : LEVEL_ARRIVE_DOWN_STAIRS));
    } else {
        level_send(actor, depth);
    }
    return act_succeed(a);
}

uint8_t act_ascend(act_t *a, uint8_t mode) NGE_BANKED
{
    return take_stairs(a, mode, FCAN_ASCEND, -1);
}

uint8_t act_descend(act_t *a, uint8_t mode) NGE_BANKED
{
    return take_stairs(a, mode, FCAN_DESCEND, 1);
}
