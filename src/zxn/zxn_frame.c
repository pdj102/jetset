#include "zxn_frame.h"

#include <arch/zxn.h>

#define ZXN_FRAME_RETRACE_MASK 0x80

/*
 * Wait until the next vertical blanking period begins.
 *
 * The first loop waits for any current retrace to finish. This ensures that
 * the second loop observes a new active-screen-to-retrace transition rather
 * than returning immediately when called during an existing retrace.
 *
 * Register 31 is the Next active-video-line low byte. Bit 7 is set while the
 * display is in vertical retrace. ZXN_READ_REG() performs the required
 * NextReg register selection and read.
 */
void zxn_frame_wait_vblank(void)
{
    while ((ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L) & ZXN_FRAME_RETRACE_MASK) != 0)
    {
    }

    while ((ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L) & ZXN_FRAME_RETRACE_MASK) == 0)
    {
    }
}

void zxn_frame_wait(void)
{
    zxn_frame_wait_vblank();
}