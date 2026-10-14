#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

/**
 * @file sprite_data.h
 * @brief Demo sprite graphics in page DEMO_SPRITE_PAGE (sprite_data.asm).
 *
 * The labels are addresses in slot 7 ($E000). Pass DEMO_SPRITE_PAGE to the
 * nge sprite loaders, which map it in for the upload.
 */

#include <stdint.h>

#include "hw/nge_palette.h"

#define DEMO_SPRITE_PAGE      101 /**< SECTION PAGE_101 in sprite_data.asm. */
#define DEMO_SPRITE_PATTERNS  2   /**< 8-bit patterns: two walking frames. */
#define DEMO_SPRITE_COLOURS   32  /**< Palette entries, from index 0. */

extern const uint8_t demo_sprite_patterns[];
extern const nge_palette_entry_9_t demo_sprite_palette[];

#endif
