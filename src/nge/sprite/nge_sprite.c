/*
 * Sprite functions kept in common memory: those called every frame, and the
 * helpers shared with the banked setup functions in nge_sprite_setup.c.
 */
#include "sprite/nge_sprite_internal.h"

#include <arch/zxn.h>

nge_sprite_t nge_sprite_table[NGE_SPRITE_MAX];
uint8_t nge_sprite_count;

nge_sprite_t *nge_sprite_entry(uint8_t id)
{
    nge_util_assert(id < NGE_SPRITE_MAX);

    if (id >= nge_sprite_count) {
        nge_sprite_count = id + 1;
    }
    return &nge_sprite_table[id];
}

void nge_sprite_entry_place(nge_sprite_t *s, uint16_t x, uint16_t y)
{
    s->x = (uint8_t)x;
    s->y = (uint8_t)y;
    s->attr2 = (s->attr2 & ATTR2_FLAGS) | (uint8_t)((x >> 8) & ATTR2_X8);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_Y8) | (uint8_t)((y >> 8) & ATTR4_Y8);
}

void nge_sprite_entry_pattern(nge_sprite_t *s, uint8_t pattern)
{
    nge_util_assert(pattern < NGE_SPRITE_PATTERNS);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | pattern;
    s->attr4 &= (uint8_t)~(ATTR4_4BIT | ATTR4_N6);
}

void nge_sprite_entry_pattern4(nge_sprite_t *s, uint8_t pattern)
{
    nge_util_assert(pattern < NGE_SPRITE_PATTERNS4);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | (pattern >> 1);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_N6) | ATTR4_4BIT | (uint8_t)((pattern & 1) ? ATTR4_N6 : 0);
}

void nge_sprite_move(uint8_t id, uint16_t x, uint16_t y)
{
    nge_sprite_entry_place(nge_sprite_entry(id), x, y);
}

void nge_sprite_set_pattern(uint8_t id, uint8_t pattern)
{
    nge_sprite_entry_pattern(nge_sprite_entry(id), pattern);
}

void nge_sprite_set_pattern4(uint8_t id, uint8_t pattern)
{
    nge_sprite_entry_pattern4(nge_sprite_entry(id), pattern);
}

void nge_sprite_set_flags(uint8_t id, uint8_t flags)
{
    nge_sprite_t *s = nge_sprite_entry(id);

    s->attr2 = (s->attr2 & ATTR2_X8) | (flags & ATTR2_FLAGS);
}

void nge_sprite_show(uint8_t id, uint8_t on)
{
    nge_sprite_t *s = nge_sprite_entry(id);

    if (on) {
        s->attr3 |= ATTR3_VISIBLE;
    } else {
        s->attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void nge_sprite_update(void)
{
    const uint8_t *p = (const uint8_t *)nge_sprite_table;
    uint16_t bytes = (uint16_t)nge_sprite_count * sizeof(nge_sprite_t);

    IO_SPRITE_SLOT = 0;
    while (bytes--) {
        IO_SPRITE_ATTRIBUTE = *p++;
    }
}

uint8_t nge_sprite_status(void)
{
    return IO_SPRITE_FLAGS & (NGE_SPRITE_STATUS_COLLISION | NGE_SPRITE_STATUS_LINE_FULL);
}
