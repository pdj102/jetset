/* nge_ecs.c: the ECS data page in slot 0 and its guard area. */
#include "test.h"

#include "ecs/nge_ecs.h"
#include "ecs/nge_entity.h"

int main(void)
{
    int i;
    int ok;

    test_init();

    /* ---- init: page 50 into slot 0, guard filled, entities free ---- */
    test_regs[0x50] = 0xFF;                                  /* the ROM */
    memset(nge_ecs_test_guard, 0, sizeof(nge_ecs_test_guard));
    nge_entity_used_count = 7;
    nge_ecs_init();
    CHECK(test_regs[0x50] == NGE_ECS_PAGE && NGE_ECS_PAGE == 50);
    ok = 1;
    for (i = 0; i < NGE_ECS_GUARD_SIZE; i++) {
        ok = ok && nge_ecs_test_guard[i] == NGE_ECS_GUARD_BYTE(i);
    }
    CHECK(ok);
    CHECK(nge_entity_used_count == 0 && nge_entity_free_count() == NGE_ENTITY_MAX);

    /* The pattern varies, so writing one value everywhere is caught */
    CHECK(NGE_ECS_GUARD_BYTE(0) != NGE_ECS_GUARD_BYTE(1));

    /* ---- check_guard ---- */
    TEST_EXPECT_HALT(nge_ecs_check_guard());
    CHECK(test_halt_count == 0);                             /* intact */

    nge_ecs_test_guard[0] = 0;                               /* *(uint8_t *)NULL = 0 */
    TEST_EXPECT_HALT(nge_ecs_check_guard());
    CHECK(test_halt_count == 1);

    nge_ecs_init();                                          /* refilled */
    nge_ecs_test_guard[NGE_ECS_GUARD_SIZE - 1] ^= 1;         /* the last byte */
    TEST_EXPECT_HALT(nge_ecs_check_guard());
    CHECK(test_halt_count == 1);

    return test_summary("ecs");
}
