/* nge_location.c: the location component. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"

#include <stdlib.h>

#define AREA0  NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)
#define COMP_CREATURE  NGE_COMPONENT(NGE_COMPONENT_USER)
#define COMP_ITEM      NGE_COMPONENT(NGE_COMPONENT_USER + 1)

/* The reference: a plain scan of every ID */
static nge_entity_id_t find_slow(nge_owner_t area, uint8_t x, uint8_t y,
                                 nge_component_mask_t mask, nge_entity_id_t from)
{
    int id;

    mask |= NGE_COMPONENT_LOCATION;
    for (id = from; id < NGE_ENTITY_MAX; id++) {
        if (NGE_ENTITY_IS_ALIVE(id) && NGE_ENTITY_HAS_COMPONENTS(id, mask) &&
            nge_owner_of[id] == area && nge_location_x[id] == x && nge_location_y[id] == y) {
            return (nge_entity_id_t)id;
        }
    }
    return NGE_ENTITY_INVALID;
}

/* An alive entity in area at (x, y) with extra components */
static nge_entity_id_t put(nge_owner_t area, uint8_t x, uint8_t y, nge_component_mask_t comp)
{
    nge_entity_id_t id = nge_entity_create();

    nge_owner_set(id, area);
    nge_location_set(id, x, y);
    nge_entity_set_components(id, comp);
    return id;
}

static void test_find(void)
{
    nge_entity_id_t monster;
    nge_entity_id_t potion;
    nge_entity_id_t scroll;
    nge_entity_id_t id;
    nge_entity_id_t seen[NGE_ENTITY_MAX];
    int n;
    int step;
    int ok;

    nge_entity_init();
    put(AREA0, 4, 7, 0);                                     /* same column, other row */
    put(AREA0, 5, 6, 0);                                     /* same row, other column */
    monster = put(AREA0, 5, 7, COMP_CREATURE);
    put(AREA1, 5, 7, COMP_ITEM);                             /* same tile, other area */
    potion = put(AREA0, 5, 7, COMP_ITEM);
    id = nge_entity_create();                               /* no location, x and y happen to match */
    nge_owner_set(id, AREA0);
    nge_location_x[id] = 5;
    nge_location_y[id] = 7;
    scroll = put(AREA0, 5, 7, COMP_ITEM);

    /* First match, filtered by components */
    CHECK(nge_location_find(AREA0, 5, 7, 0, 0) == monster);
    CHECK(nge_location_find(AREA0, 5, 7, COMP_CREATURE, 0) == monster);
    CHECK(nge_location_find(AREA0, 5, 7, COMP_ITEM, 0) == potion);
    CHECK(nge_location_find(AREA0, 5, 7, COMP_ITEM | COMP_CREATURE, 0) == NGE_ENTITY_INVALID);
    CHECK(nge_location_find(AREA0, 6, 7, 0, 0) == NGE_ENTITY_INVALID);  /* empty tile */
    CHECK(nge_location_find(AREA1, 5, 7, 0, 0) != NGE_ENTITY_INVALID);
    /* Continuing after a match; from past the last ID */
    CHECK(nge_location_find(AREA0, 5, 7, COMP_ITEM, (nge_entity_id_t)(potion + 1)) == scroll);
    CHECK(nge_location_find(AREA0, 5, 7, COMP_ITEM, (nge_entity_id_t)(scroll + 1)) == NGE_ENTITY_INVALID);
    CHECK(nge_location_find(AREA0, 5, 7, 0, NGE_ENTITY_MAX) == NGE_ENTITY_INVALID);
    CHECK(nge_location_find(AREA0, 5, 7, 0, NGE_ENTITY_INVALID) == NGE_ENTITY_INVALID);

    /* The loop: everything on the tile, in ID order */
    n = 0;
    NGE_LOCATION_FOR_EACH_AT(id, AREA0, 5, 7, 0) {
        seen[n++] = id;
    }
    CHECK(n == 3 && seen[0] == monster && seen[1] == potion && seen[2] == scroll);
    /* Destroying and moving away inside the loop is safe */
    n = 0;
    NGE_LOCATION_FOR_EACH_AT(id, AREA0, 5, 7, COMP_ITEM) {
        n++;
        if (id == potion) {
            nge_entity_destroy(potion);                      /* dying: no longer matches */
            nge_location_set(scroll, 9, 9);                  /* the next one moves away */
        }
    }
    CHECK(n == 1);
    CHECK(nge_location_find(AREA0, 5, 7, COMP_ITEM, 0) == NGE_ENTITY_INVALID);
    CHECK(nge_location_find(AREA0, 9, 9, COMP_ITEM, 0) == scroll);
    /* The last ID */
    nge_entity_init();
    for (n = 0; n < NGE_ENTITY_MAX; n++) {
        put(AREA0, 1, 1, 0);
    }
    nge_location_set(NGE_ENTITY_MAX - 1, 2, 2);
    CHECK(nge_location_find(AREA0, 2, 2, 0, 0) == NGE_ENTITY_MAX - 1);

    /* ---- randomised: the same answers as a plain scan ---- */
    srand(77);
    nge_entity_init();
    ok = 1;
    for (step = 0; step < 3000 && ok; step++) {
        uint8_t x = (uint8_t)(rand() % 4);
        uint8_t y = (uint8_t)(rand() % 4);
        nge_owner_t area = (rand() & 1) ? AREA0 : AREA1;
        nge_component_mask_t mask = (nge_component_mask_t)((rand() % 3 == 0) ? COMP_ITEM : 0);
        nge_entity_id_t from = (nge_entity_id_t)(rand() % (NGE_ENTITY_MAX + 2));

        switch (rand() % 6) {
        case 0:
            if (nge_entity_free_count() > 0) {
                put(area, x, y, (rand() & 1) ? COMP_ITEM : COMP_CREATURE);
            }
            break;
        case 1:
            id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
            if (NGE_ENTITY_IS_ALIVE(id)) {
                nge_location_set(id, x, y);
            }
            break;
        case 2:
            id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
            if (NGE_ENTITY_IS_ALIVE(id) && nge_owner_count(id) == 0) {
                nge_entity_destroy(id);
                nge_entity_release(nge_entity_next_dying());
            }
            break;
        case 3:
            id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
            if (NGE_ENTITY_IS_ALIVE(id)) {
                nge_location_remove(id);
            }
            break;
        default:
            break;
        }
        ok = nge_location_find(area, x, y, mask, from) == find_slow(area, x, y, mask, from);
    }
    CHECK(ok);
}

int main(void)
{
    nge_entity_id_t a;
    nge_entity_id_t b;

    test_init();

    CHECK(NGE_COMPONENT_LOCATION == NGE_COMPONENT(0));
    CHECK(NGE_COMPONENT_USER == 4);

    nge_entity_init();
    a = nge_entity_create();
    b = nge_entity_create();
    CHECK(!NGE_ENTITY_HAS_COMPONENTS(a, NGE_COMPONENT_LOCATION));   /* new: no location */

    /* ---- set: values and the component, other components kept ---- */
    nge_entity_set_components(a, NGE_COMPONENT(NGE_COMPONENT_USER));
    nge_location_set(a, 79, 47);
    CHECK(NGE_LOCATION_X(a) == 79 && NGE_LOCATION_Y(a) == 47);
    CHECK(nge_entity_components[a] == (NGE_COMPONENT_LOCATION | NGE_COMPONENT(NGE_COMPONENT_USER)));
    nge_location_set(b, 3, 4);
    CHECK(NGE_LOCATION_X(a) == 79 && NGE_LOCATION_X(b) == 3 && NGE_LOCATION_Y(b) == 4);

    /* The macros are lvalues: systems move entities directly */
    NGE_LOCATION_X(b)++;
    NGE_LOCATION_Y(b) = 10;
    CHECK(nge_location_x[b] == 4 && nge_location_y[b] == 10);

    /* ---- remove: only the component ---- */
    nge_location_remove(a);
    CHECK(nge_entity_components[a] == NGE_COMPONENT(NGE_COMPONENT_USER));
    CHECK(NGE_ENTITY_HAS_COMPONENTS(b, NGE_COMPONENT_LOCATION));

    /* ---- a dying entity may still be moved; a reused ID has no location ---- */
    nge_entity_destroy(b);
    nge_location_set(b, 1, 2);
    CHECK(NGE_LOCATION_X(b) == 1);
    nge_entity_release(nge_entity_next_dying());
    CHECK(nge_entity_create() == b && !NGE_ENTITY_HAS_COMPONENTS(b, NGE_COMPONENT_LOCATION));

    /* ---- fine position and pixel moves ---- */
    nge_entity_init();
    a = nge_entity_create();
    nge_location_set(a, 10, 20);
    CHECK(NGE_LOCATION_FX(a) == 0 && NGE_LOCATION_FY(a) == 0);
    CHECK(NGE_LOCATION_PX(a) == 160 && NGE_LOCATION_PY(a) == 320);    /* 16 pixels a tile */
    nge_location_set_fine(a, 15, 3);
    CHECK(NGE_LOCATION_PX(a) == 175 && NGE_LOCATION_PY(a) == 323);
    nge_location_move(a, 1, -4);                            /* carry right, borrow up */
    CHECK(NGE_LOCATION_X(a) == 11 && NGE_LOCATION_FX(a) == 0);
    CHECK(NGE_LOCATION_Y(a) == 19 && NGE_LOCATION_FY(a) == 15);
    nge_location_move(a, -17, 33);
    CHECK(NGE_LOCATION_PX(a) == 159 && NGE_LOCATION_PY(a) == 352);
    CHECK(NGE_LOCATION_X(a) == 9 && NGE_LOCATION_FX(a) == 15 && NGE_LOCATION_Y(a) == 22 && NGE_LOCATION_FY(a) == 0);
    nge_location_move(a, 0, 0);
    CHECK(NGE_LOCATION_PX(a) == 159 && NGE_LOCATION_PY(a) == 352);
    nge_location_set(a, 1, 1);                              /* set clears the fine position */
    CHECK(NGE_LOCATION_FX(a) == 0 && NGE_LOCATION_FY(a) == 0);

    TEST_EXPECT_HALT(nge_location_set_fine(a, 16, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_set_fine(a, 0, 16));
    CHECK(test_halt_count == 1);
    nge_location_remove(a);
    TEST_EXPECT_HALT(nge_location_move(a, 1, 0));           /* no location */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_set_fine(a, 0, 0));
    CHECK(test_halt_count == 1);
    b = nge_entity_create();

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(nge_location_set(50, 0, 0));            /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_set(NGE_ENTITY_MAX, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_remove(50));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_remove(NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);

    test_find();

    return test_summary("location");
}
