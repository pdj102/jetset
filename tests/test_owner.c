/* nge_owner.c, nge_owner_set.c: ownership, and its use by the entity lifecycle. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

#define AREA0  NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)
#define AREA_LAST  NGE_OWNER_AREA(NGE_OWNER_AREA_MAX - 1)

/* Release every dying entity, moving what it owns to its own owner */
static void clean_up(void)
{
    nge_entity_id_t id;

    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        nge_owner_move_all(id, NGE_OWNER_OF(id));
        nge_entity_release(id);
    }
}

int main(void)
{
    nge_entity_id_t player;
    nge_entity_id_t bag;
    nge_entity_id_t key;
    nge_entity_id_t chest;
    nge_entity_id_t monster;
    nge_entity_id_t id;
    uint8_t i;
    uint8_t visits[NGE_ENTITY_MAX];
    int n;

    test_init();

    /* ---- owner values ---- */
    CHECK(NGE_OWNER_COUNT <= 0xFF);
    CHECK(NGE_OWNER_IS_ENTITY(0) && NGE_OWNER_IS_ENTITY(NGE_ENTITY_MAX - 1));
    CHECK(!NGE_OWNER_IS_ENTITY(NGE_OWNER_WORLD) && !NGE_OWNER_IS_AREA(NGE_OWNER_WORLD));
    CHECK(NGE_OWNER_IS_AREA(AREA0) && NGE_OWNER_IS_AREA(AREA_LAST));
    CHECK(!NGE_OWNER_IS_AREA(AREA_LAST + 1) && !NGE_OWNER_IS_AREA(NGE_ENTITY_MAX - 1));
    CHECK(NGE_OWNER_AREA_OF(AREA0) == 0 && NGE_OWNER_AREA_OF(AREA_LAST) == NGE_OWNER_AREA_MAX - 1);

    /* ---- new entities are owned by the world ---- */
    nge_entity_init();
    CHECK(nge_owner_of[0] == NGE_OWNER_WORLD && nge_owner_of[NGE_ENTITY_MAX - 1] == NGE_OWNER_WORLD);
    player = nge_entity_create();
    bag = nge_entity_create();
    key = nge_entity_create();
    chest = nge_entity_create();
    monster = nge_entity_create();
    CHECK(NGE_OWNER_OF(player) == NGE_OWNER_WORLD && nge_owner_get(key) == NGE_OWNER_WORLD);
    CHECK(nge_owner_count(NGE_OWNER_WORLD) == 5);

    /* ---- set: areas and entities ---- */
    nge_owner_set(player, AREA0);
    nge_owner_set(monster, AREA0);
    nge_owner_set(chest, AREA1);
    nge_owner_set(bag, player);
    nge_owner_set(key, bag);
    CHECK(NGE_OWNER_OF(player) == AREA0 && NGE_OWNER_OF(chest) == AREA1);
    CHECK(NGE_OWNER_OF(bag) == player && nge_owner_get(key) == bag);
    CHECK(nge_owner_count(AREA0) == 2 && nge_owner_count(AREA1) == 1);
    CHECK(nge_owner_count(player) == 1 && nge_owner_count(bag) == 1);
    CHECK(nge_owner_count(key) == 0 && nge_owner_count(NGE_OWNER_WORLD) == 0);
    nge_owner_set(monster, AREA_LAST);                       /* last area */
    CHECK(NGE_OWNER_OF(monster) == AREA_LAST);
    nge_owner_set(monster, AREA0);

    /* ---- owns: directly and through other entities ---- */
    CHECK(nge_owner_owns(bag, key) && nge_owner_owns(player, key));
    CHECK(nge_owner_owns(AREA0, key) && nge_owner_owns(AREA0, player));
    CHECK(!nge_owner_owns(AREA1, key) && !nge_owner_owns(NGE_OWNER_WORLD, key));
    CHECK(!nge_owner_owns(key, key) && !nge_owner_owns(key, bag));
    CHECK(!nge_owner_owns(monster, key) && !nge_owner_owns(chest, key));

    /* ---- FOR_EACH_OWNED: direct, alive only ---- */
    memset(visits, 0, sizeof(visits));
    n = 0;
    NGE_OWNER_FOR_EACH_OWNED(i, id, AREA0) {
        visits[id]++;
        n++;
    }
    CHECK(n == 2 && visits[player] == 1 && visits[monster] == 1);
    n = 0;
    NGE_OWNER_FOR_EACH_OWNED(i, id, chest) {
        n++;
    }
    CHECK(n == 0);
    /* An if/else after the loop body is not captured by the macro */
    n = 0;
    if (1)
        NGE_OWNER_FOR_EACH_OWNED(i, id, player) n++;
    else
        n = 100;
    CHECK(n == 1);

    /* ---- moving: re-parenting keeps what an entity owns ---- */
    nge_owner_set(bag, chest);                               /* player drops the bag in the chest */
    CHECK(nge_owner_count(player) == 0 && nge_owner_count(chest) == 1);
    CHECK(nge_owner_owns(chest, key) && nge_owner_owns(AREA1, key) && !nge_owner_owns(player, key));

    /* ---- cycles are refused ---- */
    TEST_EXPECT_HALT(nge_owner_set(chest, chest));           /* itself */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(chest, key));             /* something it owns, indirectly */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(bag, key));               /* something it owns, directly */
    CHECK(test_halt_count == 1);
    CHECK(NGE_OWNER_OF(chest) == AREA1 && NGE_OWNER_OF(bag) == chest);

    /* ---- invalid owners and entities ---- */
    TEST_EXPECT_HALT(nge_owner_set(key, NGE_OWNER_COUNT));   /* past the last area */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(key, NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(key, 50));                /* a free entity */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(50, AREA0));              /* a free entity */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_set(NGE_ENTITY_MAX, AREA0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_get(50));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_owns(AREA0, 50));
    CHECK(test_halt_count == 1);
    CHECK(NGE_OWNER_OF(key) == bag);

    /* ---- move_all ---- */
    CHECK(nge_owner_move_all(AREA0, AREA1) == 2);            /* player and monster change area */
    CHECK(nge_owner_count(AREA0) == 0 && nge_owner_count(AREA1) == 3);
    CHECK(nge_owner_move_all(AREA0, AREA1) == 0);            /* nothing left */
    CHECK(nge_owner_move_all(chest, player) == 1);           /* the player takes the bag */
    CHECK(NGE_OWNER_OF(bag) == player && nge_owner_owns(player, key));
    TEST_EXPECT_HALT(nge_owner_move_all(player, key));       /* key is under player */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_move_all(AREA1, chest));      /* chest is in AREA1 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_move_all(bag, bag));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_move_all(NGE_OWNER_COUNT, AREA0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_owner_move_all(AREA0, 50));         /* to a free entity */
    CHECK(test_halt_count == 1);
    CHECK(nge_owner_count(AREA1) == 3 && nge_owner_count(bag) == 1);

    /* ---- release: an entity must own nothing ---- */
    nge_entity_destroy(bag);
    CHECK(nge_entity_next_dying() == bag);
    TEST_EXPECT_HALT(nge_entity_release(bag));               /* still owns the key */
    CHECK(test_halt_count == 1);
    CHECK(nge_owner_move_all(bag, NGE_OWNER_OF(bag)) == 1);  /* the key drops to the player */
    nge_entity_release(bag);
    CHECK(NGE_OWNER_OF(key) == player && NGE_ENTITY_STATE(bag) == NGE_ENTITY_FREE);
    CHECK(nge_owner_count(bag) == 0);                        /* free entities are not counted */

    /* Dying entities are still owned and counted: releasing the owner first is caught */
    nge_entity_destroy(player);
    nge_entity_destroy(key);
    CHECK(nge_owner_count(player) == 1);                     /* the dying key */
    CHECK(nge_entity_next_dying() == key);
    CHECK(nge_entity_next_dying() == player);
    TEST_EXPECT_HALT(nge_entity_release(player));
    CHECK(test_halt_count == 1);
    nge_owner_set(key, NGE_OWNER_WORLD);                     /* a dying entity may move */
    nge_entity_release(player);
    nge_entity_release(key);

    /* A reused ID starts in the world, owning nothing */
    id = nge_entity_create();
    CHECK(id == key && NGE_OWNER_OF(id) == NGE_OWNER_WORLD && nge_owner_count(id) == 0);

    /* ---- clean-up that drops contents into the owner's owner ---- */
    nge_entity_init();
    chest = nge_entity_create();
    nge_owner_set(chest, AREA0);
    for (n = 0; n < 5; n++) {
        nge_owner_set(nge_entity_create(), chest);
    }
    nge_entity_destroy(chest);
    clean_up();
    CHECK(nge_owner_count(AREA0) == 5 && nge_entity_alive_count == 5);

    /* A chain: destroying every link leaves the last in the area, whatever the order */
    nge_entity_init();
    id = AREA0;
    for (n = 0; n < NGE_ENTITY_MAX; n++) {                   /* each owns the next: depth MAX */
        nge_entity_id_t e = nge_entity_create();
        nge_owner_set(e, id);
        id = e;
    }
    CHECK(nge_owner_owns(AREA0, id) && nge_owner_owns(0, id));
    for (n = 0; n < NGE_ENTITY_MAX - 1; n++) {
        nge_entity_destroy((nge_entity_id_t)n);              /* owners first: released last */
    }
    clean_up();
    CHECK(nge_entity_alive_count == 1 && NGE_OWNER_OF(NGE_ENTITY_MAX - 1) == AREA0);

    nge_entity_init();
    id = AREA0;
    for (n = 0; n < NGE_ENTITY_MAX; n++) {
        nge_entity_id_t e = nge_entity_create();
        nge_owner_set(e, id);
        id = e;
    }
    for (n = NGE_ENTITY_MAX - 2; n >= 0; n--) {
        nge_entity_destroy((nge_entity_id_t)n);              /* owners last: released first */
    }
    clean_up();
    CHECK(nge_entity_alive_count == 1 && NGE_OWNER_OF(NGE_ENTITY_MAX - 1) == AREA0);

    return test_summary("owner");
}
