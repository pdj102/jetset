/* game/components: the feature component (doors), and the feature generator. */
#include "test.h"

#include <string.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "map/nge_map.h"

#include "data/tile_data.h"
#include "game/components/feature.h"
#include "game/components/kind.h"
#include "game/gen/gen_internal.h"
#include "game/level/level.h"

#define AREA   NGE_OWNER_AREA(0)
#define FLOOR  '.'

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };   /* a view of 10 x 10 tiles */

/* The door at (x, y) looks closed or open, and blocks walkers only when closed */
static int door_is(nge_entity_id_t id, uint8_t x, uint8_t y, int open)
{
    nge_entity_id_t blocker;
    uint8_t enter = nge_block_can_enter(AREA, x, y, NGE_BLOCK_NONE, NGE_ENTITY_INVALID, &blocker);

    return NGE_LOCATION_X(id) == x && NGE_LOCATION_Y(id) == y && FEATURE_IS_OPEN(id) == open &&
           NGE_MAP_TILE(x, y).tile_id == (open ? LOOK_DOOR_OPEN : LOOK_DOOR_SHUT) &&
           NGE_MAP_TILE(x, y).tile_attr ==
               (uint8_t)((open ? LOOK_DOOR_OPEN_BANK : LOOK_DOOR_SHUT_BANK) << 4) &&
           NGE_BLOCK_OF(id) == (open ? NGE_BLOCK_NONE : NGE_BLOCK_SOLID) &&
           (open ? enter == 1 : (enter == 0 && blocker == id));
}

int main(void)
{
    gen_feature_info_t info;
    nge_entity_id_t door;
    nge_entity_id_t other;
    char name[20];
    uint8_t x;
    uint8_t y;

    test_init();

    nge_map_init(10, 10, &window);
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block(FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);   /* as the level sets them */
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    nge_entity_init();

    /* ---- the record: a door's looks, what it can do, what it blocks ---- */
    gen_feature_info(FT_DOOR, &info);
    CHECK(memcmp(&info, &gen_features[FT_DOOR].info, sizeof(info)) == 0);
    CHECK(info.can == FCAN_OPEN && info.block_shut == NGE_BLOCK_SOLID &&
          info.look[0].tile_id == LOOK_DOOR_SHUT && info.look[1].tile_id == LOOK_DOOR_OPEN);

    /* ---- generated: a feature of its kind, closed, owned, not placed or drawn ---- */
    door = gen_feature(FT_DOOR, AREA);
    CHECK(door != NGE_ENTITY_INVALID && FEATURE_IS(door) && KIND_IS(door, CLASS_FEATURE) &&
          kind_index[door] == FT_DOOR && NGE_OWNER_OF(door) == AREA && !FEATURE_IS_OPEN(door));
    CHECK(!NGE_ENTITY_HAS_COMPONENT(door, NGE_COMPONENT_LOCATION) && !NGE_ENTITY_HAS_COMPONENT(door, NGE_COMPONENT_BLOCK) &&
          !NGE_ENTITY_HAS_COMPONENT(door, NGE_COMPONENT_SPRITE));
    gen_name(door, name, sizeof(name));
    CHECK_STR(name, "door");
    CHECK(GEN_HOOK_EF(gen_hook_of(door, GEN_HOOK_USE)) == EF_NONE && GEN_HOOK_EF(gen_feature_hook(FT_DOOR, GEN_HOOK_HIT)) == EF_NONE);

    /* ---- placed: located, drawn closed, blocking ---- */
    feature_place(door, 3, 4);
    CHECK(door_is(door, 3, 4, 0));

    /* ---- opened and closed: the state, the tile and the blocking together ---- */
    feature_open(door);
    CHECK(door_is(door, 3, 4, 1));
    feature_open(door);                                            /* again: no change */
    CHECK(door_is(door, 3, 4, 1));
    feature_close(door);
    CHECK(door_is(door, 3, 4, 0));
    feature_close(door);
    CHECK(door_is(door, 3, 4, 0));
    /* Another door's state is its own */
    other = gen_feature(FT_DOOR, AREA);
    feature_place(other, 5, 4);
    feature_open(other);
    CHECK(door_is(other, 5, 4, 1) && door_is(door, 3, 4, 0));
    CHECK(NGE_MAP_TILE(4, 4).tile_id == FLOOR);                   /* nothing drawn between them */

    /* ---- added in a state: open ---- */
    other = nge_entity_create();
    feature_add(other, FEAT_OPEN);
    CHECK(FEATURE_IS(other) && FEATURE_IS_OPEN(other));

    /* ---- asserts ---- */
    other = gen_feature(FT_DOOR, NGE_OWNER_WORLD);                 /* not in an area */
    TEST_EXPECT_HALT(feature_place(other, 1, 1));
    CHECK(test_halt_count == 1);
    other = gen_feature(FT_DOOR, AREA);                            /* not placed */
    TEST_EXPECT_HALT(feature_open(other));
    CHECK(test_halt_count == 1);
    other = nge_entity_create();                                   /* not a feature */
    nge_owner_set(other, AREA);
    TEST_EXPECT_HALT(feature_place(other, 1, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_feature(FT_COUNT, AREA));
    CHECK(test_halt_count == 1);

    return test_summary("feature");
}
