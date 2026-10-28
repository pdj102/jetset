/* game/components: the AI component, a marker for now. */
#include "test.h"

#include "ecs/nge_entity.h"

#include "game/components/ai.h"

int main(void)
{
    nge_entity_id_t a;
    nge_entity_id_t b;

    test_init();
    nge_entity_init();
    a = nge_entity_create();
    b = nge_entity_create();

    /* ---- add and remove: only this entity, only this component ---- */
    CHECK(!AI_IS(a));                                       /* new: none */
    NGE_ENTITY_ADD_COMPONENT(a, COMPONENT_PC);
    ai_add(a);
    CHECK(AI_IS(a) && !AI_IS(b));
    CHECK(NGE_ENTITY_HAS_COMPONENT(a, COMPONENT_PC));       /* others kept */
    ai_add(a);                                              /* again: no change */
    CHECK(AI_IS(a));
    ai_remove(a);
    CHECK(!AI_IS(a) && NGE_ENTITY_HAS_COMPONENT(a, COMPONENT_PC));
    ai_remove(a);                                           /* without it: no change */
    CHECK(!AI_IS(a));

    /* ---- cleared with the entity ---- */
    ai_add(b);
    nge_entity_destroy(b);
    nge_entity_release(nge_entity_next_dying());
    CHECK(!AI_IS(b));

    /* ---- asserts: a free entity, no entity ---- */
    TEST_EXPECT_HALT(ai_add(b));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(ai_remove(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);

    return test_summary("ai");
}
