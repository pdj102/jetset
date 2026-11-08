#ifndef NGE_VFX_H
#define NGE_VFX_H

/**
 * @file nge_vfx.h
 * @brief Visual effects: short-lived sprites that aren't entities
 *        (docs/vfx.md). A bolt flies from one map pixel to another; a
 *        burst plays a run of patterns once at a map pixel. Each can wait
 *        a delay first.
 *
 * The module owns NGE_VFX_SLOTS hardware sprites from the first one named
 * to nge_vfx_init(). Positions are map pixels, placed through the camera
 * (map/nge_map.h) each update as the render system places entities.
 *
 * An impact waits for its bolt: a burst asked for at the point a bolt in
 * flight ends at starts when that bolt arrives, or after its own delay if
 * that is later (docs/vfx.md D4). So a caller handling all the events of
 * one action at once (the shot, then the damage) gets the splash when the
 * shot lands, without tracking the shot.
 *
 * Nothing started out of view plays: a burst whose point, or a bolt whose
 * start and end, are all outside the view isn't started, and holds
 * nothing up. A full pool starts nothing either: what is shown never
 * matters to the rules.
 *
 * Typical use, once a frame:
 *
 *     nge_vfx_upload();     in the blanking period, with the other sprites
 *     ...
 *     nge_vfx_update();     with nge_render_update()
 *
 * and nge_vfx_busy() when deciding whether the world may act.
 *
 * Banking: every function is banked (NGE_BANKED, page 92, with the slots
 * and sprite records in page 93). A look passed by pointer must be in
 * common memory or on the stack.
 */

#include <stdint.h>

#include "core/nge_bank.h"

#define NGE_VFX_SLOTS  12     /**< VFX at once, each with its own hardware sprite. */
#define NGE_VFX_NONE   0xFF   /**< Not started: out of view, or no slot free. */

/*
 * Mode bits of a look, beside the sprite mode (NGE_SPRITE_PATTERN4,
 * NGE_SPRITE_SCALE_*), in bits the sprite mode doesn't use
 */
#define NGE_VFX_HOLD      0x01   /**< A burst shows its first pattern during its delay, and ends when the delay does: a stand-in. */
#define NGE_VFX_DIRECTED  0x20   /**< A bolt points its way: patterns first.. point right, first+count.. up-right; rotated and mirrored for the other 6. */

/** How a VFX looks: a run of patterns, each shown for rate frames, with sprite flags and mode. */
typedef struct nge_vfx_look {
    uint8_t first;   /**< First pattern. */
    uint8_t count;   /**< Patterns in the run, at least 1. */
    uint8_t rate;    /**< Frames each pattern shows, at least 1. */
    uint8_t flags;   /**< NGE_SPRITE_PALETTE(n), mirrors, rotate (a directed bolt adds its own). */
    uint8_t mode;    /**< NGE_SPRITE_PATTERN4, NGE_SPRITE_SCALE_*, NGE_VFX_HOLD, NGE_VFX_DIRECTED. */
} nge_vfx_look_t;

/** @brief Free every slot and hide the sprites from first_sprite on (at the next upload). */
void nge_vfx_init(uint8_t first_sprite) NGE_BANKED;

/** @brief End every VFX now: a new level, or a new game. */
void nge_vfx_clear(void) NGE_BANKED;

/**
 * @brief Play the look's patterns once at map pixel (x, y), after delay
 *        frames, or when a bolt flying to (x, y) arrives if that is later.
 * @return The slot, or NGE_VFX_NONE if out of view or no slot is free.
 */
uint8_t nge_vfx_burst(uint16_t x, uint16_t y, const nge_vfx_look_t *look, uint8_t delay) NGE_BANKED;

/**
 * @brief Fly from map pixel (x, y) to (ex, ey) at speed pixels a frame,
 *        looping the look's patterns, then disappear.
 * @return The slot, or NGE_VFX_NONE if both ends are out of view, the two
 *         are the same point, or no slot is free.
 */
uint8_t nge_vfx_bolt(uint16_t x, uint16_t y, uint16_t ex, uint16_t ey, uint8_t speed,
                     const nge_vfx_look_t *look) NGE_BANKED;

/**
 * @brief Updates after the next one until every bolt flying to (x, y) has
 *        arrived: the delay a burst there gets at least. 0 if none is.
 *        For a burst that must follow an impact (a puff after a flash).
 */
uint8_t nge_vfx_arrival(uint16_t x, uint16_t y) NGE_BANKED;

/** @brief A VFX is playing or waiting to start. */
uint8_t nge_vfx_busy(void) NGE_BANKED;

/** @brief Advance every VFX by a frame and build its sprite: once a frame, after the camera has moved. */
void nge_vfx_update(void) NGE_BANKED;

/** @brief Send the sprites to the hardware: once a frame, in the blanking period. */
void nge_vfx_upload(void) NGE_BANKED;

#endif
