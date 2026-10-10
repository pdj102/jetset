/* zxn_mmu.c: mapping pages into slots. */
#include "test.h"

#include <arch/zxn.h>

#include "zxn_mmu.h"

int main(void)
{
    uint8_t old;
    uint8_t inner;

    test_init();

    /* NEX loader defaults */
    test_regs[REG_MMU0] = ZXN_MMU_PAGE_ROM;
    test_regs[REG_MMU6] = 0;
    test_regs[REG_MMU7] = 1;

    CHECK(zxn_mmu_get(6) == 0);
    CHECK(zxn_mmu_get(0) == ZXN_MMU_PAGE_ROM);
    CHECK(ZXN_MMU_SLOT_ADDR(ZXN_MMU_SLOT_BANKED_0) == 0xC000);
    CHECK(ZXN_MMU_SLOT_ADDR(ZXN_MMU_SLOT_BANKED_1) == 0xE000);

    /* Map returns the previous page and writes the slot's register */
    old = zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, 32);
    CHECK(old == 0);
    CHECK(test_regs[REG_MMU6] == 32);
    CHECK(test_reg_log_count == 1 && test_reg_log_reg[0] == REG_MMU6);

    /* Nested maps restore in reverse order */
    inner = zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, 40);
    CHECK(inner == 32);
    zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, inner);
    CHECK(test_regs[REG_MMU6] == 32);
    zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, old);
    CHECK(test_regs[REG_MMU6] == 0);

    /* Slot 7 and the ROM slots are allowed */
    CHECK(zxn_mmu_map(ZXN_MMU_SLOT_BANKED_1, 33) == 1 && test_regs[REG_MMU7] == 33);
    CHECK(zxn_mmu_map(0, 50) == ZXN_MMU_PAGE_ROM && test_regs[REG_MMU0] == 50);
    CHECK(zxn_mmu_map(1, ZXN_MMU_PAGE_ROM) == 0 && test_regs[REG_MMU1] == ZXN_MMU_PAGE_ROM);

    /* Video, program and stack slots, and slots past 7, are refused */
    TEST_EXPECT_HALT(zxn_mmu_map(2, 32));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_mmu_map(5, 32));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_mmu_map(8, 32));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_mmu_get(8));
    CHECK(test_halt_count == 1);

    return test_summary("mmu");
}
