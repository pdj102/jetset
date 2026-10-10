#ifndef ZXN_SPRITE_H
#define ZXN_SPRITE_H

/**
 * @file zxn_sprite.h
 * @brief ZX Next hardware sprites.
 *
 * The setters change a copy of the sprite attributes in RAM. zxn_sprite_update()
 * sends the copy to the hardware; call it once per frame, straight after
 * zxn_frame_wait_vblank(), so sprites never change while they are drawn.
 *
 * Coordinates: sprites are shown over the border, so (0,0) is the top-left of
 * the 320x256 display, the same origin as the 40x32 tilemap. A sprite at
 * (ZXN_SPRITE_FROM_TILE(col), ZXN_SPRITE_FROM_TILE(row)) lines up with tile
 * (col, row).
 *
 * Patterns are 16x16 pixels: 256 bytes at 8 bits per pixel (64 patterns), or
 * 128 bytes at 4 bits per pixel (128 patterns), in the hardware's 16K pattern
 * memory. Pixels equal to ZXN_SPRITE_TRANSPARENT are transparent.
 */

#include <stdint.h>

#include "zxn_util.h"

#define ZXN_SPRITE_MAX          128  /**< Number of hardware sprites. */
#define ZXN_SPRITE_PATTERNS     64   /**< Number of 8-bit patterns. */
#define ZXN_SPRITE_PATTERNS4    128  /**< Number of 4-bit patterns. */
#define ZXN_SPRITE_PATTERN_SIZE  256 /**< Bytes per 8-bit pattern. */
#define ZXN_SPRITE_PATTERN4_SIZE 128 /**< Bytes per 4-bit pattern. */
#define ZXN_SPRITE_TRANSPARENT  0    /**< Transparent pixel value set by zxn_sprite_init(). */

/** Pixel coordinate of a tile column or row. */
#define ZXN_SPRITE_FROM_TILE(t)  ((uint16_t)(t) << 3)

/* Flags for zxn_sprite_set(), zxn_sprite_set4() and zxn_sprite_set_flags() */
#define ZXN_SPRITE_MIRROR_X   0x08  /**< Mirror horizontally. */
#define ZXN_SPRITE_MIRROR_Y   0x04  /**< Mirror vertically. */
#define ZXN_SPRITE_ROTATE     0x02  /**< Rotate 90 degrees clockwise (before mirroring). */
/** Palette offset 0-15, added to the pattern's pixel values (high nibble). */
#define ZXN_SPRITE_PALETTE(n) ((uint8_t)((n) << 4))

/* Scale values for zxn_sprite_set_scale(); OR one X and one Y value */
#define ZXN_SPRITE_SCALE_1    0x00  /**< Normal size. */
#define ZXN_SPRITE_SCALE_X2   0x08
#define ZXN_SPRITE_SCALE_X4   0x10
#define ZXN_SPRITE_SCALE_X8   0x18
#define ZXN_SPRITE_SCALE_Y2   0x02
#define ZXN_SPRITE_SCALE_Y4   0x04
#define ZXN_SPRITE_SCALE_Y8   0x06

/* zxn_sprite_status() bits */
#define ZXN_SPRITE_STATUS_COLLISION  0x01  /**< Non-transparent pixels of two sprites overlapped. */
#define ZXN_SPRITE_STATUS_LINE_FULL  0x02  /**< Too many sprites on a line; some were not drawn. */

/**
 * @brief Hide all sprites, set the transparent pixel value and show the sprite layer.
 *
 * Turns on the sprite layer and sprites over the border, keeping the layer
 * priority set by zxn_video_init(). Call before the other sprite functions.
 */
void zxn_sprite_init(void);

/**
 * @brief Set the pixel value that is transparent in all patterns.
 * @param index Pixel value; for 4-bit patterns only the low 4 bits are used.
 */
void zxn_sprite_set_transparency(uint8_t index);

/**
 * @brief Load 8-bit patterns into the hardware pattern memory.
 * @param first First pattern to replace (0-63).
 * @param data ZXN_SPRITE_PATTERN_SIZE bytes per pattern; may be in a banked page.
 * @param count Number of patterns.
 */
void zxn_sprite_load_patterns(uint8_t first, const uint8_t *data, uint8_t count);

/**
 * @brief Load 4-bit patterns into the hardware pattern memory.
 *
 * Two 4-bit patterns share the memory of one 8-bit pattern: 4-bit patterns
 * 2n and 2n+1 replace 8-bit pattern n.
 *
 * @param first First pattern to replace (0-127).
 * @param data ZXN_SPRITE_PATTERN4_SIZE bytes per pattern; may be in a banked page.
 * @param count Number of patterns.
 */
void zxn_sprite_load_patterns4(uint8_t first, const uint8_t *data, uint8_t count);

/**
 * @brief Show a sprite with an 8-bit pattern.
 * @param id Sprite 0-127; lower ids are drawn underneath higher ones.
 * @param x X position 0-511 (0-319 is on screen).
 * @param y Y position 0-511 (0-255 is on screen).
 * @param pattern Pattern 0-63.
 * @param flags ZXN_SPRITE_MIRROR_X, ZXN_SPRITE_MIRROR_Y, ZXN_SPRITE_ROTATE, ZXN_SPRITE_PALETTE(n).
 */
void zxn_sprite_set(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags);

/**
 * @brief Show a sprite with a 4-bit pattern.
 *
 * The pattern's pixel values 0-15 select colours from palette offset n
 * (entries n*16 to n*16+15), set with ZXN_SPRITE_PALETTE(n) in flags.
 *
 * @param pattern Pattern 0-127.
 * @see zxn_sprite_set() for the other parameters.
 */
void zxn_sprite_set4(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags);

/** @brief Move a sprite. */
void zxn_sprite_move(uint8_t id, uint16_t x, uint16_t y);

/** @brief Change a sprite to 8-bit pattern 0-63 (for example the next animation frame). */
void zxn_sprite_set_pattern(uint8_t id, uint8_t pattern);

/** @brief Change a sprite to 4-bit pattern 0-127. */
void zxn_sprite_set_pattern4(uint8_t id, uint8_t pattern);

/** @brief Replace a sprite's mirror, rotate and palette flags. */
void zxn_sprite_set_flags(uint8_t id, uint8_t flags);

/** @brief Set a sprite's size: ZXN_SPRITE_SCALE_X* | ZXN_SPRITE_SCALE_Y*. */
void zxn_sprite_set_scale(uint8_t id, uint8_t scale);

/** @brief Show (on != 0) or hide a sprite, keeping its settings. */
void zxn_sprite_show(uint8_t id, uint8_t on);

/** @brief Hide every sprite. */
void zxn_sprite_hide_all(void);

/**
 * @brief Send the sprite attributes to the hardware.
 *
 * Sends only sprites 0 to the highest id used since zxn_sprite_init().
 */
void zxn_sprite_update(void);

/**
 * @brief Clip sprites to a window.
 *
 * Coordinates are inclusive and in hardware units: x1 and x2 in 2-pixel steps
 * (0-159 covers the 320 pixel width), y1 and y2 in pixels (0-255).
 *
 * @param r Clip window.
 */
void zxn_sprite_set_clip(const zxn_rect_t *r);

/** @brief Remove the clip window: sprites show anywhere on the display. */
void zxn_sprite_clear_clip(void);

/**
 * @brief Read and clear the sprite status.
 *
 * CSpect does not emulate the collision bit (CSpect known bugs on the
 * SpecNext wiki); test collision handling on hardware.
 * @return ZXN_SPRITE_STATUS_COLLISION and ZXN_SPRITE_STATUS_LINE_FULL bits,
 *         set if it happened since the last call.
 */
uint8_t zxn_sprite_status(void);

#endif
