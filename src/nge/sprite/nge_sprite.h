#ifndef NGE_SPRITE_H
#define NGE_SPRITE_H

/**
 * @file nge_sprite.h
 * @brief ZX Next hardware sprites.
 *
 * The setters change a copy of the sprite attributes in RAM. nge_sprite_update()
 * sends the copy to the hardware; call it once per frame, straight after
 * nge_frame_wait_vblank(), so sprites never change while they are drawn.
 *
 * Coordinates: sprites are shown over the border, so (0,0) is the top-left of
 * the 320x256 display, the same origin as the 40x32 tilemap. A sprite at
 * (NGE_SPRITE_FROM_TILE(col), NGE_SPRITE_FROM_TILE(row)) lines up with tile
 * (col, row).
 *
 * Patterns are 16x16 pixels: 256 bytes at 8 bits per pixel (64 patterns), or
 * 128 bytes at 4 bits per pixel (128 patterns), in the hardware's 16K pattern
 * memory. Pixels equal to NGE_SPRITE_TRANSPARENT are transparent.
 *
 * Banking: the setup functions (init, set_transparency, the loaders, set,
 * set4, set_scale, hide_all, set_clip, clear_clip) are banked (NGE_BANKED,
 * page 40): call nge_hw_init() before them, and not while game code has its
 * own page mapped in slot 6 or 7. The per-frame functions (move, set_pattern,
 * set_pattern4, set_flags, show, update, status) are in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "core/nge_util.h"
#include "hw/nge_palette.h"

#define NGE_SPRITE_MAX          128  /**< Number of hardware sprites. */
#define NGE_SPRITE_PATTERNS     64   /**< Number of 8-bit patterns. */
#define NGE_SPRITE_PATTERNS4    128  /**< Number of 4-bit patterns. */
#define NGE_SPRITE_PATTERN_SIZE  256 /**< Bytes per 8-bit pattern. */
#define NGE_SPRITE_PATTERN4_SIZE 128 /**< Bytes per 4-bit pattern. */
#define NGE_SPRITE_TRANSPARENT  0    /**< Transparent pixel value set by nge_sprite_init(). */

/** Pixel coordinate of a tile column or row. */
#define NGE_SPRITE_FROM_TILE(t)  ((uint16_t)(t) << 3)

/* Flags for nge_sprite_set(), nge_sprite_set4() and nge_sprite_set_flags() */
#define NGE_SPRITE_MIRROR_X   0x08  /**< Mirror horizontally. */
#define NGE_SPRITE_MIRROR_Y   0x04  /**< Mirror vertically. */
#define NGE_SPRITE_ROTATE     0x02  /**< Rotate 90 degrees clockwise (before mirroring). */
/** Palette offset 0-15, added to the pattern's pixel values (high nibble). */
#define NGE_SPRITE_PALETTE(n) ((uint8_t)((n) << 4))

/* Scale values for nge_sprite_set_scale(); OR one X and one Y value */
#define NGE_SPRITE_SCALE_1    0x00  /**< Normal size. */
#define NGE_SPRITE_SCALE_X2   0x08
#define NGE_SPRITE_SCALE_X4   0x10
#define NGE_SPRITE_SCALE_X8   0x18
#define NGE_SPRITE_SCALE_Y2   0x02
#define NGE_SPRITE_SCALE_Y4   0x04
#define NGE_SPRITE_SCALE_Y8   0x06

/* nge_sprite_status() bits */
#define NGE_SPRITE_STATUS_COLLISION  0x01  /**< Non-transparent pixels of two sprites overlapped. */
#define NGE_SPRITE_STATUS_LINE_FULL  0x02  /**< Too many sprites on a line; some were not drawn. */

/**
 * @brief Hide all sprites, set the transparent pixel value and show the sprite layer.
 *
 * Turns on the sprite layer and sprites over the border, keeping the layer
 * priority set by nge_video_init(). Call before the other sprite functions.
 */
void nge_sprite_init(void) NGE_BANKED;

/**
 * @brief Set the pixel value that is transparent in all patterns.
 * @param index Pixel value; for 4-bit patterns only the low 4 bits are used.
 */
void nge_sprite_set_transparency(uint8_t index) NGE_BANKED;

/** Page argument of the loaders: the data is in common memory ($6A00-$BFFF). */
#define NGE_SPRITE_DATA_COMMON  0

/*
 * The loaders read sprite data from one of two places:
 * - page = NGE_SPRITE_DATA_COMMON: data in common memory;
 * - page = an odd page number: data assembled for slot 7 ($E000-$FFFF), for
 *   example in a SECTION PAGE_n with n odd. The loader maps the page into
 *   slot 7 for the upload and maps the previous page back. A call loads at
 *   most 8K from one page (32 8-bit patterns); use one call per page.
 * The loaders are banked, so data in slot 6 or 7 mapped by the caller is not
 * visible to them: pass its page instead.
 */

/**
 * @brief Load 8-bit patterns into the hardware pattern memory.
 * @param page Odd page holding the data, or NGE_SPRITE_DATA_COMMON.
 * @param first First pattern to replace (0-63).
 * @param data NGE_SPRITE_PATTERN_SIZE bytes per pattern.
 * @param count Number of patterns.
 */
void nge_sprite_load_patterns(uint8_t page, uint8_t first, const uint8_t *data, uint8_t count) NGE_BANKED;

/**
 * @brief Load 4-bit patterns into the hardware pattern memory.
 *
 * Two 4-bit patterns share the memory of one 8-bit pattern: 4-bit patterns
 * 2n and 2n+1 replace 8-bit pattern n.
 *
 * @param page Odd page holding the data, or NGE_SPRITE_DATA_COMMON.
 * @param first First pattern to replace (0-127).
 * @param data NGE_SPRITE_PATTERN4_SIZE bytes per pattern.
 * @param count Number of patterns.
 */
void nge_sprite_load_patterns4(uint8_t page, uint8_t first, const uint8_t *data, uint8_t count) NGE_BANKED;

/**
 * @brief Load entries of the first sprite palette.
 * @param page Odd page holding the entries, or NGE_SPRITE_DATA_COMMON.
 * @param first First palette index to write (0-255).
 * @param entries 9-bit palette entries.
 * @param count Number of entries.
 */
void nge_sprite_load_palette(uint8_t page, uint8_t first, const nge_palette_entry_9_t *entries,
                             uint16_t count) NGE_BANKED;

/**
 * @brief Show a sprite with an 8-bit pattern.
 * @param id Sprite 0-127; lower ids are drawn underneath higher ones.
 * @param x X position 0-511 (0-319 is on screen).
 * @param y Y position 0-511 (0-255 is on screen).
 * @param pattern Pattern 0-63.
 * @param flags NGE_SPRITE_MIRROR_X, NGE_SPRITE_MIRROR_Y, NGE_SPRITE_ROTATE, NGE_SPRITE_PALETTE(n).
 */
void nge_sprite_set(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags) NGE_BANKED;

/**
 * @brief Show a sprite with a 4-bit pattern.
 *
 * The pattern's pixel values 0-15 select colours from palette offset n
 * (entries n*16 to n*16+15), set with NGE_SPRITE_PALETTE(n) in flags.
 *
 * @param pattern Pattern 0-127.
 * @see nge_sprite_set() for the other parameters.
 */
void nge_sprite_set4(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags) NGE_BANKED;

/** @brief Move a sprite. */
void nge_sprite_move(uint8_t id, uint16_t x, uint16_t y);

/** @brief Change a sprite to 8-bit pattern 0-63 (for example the next animation frame). */
void nge_sprite_set_pattern(uint8_t id, uint8_t pattern);

/** @brief Change a sprite to 4-bit pattern 0-127. */
void nge_sprite_set_pattern4(uint8_t id, uint8_t pattern);

/** @brief Replace a sprite's mirror, rotate and palette flags. */
void nge_sprite_set_flags(uint8_t id, uint8_t flags);

/** @brief Set a sprite's size: NGE_SPRITE_SCALE_X* | NGE_SPRITE_SCALE_Y*. */
void nge_sprite_set_scale(uint8_t id, uint8_t scale) NGE_BANKED;

/** @brief Show (on != 0) or hide a sprite, keeping its settings. */
void nge_sprite_show(uint8_t id, uint8_t on);

/** @brief Hide every sprite. */
void nge_sprite_hide_all(void) NGE_BANKED;

/**
 * @brief Send the sprite attributes to the hardware.
 *
 * Sends only sprites 0 to the highest id used since nge_sprite_init().
 */
void nge_sprite_update(void);

/**
 * @brief Clip sprites to a window.
 *
 * Coordinates are inclusive and in hardware units: x1 and x2 in 2-pixel steps
 * (0-159 covers the 320 pixel width), y1 and y2 in pixels (0-255).
 *
 * @param r Clip window.
 */
void nge_sprite_set_clip(const nge_rect_t *r) NGE_BANKED;

/** @brief Remove the clip window: sprites show anywhere on the display. */
void nge_sprite_clear_clip(void) NGE_BANKED;

/**
 * @brief Read and clear the sprite status.
 *
 * CSpect does not emulate the collision bit (CSpect known bugs on the
 * SpecNext wiki); test collision handling on hardware.
 * @return NGE_SPRITE_STATUS_COLLISION and NGE_SPRITE_STATUS_LINE_FULL bits,
 *         set if it happened since the last call.
 */
uint8_t nge_sprite_status(void);

#endif
