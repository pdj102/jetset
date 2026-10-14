/*
 * Sprite functions kept in common memory: those called every frame (also by
 * banked code with records in its own page), and the helpers shared with
 * the banked setup functions in nge_sprite_setup.c.
 */
#include "sprite/nge_sprite_internal.h"

#include <arch/zxn.h>

nge_sprite_attr_t *nge_sprite_table;
uint8_t nge_sprite_table_first;
uint8_t nge_sprite_table_size;
uint8_t nge_sprite_count;

nge_sprite_attr_t *nge_sprite_entry(uint8_t id)
{
    uint8_t i = id - nge_sprite_table_first;

    nge_util_assert(nge_sprite_table != NULL);   /* nge_sprite_table_init() first */
    nge_util_assert(id >= nge_sprite_table_first && i < nge_sprite_table_size);

    if (i >= nge_sprite_count) {
        nge_sprite_count = i + 1;
    }
    return &nge_sprite_table[i];
}

void nge_sprite_entry_place(nge_sprite_attr_t *s, uint16_t x, uint16_t y)
{
    s->x = (uint8_t)x;
    s->y = (uint8_t)y;
    s->attr2 = (s->attr2 & ATTR2_FLAGS) | (uint8_t)((x >> 8) & ATTR2_X8);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_Y8) | (uint8_t)((y >> 8) & ATTR4_Y8);
}

void nge_sprite_entry_pattern(nge_sprite_attr_t *s, uint8_t pattern)
{
    nge_util_assert(pattern < NGE_SPRITE_PATTERNS);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | pattern;
    s->attr4 &= (uint8_t)~(ATTR4_4BIT | ATTR4_N6);
}

void nge_sprite_entry_pattern4(nge_sprite_attr_t *s, uint8_t pattern)
{
    nge_util_assert(pattern < NGE_SPRITE_PATTERNS4);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | (pattern >> 1);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_N6) | ATTR4_4BIT | (uint8_t)((pattern & 1) ? ATTR4_N6 : 0);
}

void nge_sprite_attr_build(nge_sprite_attr_t *a, uint16_t x, uint16_t y, uint8_t pattern,
                           uint8_t flags, uint8_t mode)
{
    a->x = (uint8_t)x;
    a->y = (uint8_t)y;
    a->attr2 = (flags & ATTR2_FLAGS) | (uint8_t)((x >> 8) & ATTR2_X8);
    a->attr4 = (mode & (ATTR4_4BIT | ATTR4_SCALE)) | (uint8_t)((y >> 8) & ATTR4_Y8);
    if (mode & NGE_SPRITE_PATTERN4) {
        nge_util_assert(pattern < NGE_SPRITE_PATTERNS4);
        a->attr3 = ATTR3_VISIBLE | ATTR3_ATTR4 | (pattern >> 1);
        if (pattern & 1) {
            a->attr4 |= ATTR4_N6;
        }
    } else {
        nge_util_assert(pattern < NGE_SPRITE_PATTERNS);
        a->attr3 = ATTR3_VISIBLE | ATTR3_ATTR4 | pattern;
    }
}

void nge_sprite_upload(uint8_t first, const nge_sprite_attr_t *attrs, uint8_t count)
{
    const uint8_t *p = (const uint8_t *)attrs;
    uint16_t bytes = (uint16_t)count * sizeof(nge_sprite_attr_t);

    nge_util_assert((uint16_t)first + count <= NGE_SPRITE_MAX);

    IO_SPRITE_SLOT = first;
    while (bytes--) {
        IO_SPRITE_ATTRIBUTE = *p++;
    }
}

void nge_sprite_table_init(nge_sprite_attr_t *table, uint8_t first, uint8_t size)
{
    uint8_t i;

    nge_util_assert(table != NULL && size > 0);
    nge_util_assert((uint16_t)first + size <= NGE_SPRITE_MAX);

    for (i = 0; i < size; i++) {
        table[i].x = 0;
        table[i].y = 0;
        table[i].attr2 = 0;
        table[i].attr3 = ATTR3_ATTR4;   /* hidden */
        table[i].attr4 = 0;
    }
    nge_sprite_table = table;
    nge_sprite_table_first = first;
    nge_sprite_table_size = size;
    nge_sprite_count = 0;
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
    nge_sprite_attr_t *s = nge_sprite_entry(id);

    s->attr2 = (s->attr2 & ATTR2_X8) | (flags & ATTR2_FLAGS);
}

void nge_sprite_show(uint8_t id, uint8_t on)
{
    nge_sprite_attr_t *s = nge_sprite_entry(id);

    if (on) {
        s->attr3 |= ATTR3_VISIBLE;
    } else {
        s->attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void nge_sprite_update(void)
{
    if (nge_sprite_table != NULL) {
        nge_sprite_upload(nge_sprite_table_first, nge_sprite_table, nge_sprite_count);
    }
}

uint8_t nge_sprite_status(void)
{
    return IO_SPRITE_FLAGS & (NGE_SPRITE_STATUS_COLLISION | NGE_SPRITE_STATUS_LINE_FULL);
}
