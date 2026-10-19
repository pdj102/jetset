#ifndef NGE_FRAME_H
#define NGE_FRAME_H

/**
 * @file nge_frame.h
 * @brief Frame synchronization functions for the ZX Next.
 */

#include <stdint.h>

/** Lines in a 50Hz frame: nge_frame_line() runs 0 to NGE_FRAME_LINES-1. */
#define NGE_FRAME_LINES  312

/**
 * @brief The raster line being drawn: 0 is the first line of the 256x192
 *        display area, 192 the first below it. One line is 64 microseconds
 *        at 50Hz, so the difference between two reads times short work.
 */
uint16_t nge_frame_line(void);

/**
 * @brief Wait until the next vertical blanking period begins.
 *
 * If called during vertical blanking, waits for that period to end and then
 * waits for the following vertical blanking period to begin.
 */
void nge_frame_wait_vblank(void);

/**
 * @brief Wait for the next frame's vertical blanking period.
 *
 * Compatibility alias for nge_frame_wait_vblank().
 */
void nge_frame_wait(void);

#endif