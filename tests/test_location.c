/* nge_location.c: the location component. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/components/nge_location.h"

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
    CHECK(NGE_LOCATION_PX(a) == 80 && NGE_LOCATION_PY(a) == 160);
    nge_location_set_fine(a, 7, 3);
    CHECK(NGE_LOCATION_PX(a) == 87 && NGE_LOCATION_PY(a) == 163);
    nge_location_move(a, 1, -4);                            /* carry right, borrow up */
    CHECK(NGE_LOCATION_X(a) == 11 && NGE_LOCATION_FX(a) == 0);
    CHECK(NGE_LOCATION_Y(a) == 19 && NGE_LOCATION_FY(a) == 7);
    nge_location_move(a, -9, 17);
    CHECK(NGE_LOCATION_PX(a) == 79 && NGE_LOCATION_PY(a) == 176);
    CHECK(NGE_LOCATION_X(a) == 9 && NGE_LOCATION_FX(a) == 7 && NGE_LOCATION_Y(a) == 22 && NGE_LOCATION_FY(a) == 0);
    nge_location_move(a, 0, 0);
    CHECK(NGE_LOCATION_PX(a) == 79 && NGE_LOCATION_PY(a) == 176);
    nge_location_set(a, 1, 1);                              /* set clears the fine position */
    CHECK(NGE_LOCATION_FX(a) == 0 && NGE_LOCATION_FY(a) == 0);

    TEST_EXPECT_HALT(nge_location_set_fine(a, 8, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_location_set_fine(a, 0, 8));
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

    return test_summary("location");
}
