#include "devices/nge_cspect.h"

#ifdef NGE_CSPECT

/* fastcall passes s in HL, which is where DebugOut expects the string */
void nge_cspect_print(const char *s) __z88dk_fastcall __naked
{
    (void)s;

    __asm
        rst 0x18
        ret
    __endasm;
}

#else

/* Keeps the file non-empty when the device is disabled */
typedef int nge_cspect_disabled_t;

#endif
