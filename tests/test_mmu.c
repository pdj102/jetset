/* nge_mmu.c: mapping pages into slots. */
#include "test.h"

#include <arch/zxn.h>

#include "hw/nge_mmu.h"

int main(void)
{
    uint8_t old;
    uint8_t inner;

    test_init();

    /* NEX loader defaults */
    test_regs[REG_MMU0] = NGE_MMU_PAGE_ROM;
    test_regs[REG_MMU6] = 0;
    test_regs[REG_MMU7] = 1;

    CHECK(nge_mmu_get(6) == 0);
    CHECK(nge_mmu_get(0) == NGE_MMU_PAGE_ROM);
    CHECK(NGE_MMU_SLOT_ADDR(6) == 0xC000);
    CHECK(NGE_MMU_SLOT_ADDR(7) == 0xE000);

    /* Map returns the previous page and writes the slot's register */
    old = nge_mmu_map(6, 32);
    CHECK(old == 0);
    CHECK(test_regs[REG_MMU6] == 32);
    CHECK(test_reg_log_count == 1 && test_reg_log_reg[0] == REG_MMU6);

    /* Nested maps restore in reverse order */
    inner = nge_mmu_map(6, 40);
    CHECK(inner == 32);
    nge_mmu_map(6, inner);
    CHECK(test_regs[REG_MMU6] == 32);
    nge_mmu_map(6, old);
    CHECK(test_regs[REG_MMU6] == 0);

    /* Other slots, including the ROM slots */
    CHECK(nge_mmu_map(7, 33) == 1 && test_regs[REG_MMU7] == 33);
    CHECK(nge_mmu_map(0, 50) == NGE_MMU_PAGE_ROM && test_regs[REG_MMU0] == 50);
    CHECK(nge_mmu_map(1, NGE_MMU_PAGE_ROM) == 0 && test_regs[REG_MMU1] == NGE_MMU_PAGE_ROM);

    /* Slots past 7 are refused, including a register number by mistake */
    test_reg_log_count = 0;
    TEST_EXPECT_HALT(nge_mmu_map(8, 32));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_mmu_map(REG_MMU0, 32));
    CHECK(test_halt_count == 1);
    CHECK(test_reg_log_count == 0);
    TEST_EXPECT_HALT(nge_mmu_get(8));
    CHECK(test_halt_count == 1);

    return test_summary("mmu");
}
