#include "hw/nge_mmu.h"

#include <arch/zxn.h>

#include "core/nge_util.h"

#define NGE_MMU_SLOTS 8

uint8_t nge_mmu_get(uint8_t slot)
{
    nge_util_assert(slot < NGE_MMU_SLOTS);

    return ZXN_READ_REG(REG_MMU0 + slot);
}

uint8_t nge_mmu_map(uint8_t slot, uint8_t page)
{
    uint8_t old;

    /* A slot number, not a register number (REG_MMU0 is 0x50) */
    nge_util_assert(slot < NGE_MMU_SLOTS);

    old = ZXN_READ_REG(REG_MMU0 + slot);
    ZXN_WRITE_REG(REG_MMU0 + slot, page);
    return old;
}
