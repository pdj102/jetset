/*
 * Level functions and data in common memory: the map, used every frame by
 * the map and render systems, the checks made every turn, the requests to
 * change level, and the clean-up run every frame. Setting up and building
 * a level is banked, in level_bank.c and level_gen.c, and leaving one in
 * level_travel.c.
 */
#include "game/level/level_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "game/components/feature.h"
#include "game/components/inventory.h"
#include "game/components/item.h"

uint8_t level_depth;
uint16_t level_dev_seed;
uint8_t level_request_waiting;

static uint8_t request_depth;
static uint8_t request_arrival;

uint8_t level_can_enter(nge_entity_id_t id, uint8_t x, uint8_t y, nge_entity_id_t *blocker)
{
    nge_block_t pass = NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK) ? NGE_BLOCK_PASS(id)
                                                                         : NGE_BLOCK_NONE;

    return nge_block_can_enter(LEVEL_AREA, x, y, pass, id, blocker);
}

uint8_t level_holds(nge_entity_id_t id)
{
    nge_owner_t o = NGE_OWNER_OF(id);

    nge_util_assert(id < NGE_ENTITY_MAX);
    while (NGE_OWNER_IS_ENTITY(o)) {
        o = NGE_OWNER_OF(o);   /* ownership has no cycles: this ends */
    }
    return o == LEVEL_AREA;
}

/* ---- Requests ---- */

void level_request(uint8_t depth, uint8_t arrival)
{
    nge_util_assert(arrival <= LEVEL_ARRIVE_DOWN_STAIRS);

    if (level_request_waiting) {
        return;   /* the first wins */
    }
    if (depth == 0 && arrival != LEVEL_ARRIVE_DOWN_STAIRS) {
        depth = 1;   /* only the stairs lead out */
    } else if (depth > LEVEL_DEPTH_BOTTOM) {
        depth = LEVEL_DEPTH_BOTTOM;
    }
    request_depth = depth;
    request_arrival = arrival;
    level_request_waiting = 1;
}

void level_take_request(uint8_t *depth, uint8_t *arrival)
{
    nge_util_assert(level_request_waiting);

    *depth = request_depth;
    *arrival = request_arrival;
    level_request_waiting = 0;
}

void level_clear_request(void)
{
    level_request_waiting = 0;
}

/* ---- Clean-up ---- */

void level_release_dying(void)
{
    nge_entity_id_t id;
    nge_entity_id_t owned;
    uint8_t i;
    uint8_t gone;

    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        if (FEATURE_IS(id) && NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION)) {
            level_put_floor(NGE_LOCATION_X(id), NGE_LOCATION_Y(id));   /* a feature gone: floor */
        }
        inv_drop_all(id);
        NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
            if (!ITEM_IS(owned)) {
                gone = nge_entity_destroy(owned);
                nge_util_assert(gone);   /* a persistent non-item would be left nowhere */
                (void)gone;
            }
        }
        /* The rest (items it held off the map, dying effects) pass to its owner */
        nge_owner_move_all(id, NGE_OWNER_OF(id));
        nge_entity_release(id);
    }
}
