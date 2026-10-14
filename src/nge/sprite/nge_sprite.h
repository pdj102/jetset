#ifndef NGE_SPRITE_H
#define NGE_SPRITE_H

/**
 * @file nge_sprite.h
 * @brief ZX Next hardware sprites.
 *
 * A sprite is described by a record (nge_sprite_attr_t, 5 bytes): its
 * position, pattern, flags, scale and visibility, in the order the hardware
 * takes them. The caller owns the records, in common memory or in a banked
 * caller's own page (the render system, ecs/systems/nge_render.h, keeps its
 * records in its page data), and changes them with the record functions,
 * which take a pointer to a record:
 *
 *     static nge_sprite_attr_t hud[4];             // hardware sprites 112-115
 *
 *     nge_sprite_clear(hud, 4);                    // all hidden
 *     nge_sprite_set(&hud[1], 10, 20, 1, 0, 0);    // visible, pattern 1
 *     ...
 *     nge_frame_wait_vblank();
 *     nge_sprite_upload(112, hud, 4);              // records to sprites 112-115
 *
 * Sprite IDs appear only in nge_sprite_upload(), which sends a run of
 * records to a run of hardware sprites. Callers can share the 128 hardware
 * sprites as long as each ID has one owner; the game manages the split.
 * With the default layer priority, higher IDs are drawn on top.
 *
 * Upload once per frame, straight after nge_frame_wait_vblank(), so sprites
 * never change while they are drawn.
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
 * Banking: the record functions, nge_sprite_upload() and
 * nge_sprite_status() are in common memory, so a banked caller can use them
 * on records in its own page. The hardware set-up functions, which never
 * touch records (init, hide_all, set_transparency, the loaders, set_clip,
 * clear_clip), are banked (NGE_BANKED, page 42). 
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

/* Flags for nge_sprite_set() and nge_sprite_set_flags() */
#define NGE_SPRITE_MIRROR_X   0x08  /**< Mirror horizontally. */
#define NGE_SPRITE_MIRROR_Y   0x04  /**< Mirror vertically. */
#define NGE_SPRITE_ROTATE     0x02  /**< Rotate 90 degrees clockwise (before mirroring). */
/** Palette offset 0-15, added to the pattern's pixel values (high nibble). */
#define NGE_SPRITE_PALETTE(n) ((uint8_t)((n) << 4))

/* Scale values for nge_sprite_set() mode and nge_sprite_set_scale(); OR one X and one Y value */
#define NGE_SPRITE_SCALE_1    0x00  /**< Normal size. */
#define NGE_SPRITE_SCALE_X2   0x08
#define NGE_SPRITE_SCALE_X4   0x10
#define NGE_SPRITE_SCALE_X8   0x18
#define NGE_SPRITE_SCALE_Y2   0x02
#define NGE_SPRITE_SCALE_Y4   0x04
#define NGE_SPRITE_SCALE_Y8   0x06

/**
 * Mode bit for nge_sprite_set(): the pattern is a 4-bit pattern (0-127),
 * whose pixel values 0-15 select colours from palette offset n (entries
 * n*16 to n*16+15), set with NGE_SPRITE_PALETTE(n) in the flags.
 */
#define NGE_SPRITE_PATTERN4   0x80

/* nge_sprite_status() bits */
#define NGE_SPRITE_STATUS_COLLISION  0x01  /**< Non-transparent pixels of two sprites overlapped. */
#define NGE_SPRITE_STATUS_LINE_FULL  0x02  /**< Too many sprites on a line; some were not drawn. */

/** One hardware sprite's attributes, in the order the hardware takes them (5 bytes). */
typedef struct nge_sprite_attr {
    uint8_t x;       /**< X bits 0-7. */
    uint8_t y;       /**< Y bits 0-7. */
    uint8_t attr2;   /**< Palette offset, mirror, rotate, X bit 8. */
    uint8_t attr3;   /**< Visible, 5-byte flag, pattern. */
    uint8_t attr4;   /**< 4-bit pattern, N6, scale, Y bit 8. */
} nge_sprite_attr_t;

/* ---- Records: common memory ---- */

/** @brief Make count records hidden sprites. */
void nge_sprite_clear(nge_sprite_attr_t *a, uint8_t count);

/**
 * @brief Make a record a visible sprite.
 * @param a Record.
 * @param x X position 0-511 (0-319 is on screen).
 * @param y Y position 0-511 (0-255 is on screen).
 * @param pattern 8-bit pattern 0-63, or 4-bit pattern 0-127 with NGE_SPRITE_PATTERN4 in mode.
 * @param flags NGE_SPRITE_MIRROR_X, NGE_SPRITE_MIRROR_Y, NGE_SPRITE_ROTATE, NGE_SPRITE_PALETTE(n).
 * @param mode NGE_SPRITE_SCALE_X* | NGE_SPRITE_SCALE_Y* (or NGE_SPRITE_SCALE_1), and NGE_SPRITE_PATTERN4.
 */
void nge_sprite_set(nge_sprite_attr_t *a, uint16_t x, uint16_t y, uint8_t pattern,
                    uint8_t flags, uint8_t mode);

/** @brief Move a sprite. */
void nge_sprite_move(nge_sprite_attr_t *a, uint16_t x, uint16_t y);

/**
 * @brief Change a sprite's pattern (for example the next animation frame):
 *        8-bit 0-63, or 4-bit 0-127 if the record was set with NGE_SPRITE_PATTERN4.
 */
void nge_sprite_set_pattern(nge_sprite_attr_t *a, uint8_t pattern);

/** @brief Replace a sprite's mirror, rotate and palette flags. */
void nge_sprite_set_flags(nge_sprite_attr_t *a, uint8_t flags);

/** @brief Set a sprite's size: NGE_SPRITE_SCALE_X* | NGE_SPRITE_SCALE_Y*. */
void nge_sprite_set_scale(nge_sprite_attr_t *a, uint8_t scale);

/** @brief Show (on != 0) or hide a sprite, keeping its settings. */
void nge_sprite_show(nge_sprite_attr_t *a, uint8_t on);

/* ---- Hardware: common memory ---- */

/**
 * @brief Send count records to hardware sprites first to first+count-1.
 * @param a Records: in common memory, or in a banked caller's own page.
 */
void nge_sprite_upload(uint8_t first, const nge_sprite_attr_t *a, uint8_t count);

/**
 * @brief Read and clear the sprite status.
 *
 * CSpect does not emulate the collision bit (CSpect known bugs on the
 * SpecNext wiki); test collision handling on hardware.
 * @return NGE_SPRITE_STATUS_COLLISION and NGE_SPRITE_STATUS_LINE_FULL bits,
 *         set if it happened since the last call.
 */
uint8_t nge_sprite_status(void);

/* ---- Hardware set-up: banked ---- */

/**
 * @brief Hide all 128 hardware sprites, set the transparent pixel value and show the sprite layer.
 *
 * Turns on the sprite layer and sprites over the border, keeping the layer
 * priority set by nge_video_init(). Call before the other sprite functions.
 */
void nge_sprite_init(void) NGE_BANKED;

/**
 * @brief Hide all 128 hardware sprites now.
 *
 * Records are not changed: sprites show again at their next upload.
 */
void nge_sprite_hide_all(void) NGE_BANKED;

/**
 * @brief Set the pixel value that is transparent in all patterns.
 * @param index Pixel value; for 4-bit patterns only the low 4 bits are used.
 */
void nge_sprite_set_transparency(uint8_t index) NGE_BANKED;

/** Page argument of the loaders: the data is in common memory ($6A00-$BFFF). */
#define NGE_SPRITE_DATA_COMMON  0

/*
 * Loaders: copy pattern or palette data into the sprite hardware.
 *
 * Data in a banked page (page = an odd page number): the loader maps that
 * page into slot 7, copies the data from it into the hardware, then maps the
 * previous page back into slot 7. The data must therefore be assembled for
 * slot 7 ($E000-$FFFF), for example in a SECTION PAGE_n with n odd (z88dk
 * assembles odd pages for $E000).
 *
 * Data in common memory (page = NGE_SPRITE_DATA_COMMON): nothing is mapped.
 *
 * The data argument is the address of the first pattern (or palette entry)
 * to load, as the Z80 sees it while the data is mapped: for data in a page,
 * an address in $E000-$FFFF, not an offset into the page. A label in a
 * SECTION PAGE_n with n odd already has this address, so pass the label (or
 * label + n * pattern size to start part-way through a set). The data loaded
 * must end by $FFFF: a call loads at most 8K from one page (32 8-bit or 64
 * 4-bit patterns); use one call per page for larger sets.
 *
 * The loaders are banked, so their own page pair is in slots 6 and 7 while
 * they run: data the caller has mapped into slot 6 or 7 is not visible to
 * them. Pass its page instead and let the loader map it.
 */

/**
 * @brief Load 8-bit patterns into the hardware pattern memory.
 * @param page Odd page holding the data, or NGE_SPRITE_DATA_COMMON.
 * @param first First hardware pattern to replace (0-63).
 * @param data Address of the first pattern to load ($E000-$FFFF for data in
 *             a page), NGE_SPRITE_PATTERN_SIZE bytes per pattern.
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
 * @param first First hardware 4-bit pattern to replace (0-127).
 * @param data Address of the first pattern to load ($E000-$FFFF for data in
 *             a page), NGE_SPRITE_PATTERN4_SIZE bytes per pattern.
 * @param count Number of patterns.
 */
void nge_sprite_load_patterns4(uint8_t page, uint8_t first, const uint8_t *data, uint8_t count) NGE_BANKED;

/**
 * @brief Load entries of the first sprite palette.
 * @param page Odd page holding the entries, or NGE_SPRITE_DATA_COMMON.
 * @param first First palette index to write (0-255).
 * @param entries Address of the first 9-bit palette entry to load
 *                ($E000-$FFFF for entries in a page).
 * @param count Number of entries.
 */
void nge_sprite_load_palette(uint8_t page, uint8_t first, const nge_palette_entry_9_t *entries,
                             uint16_t count) NGE_BANKED;

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

#endif
