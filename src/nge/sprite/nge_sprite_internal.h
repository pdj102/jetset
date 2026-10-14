#ifndef NGE_SPRITE_INTERNAL_H
#define NGE_SPRITE_INTERNAL_H

/*
 * Shared by nge_sprite.c (common memory: per-frame functions and pattern
 * loading) and nge_sprite_setup.c (banked: setup functions). Not for use
 * outside the sprite module.
 */

#include "sprite/nge_sprite.h"

/* Register 0x15 bit 5: apply the clip window in over-border mode (X in 2-pixel units) */
#define NGE_RSLS_CLIP_OVER_BORDER  0x20

/* attr2 */
#define ATTR2_X8     0x01  /* X bit 8 */
#define ATTR2_FLAGS  0xFE  /* palette offset, mirror, rotate */
/* attr3 */
#define ATTR3_VISIBLE  0x80
#define ATTR3_ATTR4    0x40  /* attr4 is present: always set, so every sprite is 5 bytes */
#define ATTR3_PATTERN  0x3F
/* attr4 (anchor sprite) */
#define ATTR4_4BIT     0x80  /* H: 4-bit pattern */
#define ATTR4_N6       0x40  /* second 128 bytes of the 256-byte pattern slot */
#define ATTR4_SCALE    0x1E
#define ATTR4_Y8       0x01  /* Y bit 8 */

/* The game's table (nge_sprite_table_init()): hardware sprites
   nge_sprite_table_first to nge_sprite_table_first + nge_sprite_table_size - 1;
   entries 0 to nge_sprite_count-1 are sent by nge_sprite_update() */
extern nge_sprite_attr_t *nge_sprite_table;
extern uint8_t nge_sprite_table_first;
extern uint8_t nge_sprite_table_size;
extern uint8_t nge_sprite_count;

/* Helpers in nge_sprite.c (common memory, so banked code can call them) */

/* Get a hardware sprite's table entry for changing; it is sent from now on. */
nge_sprite_attr_t *nge_sprite_entry(uint8_t id);
void nge_sprite_entry_place(nge_sprite_attr_t *s, uint16_t x, uint16_t y);
void nge_sprite_entry_pattern(nge_sprite_attr_t *s, uint8_t pattern);
void nge_sprite_entry_pattern4(nge_sprite_attr_t *s, uint8_t pattern);

#endif
