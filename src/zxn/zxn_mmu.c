#include "zxn_mmu.h"

#include <arch/zxn.h>

#include "zxn_util.h"

#define ZXN_MMU_SLOTS 8

uint8_t zxn_mmu_get(uint8_t slot)
{
    zxn_util_assert(slot < ZXN_MMU_SLOTS);

    return ZXN_READ_REG(REG_MMU0 + slot);
}

uint8_t zxn_mmu_map(uint8_t slot, uint8_t page)
{
    uint8_t old;

    old = ZXN_READ_REG(REG_MMU0 + slot);
    ZXN_WRITE_REG(REG_MMU0 + slot, page);
    return old;
}
