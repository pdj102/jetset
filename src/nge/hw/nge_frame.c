#include "hw/nge_frame.h"

#include <arch/zxn.h>
#include <stdint.h>

/*
 * Line numbering starts at 0 on the first line of the 256x192 display area,
 * so line 192 is the first line below it. Lines 192 up to the end of the
 * frame (311 at 50Hz, 261 at 60Hz) are the lower border and vertical retrace,
 * which is the time to update the display without tearing.
 */
#define NGE_FRAME_VBLANK_LINE  192

/*
 * Read the 9-bit active video line: register 30 bit 0 is bit 8 and register 31
 * is the low byte. The two reads are not atomic, so the high byte is read
 * again and the read retried if it changed (the line crossed 255 -> 256 or
 * the frame wrapped back to line 0 between the reads).
 */
static uint16_t frame_line(void)
{
    uint8_t h;
    uint8_t l;

    do {
        h = ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_H) & 0x01;
        l = ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L);
    } while ((ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_H) & 0x01) != h);

    return ((uint16_t)h << 8) | l;
}

/*
 * Wait until the next vertical blanking period begins.
 *
 * The first loop waits for any current blanking period to finish. This
 * ensures that the second loop observes a new display-to-blanking transition
 * rather than returning immediately when called during an existing one.
 */
void nge_frame_wait_vblank(void)
{
    while (frame_line() >= NGE_FRAME_VBLANK_LINE)
    {
    }

    while (frame_line() < NGE_FRAME_VBLANK_LINE)
    {
    }
}

void nge_frame_wait(void)
{
    nge_frame_wait_vblank();
}
