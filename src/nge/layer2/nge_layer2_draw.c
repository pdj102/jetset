/*
 * Layer 2 drawing: banked in page 90 with its literals. Every function maps
 * the Layer 2 page it writes into slot 7, calls no banked function while it
 * is there, and maps the old page back (N-BANK-4). The font is read from
 * bank 5, which is always mapped.
 */
#ifdef __SDCC
#pragma codeseg PAGE_90
#pragma constseg PAGE_90
#endif

#include "layer2/nge_layer2.h"

#include <arch/zxn.h>
#include <string.h>

#include "core/nge_util.h"
#include "hw/nge_mmu.h"

/* Next registers without z88dk names */
#define NGE_REG_DISPLAY_CONTROL   0x69
#define NGE_RDC_ENABLE_LAYER_2    0x80
#define NGE_REG_LAYER_2_CONTROL   0x70
#define NGE_RL2C_320x256          0x10   /* bits 5-4: 320 x 256 x 8; bits 3-0: palette offset 0 */
#define NGE_REG_LAYER_2_OFFSET_X_MSB  0x71
#define NGE_RSLS_PRIORITY_MASK    0x1C   /* REG_SPRITE_LAYER_SYSTEM bits 4-2: the layer order */

#define SLOT        7
#define SLOT_BASE   ((uint8_t *)0xE000)
#define CELL        8                  /* pixels a cell */
#define COLUMN      256                /* bytes a pixel column */
#define CELLS_PER_PAGE  4              /* 32 pixel columns an 8K page */
#define FONT        ((const uint8_t *)0x4A00)   /* the tile definitions (data/tile_defs_data.asm) */
#define GLYPH_BYTES 32                 /* 8 x 8 pixels, 4 bits each */

/* Map the page holding cell column col; the address of its first pixel column */
static uint8_t *map_col(uint8_t col)
{
    nge_mmu_map(SLOT, (uint8_t)(NGE_LAYER2_PAGE + col / CELLS_PER_PAGE));
    return SLOT_BASE + (uint16_t)(col % CELLS_PER_PAGE) * CELL * COLUMN;
}

/* The layer order: priority (RSLS_LAYER_PRIORITY_*), keeping the register's other bits */
static void set_priority(uint8_t priority)
{
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  (uint8_t)((ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) & ~NGE_RSLS_PRIORITY_MASK) | priority));
}

void nge_layer2_init(void) NGE_BANKED
{
    uint16_t i;

    ZXN_WRITE_REG(REG_LAYER_2_RAM_BANK, NGE_LAYER2_PAGE / 2);
    ZXN_WRITE_REG(NGE_REG_LAYER_2_CONTROL, NGE_RL2C_320x256);
    ZXN_WRITE_REG(REG_LAYER_2_OFFSET_X, 0);
    ZXN_WRITE_REG(NGE_REG_LAYER_2_OFFSET_X_MSB, 0);
    ZXN_WRITE_REG(REG_LAYER_2_OFFSET_Y, 0);
    ZXN_WRITE_REG(REG_GLOBAL_TRANSPARENCY_COLOR, NGE_LAYER2_TRANSPARENT);

    /* The whole layer: at 320 x 256 the clip window's X is in units of 2 pixels */
    ZXN_WRITE_REG(REG_CLIP_WINDOW_CONTROL, RCWC_RESET_LAYER_2_CLIP_INDEX);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_LAYER_2, 0);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_LAYER_2, 159);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_LAYER_2, 0);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_LAYER_2, 255);

    /* Palette 0, identity: a pixel's byte is its RRRGGGBB colour */
    ZXN_WRITE_REG(REG_PALETTE_CONTROL,
                  (uint8_t)((ZXN_READ_REG(REG_PALETTE_CONTROL) & 0x8F) | RPC_SELECT_LAYER_2_PALETTE_0));
    ZXN_WRITE_REG(REG_PALETTE_INDEX, 0);
    for (i = 0; i < 256; i++) {
        ZXN_WRITE_REG(REG_PALETTE_VALUE_8, (uint8_t)i);
    }

    nge_layer2_fill(0, 0, NGE_LAYER2_COLS - 1, NGE_LAYER2_ROWS - 1, NGE_LAYER2_TRANSPARENT);
    nge_layer2_show(1);
}

void nge_layer2_show(uint8_t on) NGE_BANKED
{
    uint8_t dc = ZXN_READ_REG(NGE_REG_DISPLAY_CONTROL);

    if (on) {
        ZXN_WRITE_REG(NGE_REG_DISPLAY_CONTROL, (uint8_t)(dc | NGE_RDC_ENABLE_LAYER_2));
        set_priority(RSLS_LAYER_PRIORITY_LSU);   /* Layer 2 over the sprites over the tilemap */
    } else {
        ZXN_WRITE_REG(NGE_REG_DISPLAY_CONTROL, (uint8_t)(dc & ~NGE_RDC_ENABLE_LAYER_2));
        set_priority(RSLS_LAYER_PRIORITY_SLU);   /* the order nge_video_init() sets */
    }
}

void nge_layer2_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);
    uint16_t height;
    uint8_t col;
    uint8_t i;
    uint8_t *p;

    nge_util_assert(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);

    height = (uint16_t)(y2 - y1 + 1) * CELL;
    for (col = x1; col <= x2; col++) {
        p = map_col(col) + (uint16_t)y1 * CELL;
        for (i = 0; i < CELL; i++) {
            memset(p, colour, height);
            p += COLUMN;
        }
    }
    nge_mmu_map(SLOT, old);
}

/* Draw glyph c into the pixel columns at p (cell column mapped), from pixel row y */
static void glyph(uint8_t *p, uint8_t c, uint8_t ink, uint8_t paper)
{
    const uint8_t *g = FONT + (uint16_t)c * GLYPH_BYTES;
    uint8_t x;
    uint8_t y;
    uint8_t b;

    for (x = 0; x < CELL; x++) {
        for (y = 0; y < CELL; y++) {
            b = g[y * 4 + x / 2];               /* 4 bytes a row, two pixels a byte */
            b = (x & 1) ? (b & 0x0F) : (b >> 4);
            p[y] = b ? ink : paper;
        }
        p += COLUMN;
    }
}

void nge_layer2_glyph(uint8_t col, uint8_t row, uint8_t c, uint8_t ink, uint8_t paper) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);

    nge_util_assert(col < NGE_LAYER2_COLS && row < NGE_LAYER2_ROWS);

    glyph(map_col(col) + (uint16_t)row * CELL, c, ink, paper);
    nge_mmu_map(SLOT, old);
}

void nge_layer2_text(uint8_t col, uint8_t row, const char *s, uint8_t ink, uint8_t paper) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);

    nge_util_assert(row < NGE_LAYER2_ROWS);

    for (; *s != '\0' && col < NGE_LAYER2_COLS; s++, col++) {
        glyph(map_col(col) + (uint16_t)row * CELL, (uint8_t)*s, ink, paper);
    }
    nge_mmu_map(SLOT, old);
}

void nge_layer2_scroll_up(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);
    uint16_t moved;
    uint8_t col;
    uint8_t i;
    uint8_t *p;

    nge_util_assert(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);

    moved = (uint16_t)(y2 - y1) * CELL;   /* every row but the top one moves up */
    for (col = x1; col <= x2; col++) {
        p = map_col(col) + (uint16_t)y1 * CELL;
        for (i = 0; i < CELL; i++) {
            memmove(p, p + CELL, moved);   /* a column's rows are contiguous */
            memset(p + moved, fill, CELL);
            p += COLUMN;
        }
    }
    nge_mmu_map(SLOT, old);
}
