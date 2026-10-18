/*
 * The feature component's functions: banked in page 110 with the other
 * component functions; its tables are in slot 0 (feature_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/components/feature.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"
#include "game/components/kind.h"
#include "game/gen/gen.h"

/* A feature in use, of the feature class */
static void check_feature(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(FEATURE_IS(id) && KIND_IS(id, CLASS_FEATURE));
}

/* Make a placed feature block and look as its state says */
static void update(nge_entity_id_t id)
{
    gen_feature_info_t info;   /* its kind's: on the stack, in common memory */
    nge_tile_t tile;
    uint8_t open = FEATURE_IS_OPEN(id);

    nge_util_assert(NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION));
    gen_feature_info(kind_index[id], &info);
    nge_block_add(id, open ? NGE_BLOCK_NONE : info.block_shut, NGE_BLOCK_NONE);
    tile.tile_id = info.look[open].tile_id;
    tile.tile_attr = (uint8_t)(info.look[open].ink << 4);
    nge_map_set(NGE_LOCATION_X(id), NGE_LOCATION_Y(id), &tile);
}

void feature_add(nge_entity_id_t id, uint8_t state) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    feature_state[id] = state;
    nge_entity_components[id] |= COMPONENT_FEATURE;
}

void feature_place(nge_entity_id_t id, uint8_t x, uint8_t y) NGE_BANKED
{
    check_feature(id);
    nge_util_assert(NGE_OWNER_IS_AREA(NGE_OWNER_OF(id)));   /* features belong to an area */

    nge_location_set(id, x, y);
    update(id);
}

void feature_open(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    feature_state[id] |= FEAT_OPEN;
    update(id);
}

void feature_close(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    feature_state[id] &= (uint8_t)~FEAT_OPEN;
    update(id);
}
