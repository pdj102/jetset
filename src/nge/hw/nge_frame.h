#ifndef NGE_FRAME_H
#define NGE_FRAME_H

/**
 * @file nge_frame.h
 * @brief Frame synchronization functions for the ZX Next.
 */

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