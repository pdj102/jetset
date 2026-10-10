#include "zxn_cspect.h"

#ifdef ZXN_CSPECT

/* fastcall passes s in HL, which is where DebugOut expects the string */
void zxn_cspect_print(const char *s) __z88dk_fastcall __naked
{
    (void)s;

    __asm
        rst 0x18
        ret
    __endasm;
}

#else

/* Keeps the file non-empty when the device is disabled */
typedef int zxn_cspect_disabled_t;

#endif
