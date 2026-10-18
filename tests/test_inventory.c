/* game/components: the item and inventory components, and moving items. */
#include "test.h"

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"

#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/gen/gen.h"

#define AREA   NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)

static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
}

/* An item of kind on the floor at (x, y) */
static nge_entity_id_t floor_item(uint8_t kind, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_item(kind, AREA);

    nge_location_set(id, x, y);
    return id;
}

static int on_floor(nge_entity_id_t id, nge_owner_t area, uint8_t x, uint8_t y)
{
    return NGE_OWNER_OF(id) == area && NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION) &&
           NGE_LOCATION_X(id) == x && NGE_LOCATION_Y(id) == y && item_slot[id] == SLOT_NONE;
}

static int stowed(nge_entity_id_t id, nge_entity_id_t holder)
{
    return NGE_OWNER_OF(id) == holder && !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION) &&
           item_slot[id] == SLOT_NONE;
}

int main(void)
{
    nge_entity_id_t player;
    nge_entity_id_t goblin;
    nge_entity_id_t rat;
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t c;
    nge_entity_id_t effect;

    test_init();

    CHECK(SLOT_COUNT <= 16 && (SLOTS_HUMANOID & SLOT_BIT(SLOT_NONE)) == 0);

    /* ---- the generators give the components from the base records ---- */
    reset();
    player = gen_creature(CR_PLAYER, AREA);
    goblin = gen_creature(CR_GOBLIN, AREA);
    rat = gen_creature(CR_RAT, AREA);
    CHECK(INV_HAS(player) && inv_capacity[player] == 20 && inv_slots[player] == SLOTS_HUMANOID);
    CHECK(INV_HAS(goblin) && inv_capacity[goblin] == 4 && inv_slots[goblin] == SLOT_BIT(SLOT_WEAPON));
    CHECK(!INV_HAS(rat) && !ITEM_IS(rat));
    a = gen_item(IT_SWORD, AREA);
    CHECK(ITEM_IS(a) && item_slot[a] == SLOT_NONE && !INV_HAS(a));

    /* ---- empty ---- */
    reset();
    player = gen_creature(CR_PLAYER, AREA);
    nge_location_set(player, 3, 4);
    CHECK(inv_count(player) == 0 && inv_next(player, NGE_ENTITY_INVALID) == NGE_ENTITY_INVALID);
    CHECK(inv_equipped(player, SLOT_WEAPON) == NGE_ENTITY_INVALID);

    /* ---- stow: owned by the holder, off the map, counted, walked in ID order ---- */
    a = floor_item(IT_SWORD, 1, 1);
    b = floor_item(IT_POTION, 2, 1);
    c = floor_item(IT_POTION, 3, 1);
    inv_stow(c, player);
    inv_stow(a, player);
    inv_stow(b, player);
    CHECK(stowed(a, player) && stowed(b, player) && stowed(c, player));
    CHECK(inv_count(player) == 3);
    CHECK(inv_next(player, NGE_ENTITY_INVALID) == a && inv_next(player, a) == b && inv_next(player, b) == c);
    CHECK(inv_next(player, c) == NGE_ENTITY_INVALID);

    /* Equipped: not counted or walked, found by slot */
    item_slot[a] = SLOT_WEAPON;   /* (no equip action yet) */
    CHECK(inv_count(player) == 2 && inv_next(player, NGE_ENTITY_INVALID) == b);
    CHECK(inv_equipped(player, SLOT_WEAPON) == a && inv_equipped(player, SLOT_HEAD) == NGE_ENTITY_INVALID);

    /* What doesn't count: non-items it owns, dying items, other holders' items */
    effect = nge_entity_create();
    nge_owner_set(effect, player);
    goblin = gen_creature(CR_GOBLIN, AREA);
    inv_stow(gen_item(IT_SWORD, AREA), goblin);
    CHECK(inv_count(player) == 2 && inv_count(goblin) == 1);
    nge_entity_destroy(c);
    CHECK(inv_count(player) == 1 && inv_next(player, b) == NGE_ENTITY_INVALID);

    /* ---- place: on the floor, owned by the area, not equipped ---- */
    inv_place(a, AREA1, 7, 8);
    CHECK(on_floor(a, AREA1, 7, 8));
    CHECK(inv_equipped(player, SLOT_WEAPON) == NGE_ENTITY_INVALID);
    inv_stow(a, player);
    CHECK(stowed(a, player) && inv_count(player) == 2);
    /* Stowing an item equipped by another holder (taking it): stowed, not equipped */
    c = gen_item(IT_SWORD, AREA);
    inv_stow(c, goblin);
    item_slot[c] = SLOT_WEAPON;
    inv_stow(c, player);
    CHECK(stowed(c, player) && inv_count(player) == 3 && inv_equipped(goblin, SLOT_WEAPON) == NGE_ENTITY_INVALID);
    nge_entity_destroy(c);

    /* ---- drop all (a holder dying): every item, stowed or equipped, on its tile ---- */
    item_slot[a] = SLOT_WEAPON;
    CHECK(inv_drop_all(player) == 2);
    CHECK(on_floor(a, AREA, 3, 4) && on_floor(b, AREA, 3, 4));
    CHECK(inv_count(player) == 0 && NGE_OWNER_OF(effect) == player);   /* the effect isn't an item */
    /* A holder off the map keeps its items */
    inv_stow(a, player);
    nge_location_remove(player);
    CHECK(inv_drop_all(player) == 0 && stowed(a, player));

    /* ---- add again: replaced ---- */
    inv_add(player, 5, 0);
    CHECK(inv_capacity[player] == 5 && inv_slots[player] == 0 && inv_count(player) == 1);

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(inv_add(player, 0, 0));                  /* no room */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(inv_stow(goblin, player));               /* not an item */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(inv_equipped(player, SLOT_NONE));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(inv_count(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);

    return test_summary("inventory");
}
