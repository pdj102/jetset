/* nge_location.c: the location component. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/nge_location.h"

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
