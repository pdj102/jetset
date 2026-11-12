/*
 * The feature component's functions: banked in page 132, as features are
 * the component with the most still to come; its tables are in slot 0
 * (feature_cdata.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_132
#pragma constseg PAGE_132
#endif

#include "game/components/feature.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"
#include "game/components/inventory.h"
#include "game/components/kind.h"
#include "game/components/stats.h"
#include "game/gen/gen.h"

typedef char feature_key_check[(KEY_COUNT <= (FEAT_KEY_MASK >> FEAT_KEY_SHIFT) + 1) ? 1 : -1];

/* A feature in use, of the feature class */
static void check_feature(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(FEATURE_IS(id) && KIND_IS(id, CLASS_FEATURE));
}

/*
 * Make a placed feature block and look as its state says (docs/features.md
 * 4.4): hidden, it blocks nothing and its tile is left as it is, its cell's
 * terrain (a feature starts hidden and is placed on drawn terrain, and only
 * finding it changes that); open, it blocks nothing; spent, it blocks as
 * when closed; either has its used look
 */
static void update(nge_entity_id_t id)
{
    gen_feature_info_t info;   /* its kind's: on the stack, in common memory */
    nge_tile_t tile;
    uint8_t state = feature_state[id];
    uint8_t open = (state & FEAT_OPEN) != 0;
    uint8_t used = (state & (FEAT_OPEN | FEAT_SPENT)) != 0;

    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION));
    if (state & FEAT_HIDDEN) {
        nge_block_add(id, NGE_BLOCK_NONE, NGE_BLOCK_NONE);
        return;
    }
    gen_feature_info(kind_index[id], &info);
    nge_block_add(id, open ? NGE_BLOCK_NONE : info.block_shut, NGE_BLOCK_NONE);
    tile.tile_id = info.look[used].tile_id;
    tile.tile_attr = (uint8_t)(info.look[used].bank << 4);
    nge_map_set(NGE_LOCATION_X(id), NGE_LOCATION_Y(id), &tile);
}

void feature_add(nge_entity_id_t id, uint8_t state) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    feature_state[id] = state;
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_FEATURE);
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

void feature_redraw(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    update(id);
}

void feature_find(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    feature_state[id] &= (uint8_t)~FEAT_HIDDEN;
    update(id);
}

void feature_spend(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    feature_state[id] |= FEAT_SPENT;
    update(id);
}

void feature_lock(nge_entity_id_t id, uint8_t key) NGE_BANKED
{
    check_feature(id);
    nge_util_assert(key != 0 && key < KEY_ANY);
    nge_util_assert(!FEATURE_IS_OPEN(id));

    feature_state[id] = (uint8_t)((feature_state[id] & (uint8_t)~FEAT_KEY_MASK) | FEAT_LOCKED |
                                  (uint8_t)(key << FEAT_KEY_SHIFT));
}

void feature_unlock(nge_entity_id_t id) NGE_BANKED
{
    check_feature(id);

    feature_state[id] &= (uint8_t)~(FEAT_LOCKED | FEAT_KEY_MASK);
}

uint8_t feature_key_fits(nge_entity_id_t id, nge_entity_id_t item) NGE_BANKED
{
    uint8_t kind = kind_index[item];
    uint8_t key;

    check_feature(id);

    if (gen_item_subclass(kind) != ITEM_KEY) {
        return 0;
    }
    key = gen_item_key_lock_group(kind);
    return key == KEY_ANY || key == (uint8_t)((feature_state[id] & FEAT_KEY_MASK) >> FEAT_KEY_SHIFT);
}

nge_entity_id_t feature_fitting_key(nge_entity_id_t id, nge_entity_id_t holder) NGE_BANKED
{
    nge_entity_id_t item = NGE_ENTITY_INVALID;

    while ((item = inv_next(holder, item)) != NGE_ENTITY_INVALID) {
        if (feature_key_fits(id, item)) {
            return item;
        }
    }
    return NGE_ENTITY_INVALID;
}

/* A d20 roll and actor's modifier for ability (ABIL_*), if it has stats */
static int16_t check_total(nge_entity_id_t actor, uint8_t ability, uint8_t roll)
{
    int16_t total = roll;

    if (NGE_ENTITY_HAS_COMPONENT(actor, COMPONENT_STATS)) {
        total += STATS_MOD(stats_ability[ability][actor]);
    }
    return total;
}

/* The checks below (Wis to find, Dex to disarm and pick) are described in the help screen (game/ui/screen_guide_pdata.c) */
uint8_t feature_found_by(nge_entity_id_t actor, nge_entity_id_t id, uint8_t roll) NGE_BANKED
{
    check_feature(id);

    return check_total(actor, ABIL_WIS, roll) >= (int16_t)gen_feature_dc(kind_index[id]);
}

int8_t feature_disarm_margin(nge_entity_id_t actor, nge_entity_id_t id, uint8_t roll) NGE_BANKED
{
    int16_t margin;

    check_feature(id);

    margin = check_total(actor, ABIL_DEX, roll) - (int16_t)gen_feature_dc(kind_index[id]);
    if (margin < -128) {
        margin = -128;
    }
    return (int8_t)margin;
}

uint8_t feature_pick_works(nge_entity_id_t actor, nge_entity_id_t pick, uint8_t roll) NGE_BANKED
{
    int16_t total = (int16_t)roll + gen_item_pick_bonus(kind_index[pick]);

    if (NGE_ENTITY_HAS_COMPONENT(actor, COMPONENT_STATS)) {
        total += STATS_MOD(stats_ability[ABIL_DEX][actor]);
    }
    return total >= FEAT_PICK_DC;
}
