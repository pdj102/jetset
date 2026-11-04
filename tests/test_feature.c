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
#include "game/components/inventory.h"
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

    /* ---- locks: a closed door locked with a kind, its look unchanged; unlocked, shut ---- */
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 7, 7);
    CHECK(!FEATURE_IS_LOCKED(door));
    feature_lock(door, KEY_BRASS);
    CHECK(FEATURE_IS_LOCKED(door) && door_is(door, 7, 7, 0));
    {
        nge_entity_id_t iron = gen_item(IT_KEY_IRON, AREA);
        nge_entity_id_t brass = gen_item(IT_KEY_BRASS, AREA);
        nge_entity_id_t skeleton = gen_item(IT_KEY_SKELETON, AREA);
        nge_entity_id_t sword = gen_item(IT_SWORD, AREA);
        nge_entity_id_t pick = gen_item(IT_LOCK_PICK, AREA);
        nge_entity_id_t fine = gen_item(IT_FINE_LOCK_PICK, AREA);
        nge_entity_id_t goblin = gen_creature(CR_GOBLIN, AREA);   /* Dex 14: +2 */
        nge_entity_id_t nobody = nge_entity_create();             /* no stats */

        CHECK(feature_key_fits(door, brass) && feature_key_fits(door, skeleton));
        CHECK(!feature_key_fits(door, iron) && !feature_key_fits(door, sword) && !feature_key_fits(door, pick));
        /* A new lock replaces the old */
        feature_lock(door, KEY_IRON);
        CHECK(feature_key_fits(door, iron) && !feature_key_fits(door, brass));
        /* The first that fits in the pack: not one on the floor, not a wrong one */
        CHECK(feature_fitting_key(door, goblin) == NGE_ENTITY_INVALID);
        inv_stow(sword, goblin);
        inv_stow(brass, goblin);
        CHECK(feature_fitting_key(door, goblin) == NGE_ENTITY_INVALID);
        inv_stow(iron, goblin);
        CHECK(feature_fitting_key(door, goblin) == iron);
        /* The pick: the roll, its bonus, and the Dexterity modifier if it has one */
        CHECK(!feature_pick_works(goblin, pick, FEAT_PICK_DC - 3) && feature_pick_works(goblin, pick, FEAT_PICK_DC - 2));
        CHECK(!feature_pick_works(goblin, fine, FEAT_PICK_DC - 6) && feature_pick_works(goblin, fine, FEAT_PICK_DC - 5));
        CHECK(!feature_pick_works(nobody, pick, FEAT_PICK_DC - 1) && feature_pick_works(nobody, pick, FEAT_PICK_DC));
    }
    feature_unlock(door);
    CHECK(!FEATURE_IS_LOCKED(door) && door_is(door, 7, 7, 0));
    feature_lock(door, KEY_IRON);                                  /* unlocking left no key behind */
    feature_unlock(door);
    CHECK(feature_state[door] == 0);
    TEST_EXPECT_HALT(feature_lock(door, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(feature_lock(door, KEY_ANY));
    CHECK(test_halt_count == 1);
    feature_open(door);
    TEST_EXPECT_HALT(feature_lock(door, KEY_IRON));                /* open */
    CHECK(test_halt_count == 1);

    /* ---- hidden: blocks nothing, its cell's tile left as it is; found, drawn and blocking ---- */
    other = gen_feature(FT_DOOR, AREA);
    feature_state[other] = FEAT_HIDDEN;                            /* as a kind that starts hidden */
    feature_place(other, 1, 8);
    CHECK(FEATURE_IS_HIDDEN(other) && NGE_MAP_TILE(1, 8).tile_id == FLOOR && NGE_BLOCK_OF(other) == NGE_BLOCK_NONE &&
          nge_block_can_enter(AREA, 1, 8, NGE_BLOCK_NONE, NGE_ENTITY_INVALID, NULL));
    feature_redraw(other);                                         /* drawn again: still the floor */
    CHECK(NGE_MAP_TILE(1, 8).tile_id == FLOOR);
    feature_find(other);
    CHECK(!FEATURE_IS_HIDDEN(other) && door_is(other, 1, 8, 0));

    /* ---- spent: the used look, still blocking as when closed; drawn again the same ---- */
    feature_spend(other);
    CHECK(FEATURE_IS_SPENT(other) && !FEATURE_IS_OPEN(other) && NGE_MAP_TILE(1, 8).tile_id == LOOK_DOOR_OPEN &&
          NGE_BLOCK_OF(other) == NGE_BLOCK_SOLID);
    NGE_MAP_TILE(1, 8).tile_id = FLOOR;
    feature_redraw(other);
    CHECK(NGE_MAP_TILE(1, 8).tile_id == LOOK_DOOR_OPEN && NGE_BLOCK_OF(other) == NGE_BLOCK_SOLID);

    /* ---- the record's new parts: none for a door ---- */
    CHECK(gen_feature_dc(FT_DOOR) == 0 && gen_feature_holds(FT_DOOR) == 0 && gen_feature_verb(FT_DOOR) == FVERB_NONE);
    CHECK(!NGE_ENTITY_HAS_COMPONENT(door, COMPONENT_HEALTH) && !NGE_ENTITY_HAS_COMPONENT(door, COMPONENT_MELEE));

    /* ---- random: a kind of the depth, never a door or stairs (rarity 0) ---- */
    for (x = 1; x <= LEVEL_DEPTH_BOTTOM; x++) {
        other = gen_feature_random(x, AREA);
        CHECK(other != NGE_ENTITY_INVALID && KIND_IS(other, CLASS_FEATURE) && NGE_OWNER_OF(other) == AREA &&
              gen_features[kind_index[other]].where.rarity != 0 && x >= gen_features[kind_index[other]].where.depth &&
              x <= gen_features[kind_index[other]].where.depth_max);
        nge_entity_destroy(other);
    }

    /* ---- breakable: a boulder has its hit points, AC and masks, and is destroyed at 0 hp ---- */
    other = gen_feature(FT_BOULDER, AREA);
    feature_place(other, 8, 1);
    CHECK(NGE_ENTITY_HAS_COMPONENT(other, COMPONENT_HEALTH) && health_hp[other] == 12 && health_ac[other] == 1 &&
          (health_resist[other] & DMG_BIT(DMG_SLASH)) && (health_immune[other] & DMG_BIT(DMG_POISON)) &&
          NGE_MAP_TILE(8, 1).tile_id == LOOK_BOULDER && NGE_BLOCK_OF(other) == NGE_BLOCK_SOLID &&
          !NGE_ENTITY_HAS_COMPONENT(other, COMPONENT_MELEE));
    health_damage(other, NGE_ENTITY_INVALID, 6, DMG_SLASH);        /* resisted: half */
    CHECK(health_hp[other] == 9 && NGE_ENTITY_IS_ALIVE(other));
    health_damage(other, NGE_ENTITY_INVALID, 9, DMG_BLUNT);
    CHECK(health_hp[other] == 0 && !NGE_ENTITY_IS_ALIVE(other));
    other = gen_feature(FT_STATUE, AREA);                          /* not breakable */
    CHECK(!NGE_ENTITY_HAS_COMPONENT(other, COMPONENT_HEALTH));

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
