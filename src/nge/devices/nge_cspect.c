#include "devices/nge_cspect.h"

#ifdef NGE_CSPECT

#include <arch/zxn.h>
#include <stdint.h>

/* MMU page number that maps the ROM into slot 0 */
#define NGE_CSPECT_ROM_PAGE  0xFF

/*
 * CSpect only prints when the ROM is in slot 0: with RAM there, rst $18
 * jumps to $0018 like a normal Z80 (tested). Game data may be in slot 0, so
 * map the ROM in just for the trap. This is safe because interrupts are
 * disabled (nge_hw_init), the string is never in slot 0 (it is in common
 * memory: the debug stream's line buffer or string literals), and this code
 * is in common memory.
 */
void nge_cspect_print(const char *s) __z88dk_fastcall
{
    uint8_t slot0 = ZXN_READ_REG(REG_MMU0);

    ZXN_WRITE_REG(REG_MMU0, NGE_CSPECT_ROM_PAGE);
    nge_cspect_trap(s);
    ZXN_WRITE_REG(REG_MMU0, slot0);
}

#else

/* Keeps the file non-empty when the device is disabled */
typedef int nge_cspect_disabled_t;

#endif
