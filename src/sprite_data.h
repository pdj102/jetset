#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

/**
 * @file sprite_data.h
 * @brief Demo sprite graphics in banked page DEMO_SPRITE_PAGE (sprite_data.asm).
 *
 * The labels are addresses in slot 6 ($C000): map DEMO_SPRITE_PAGE into
 * ZXN_MMU_SLOT_BANKED_0 before reading them.
 */

#include <stdint.h>

#include "zxn/zxn_palette.h"

#define DEMO_SPRITE_PAGE      32  /**< SECTION PAGE_32 in sprite_data.asm. */
#define DEMO_SPRITE_PATTERNS  2   /**< 8-bit patterns: two walking frames. */
#define DEMO_SPRITE_COLOURS   32  /**< Palette entries, from index 0. */

extern const uint8_t demo_sprite_patterns[];
extern const zxn_palette_entry_9_t demo_sprite_palette[];

#endif
