/*
 * Sprite setup functions: banked, called through banked_call (see
 * core/nge_bank.h). The per-frame functions are in nge_sprite.c.
 *
 * This code must stay in page 42 (slot 6 during the call): the loaders map
 * the data page into slot 7, which would unmap any code there.
 */
#ifdef __SDCC
#pragma codeseg PAGE_42
#endif

#include "sprite/nge_sprite_internal.h"

#include <arch/zxn.h>
#include <string.h>

#include "hw/nge_mmu.h"

#define DATA_SLOT  7   /* $E000: odd pages are assembled for it */

/* Map the loader's data page into slot 7; returns the page to map back */
static uint8_t data_map(uint8_t page)
{
    if (page == NGE_SPRITE_DATA_COMMON) {
        return 0;
    }
    nge_util_assert(page & 1);
    return nge_mmu_map(DATA_SLOT, page);
}

static void data_unmap(uint8_t page, uint8_t old)
{
    if (page != NGE_SPRITE_DATA_COMMON) {
        nge_mmu_map(DATA_SLOT, old);
    }
}

/* Send bytes to the pattern memory, from the pattern selected on port $303B */
static void pattern_out(const uint8_t *data, uint16_t bytes)
{
    while (bytes--) {
        IO_SPRITE_PATTERN = *data++;
    }
}

void nge_sprite_load_patterns(uint8_t page, uint8_t first, const uint8_t *data, uint8_t n) NGE_BANKED
{
    uint8_t old;

    nge_util_assert(data != NULL);
    nge_util_assert((uint16_t)first + n <= NGE_SPRITE_PATTERNS);

    old = data_map(page);
    IO_SPRITE_SLOT = first;
    pattern_out(data, (uint16_t)n * NGE_SPRITE_PATTERN_SIZE);
    data_unmap(page, old);
}

void nge_sprite_load_patterns4(uint8_t page, uint8_t first, const uint8_t *data, uint8_t n) NGE_BANKED
{
    uint8_t old;

    nge_util_assert(data != NULL);
    nge_util_assert((uint16_t)first + n <= NGE_SPRITE_PATTERNS4);

    old = data_map(page);
    /* Bit 7 starts the upload half way into the 256-byte slot */
    IO_SPRITE_SLOT = (first >> 1) | (uint8_t)((first & 1) << 7);
    pattern_out(data, (uint16_t)n * NGE_SPRITE_PATTERN4_SIZE);
    data_unmap(page, old);
}

void nge_sprite_load_palette(uint8_t page, uint8_t first, const nge_palette_entry_9_t *entries,
                             uint16_t n) NGE_BANKED
{
    uint8_t old = data_map(page);

    nge_write_palette9(RPC_SELECT_SPRITES_PALETTE_0, first, entries, n);
    data_unmap(page, old);
}

void nge_sprite_init(void) NGE_BANKED
{
    uint8_t i;

    memset(nge_sprite_table, 0, sizeof(nge_sprite_table));
    for (i = 0; i < NGE_SPRITE_MAX; i++) {
        nge_sprite_table[i].attr3 = ATTR3_ATTR4;
    }
    nge_sprite_count = NGE_SPRITE_MAX;
    nge_sprite_update();
    nge_sprite_count = 0;

    nge_sprite_set_transparency(NGE_SPRITE_TRANSPARENT);
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER);
}

void nge_sprite_set_transparency(uint8_t index) NGE_BANKED
{
    ZXN_WRITE_REG(REG_SPRITE_TRANSPARENCY_INDEX, index);
}

void nge_sprite_set(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags) NGE_BANKED
{
    nge_sprite_t *s = nge_sprite_entry(id);

    s->attr2 = flags & ATTR2_FLAGS;
    nge_sprite_entry_place(s, x, y);
    nge_sprite_entry_pattern(s, pattern);
    s->attr3 |= ATTR3_VISIBLE;
}

void nge_sprite_set4(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags) NGE_BANKED
{
    nge_sprite_t *s = nge_sprite_entry(id);

    s->attr2 = flags & ATTR2_FLAGS;
    nge_sprite_entry_place(s, x, y);
    nge_sprite_entry_pattern4(s, pattern);
    s->attr3 |= ATTR3_VISIBLE;
}

void nge_sprite_set_scale(uint8_t id, uint8_t scale) NGE_BANKED
{
    nge_sprite_t *s = nge_sprite_entry(id);

    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_SCALE) | (scale & ATTR4_SCALE);
}

void nge_sprite_hide_all(void) NGE_BANKED
{
    uint8_t i;

    for (i = 0; i < nge_sprite_count; i++) {
        nge_sprite_table[i].attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void nge_sprite_set_clip(const nge_rect_t *r) NGE_BANKED
{
    nge_util_assert(r != NULL);
    nge_util_assert(r->x1 <= r->x2 && r->y1 <= r->y2);

    ZXN_WRITE_REG(REG_CLIP_WINDOW_CONTROL, RCWC_RESET_SPRITE_CLIP_INDEX);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->x1);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->x2);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->y1);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->y2);
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) | NGE_RSLS_CLIP_OVER_BORDER);
}

void nge_sprite_clear_clip(void) NGE_BANKED
{
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) & (uint8_t)~NGE_RSLS_CLIP_OVER_BORDER);
}
