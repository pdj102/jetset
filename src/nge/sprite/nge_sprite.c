/*
 * Sprite functions kept in common memory: the record functions and the
 * upload, used every frame, also by banked code with records in its own
 * page. The banked hardware set-up functions are in nge_sprite_setup.c.
 */
#include "sprite/nge_sprite_internal.h"

#include <arch/zxn.h>

void nge_sprite_clear(nge_sprite_attr_t *a, uint8_t count)
{
    nge_util_assert(a != NULL);

    while (count--) {
        a->x = 0;
        a->y = 0;
        a->attr2 = 0;
        a->attr3 = ATTR3_ATTR4;   /* hidden */
        a->attr4 = 0;
        a++;
    }
}

void nge_sprite_set(nge_sprite_attr_t *a, uint16_t x, uint16_t y, uint8_t pattern,
                    uint8_t flags, uint8_t mode)
{
    a->attr2 = flags & ATTR2_FLAGS;
    a->attr4 = mode & (ATTR4_4BIT | ATTR4_SCALE);
    nge_sprite_move(a, x, y);
    a->attr3 = ATTR3_VISIBLE;
    nge_sprite_set_pattern(a, pattern);
}

void nge_sprite_move(nge_sprite_attr_t *a, uint16_t x, uint16_t y)
{
    a->x = (uint8_t)x;
    a->y = (uint8_t)y;
    a->attr2 = (a->attr2 & ATTR2_FLAGS) | (uint8_t)((x >> 8) & ATTR2_X8);
    a->attr4 = (a->attr4 & (uint8_t)~ATTR4_Y8) | (uint8_t)((y >> 8) & ATTR4_Y8);
}

void nge_sprite_set_pattern(nge_sprite_attr_t *a, uint8_t pattern)
{
    if (a->attr4 & ATTR4_4BIT) {
        nge_util_assert(pattern < NGE_SPRITE_PATTERNS4);
        /* 4-bit pattern n is the (n & 1) half of 256-byte slot n >> 1 */
        a->attr3 = (a->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | (pattern >> 1);
        a->attr4 = (a->attr4 & (uint8_t)~ATTR4_N6) | (uint8_t)((pattern & 1) ? ATTR4_N6 : 0);
    } else {
        nge_util_assert(pattern < NGE_SPRITE_PATTERNS);
        a->attr3 = (a->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | pattern;
    }
}

void nge_sprite_set_flags(nge_sprite_attr_t *a, uint8_t flags)
{
    a->attr2 = (a->attr2 & ATTR2_X8) | (flags & ATTR2_FLAGS);
}

void nge_sprite_set_scale(nge_sprite_attr_t *a, uint8_t scale)
{
    a->attr4 = (a->attr4 & (uint8_t)~ATTR4_SCALE) | (scale & ATTR4_SCALE);
}

void nge_sprite_show(nge_sprite_attr_t *a, uint8_t on)
{
    if (on) {
        a->attr3 |= ATTR3_VISIBLE;
    } else {
        a->attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void nge_sprite_upload(uint8_t first, const nge_sprite_attr_t *a, uint8_t count)
{
    const uint8_t *p = (const uint8_t *)a;
    uint16_t bytes = (uint16_t)count * sizeof(nge_sprite_attr_t);

    nge_util_assert((uint16_t)first + count <= NGE_SPRITE_MAX);

    IO_SPRITE_SLOT = first;
    while (bytes--) {
        IO_SPRITE_ATTRIBUTE = *p++;
    }
}

uint8_t nge_sprite_status(void)
{
    return IO_SPRITE_FLAGS & (NGE_SPRITE_STATUS_COLLISION | NGE_SPRITE_STATUS_LINE_FULL);
}
