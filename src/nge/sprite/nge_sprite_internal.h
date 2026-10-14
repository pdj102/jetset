#ifndef NGE_SPRITE_INTERNAL_H
#define NGE_SPRITE_INTERNAL_H

/*
 * Shared by nge_sprite.c (common memory: record functions and upload) and
 * nge_sprite_setup.c (banked: hardware set-up). Not for use outside the
 * sprite module.
 */

#include "sprite/nge_sprite.h"

#include <stddef.h>

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

#endif
