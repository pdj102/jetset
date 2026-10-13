#include "devices/nge_cspect.h"

#ifdef NGE_CSPECT

/*
 * The DebugOut trap itself. Kept in its own file because it is the only part
 * that cannot run on the PC: the tests replace it with a fake that records
 * each string (see tests/Makefile, LIB_EXCLUDE).
 *
 * fastcall passes s in HL, which is where DebugOut expects the string.
 */
void nge_cspect_trap(const char *s) __z88dk_fastcall __naked
{
    (void)s;

    __asm
        rst 0x18
        ret
    __endasm;
}

#else

/* Keeps the file non-empty when the device is disabled */
typedef int nge_cspect_trap_disabled_t;

#endif
