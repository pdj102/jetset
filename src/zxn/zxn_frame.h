#ifndef ZXN_FRAME_H
#define ZXN_FRAME_H

/**
 * @file zxn_frame.h
 * @brief Frame synchronization functions for the ZX Next.
 */

/**
 * @brief Wait until the next vertical blanking period begins.
 *
 * If called during vertical blanking, waits for that period to end and then
 * waits for the following vertical blanking period to begin.
 */
void zxn_frame_wait_vblank(void);

/**
 * @brief Wait for the next frame's vertical blanking period.
 *
 * Compatibility alias for zxn_frame_wait_vblank().
 */
void zxn_frame_wait(void);

#endif