/* nge-dataseg: PAGE_90 */
/*
 * Layer 2 drawing: banked in page 90 with its literals, and with its one
 * table, the font as column bitmasks.
 *
 * The fills, scrolls and glyphs are done by the CPU, in assembly. The DMA
 * was tried for fills and scrolls and taken out: at 320 x 256, a cell
 * column's rows are only 32 bytes, and the DMA's set-up for each was no
 * cheaper than the CPU's copy (docs/game_loop.md, section 10). The table is in the code page, not
 * the odd data page as page data usually is, because every function here
 * maps a Layer 2 page into slot 7, where page 91 would be: the font must
 * stay readable in slot 6 while that page is mapped.
 *
 * Every function maps the Layer 2 page it writes into slot 7, calls no
 * banked function while it is there, and maps the old page back (N-BANK-4).
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
#define TILE_FONT   ((const uint8_t *)0x4A00)   /* the tile definitions (data/tile_defs_data.asm) */
#define TILE_BYTES  32                 /* 8 x 8 pixels, 4 bits each */
#define FONT_FIRST  32                 /* the font's characters: space to DEL */
#define FONT_COUNT  96

/* A cell never crosses a page: the drawing maps one page a cell column */
typedef char nge_layer2_cells_check[(8192 / (CELL * COLUMN) == CELLS_PER_PAGE) ? 1 : -1];

/*
 * The font as column bitmasks: 8 bytes a character, one a pixel column,
 * bit 7 the top pixel. A column of Layer 2 is 8 contiguous bytes, so a
 * glyph's column is drawn from one byte. Built by nge_layer2_init()
 */
static uint8_t font[FONT_COUNT][CELL];

/* The fill and scroll of a cell column (assembly on the Next, below) */
static void fill_cols(uint8_t *p, uint8_t cells, uint8_t colour);
static void scroll_cols(uint8_t *p, uint8_t cells, uint8_t fill);

/* ---- Set-up ---- */

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

/* The font's column bitmasks, from the tile definitions (4 bits a pixel, 0 background) */
static void make_font(void)
{
    const uint8_t *tile;
    uint8_t c;
    uint8_t x;
    uint8_t y;
    uint8_t b;
    uint8_t mask;

    for (c = 0; c < FONT_COUNT; c++) {
        tile = TILE_FONT + (uint16_t)(FONT_FIRST + c) * TILE_BYTES;
        for (x = 0; x < CELL; x++) {
            mask = 0;
            for (y = 0; y < CELL; y++) {
                b = tile[y * 4 + x / 2];   /* 4 bytes a row, two pixels a byte */
                if ((x & 1) ? (b & 0x0F) : (b >> 4)) {
                    mask |= (uint8_t)(0x80 >> y);
                }
            }
            font[c][x] = mask;
        }
    }
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

    make_font();
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

/* ---- Drawing ---- */

void nge_layer2_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);
    uint8_t cells;
    uint8_t col;

    nge_util_assert(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);

    cells = (uint8_t)(y2 - y1 + 1);
    for (col = x1; col <= x2; col++) {
        fill_cols(map_col(col) + (uint16_t)y1 * CELL, cells, colour);
    }
    nge_mmu_map(SLOT, old);
}

#ifdef __SDCC
/*
 * Fill cells cells (1 to 32) down the 8 pixel columns from p with colour:
 * unrolled stores, about 11 T-states a byte. A cell column never crosses a
 * 256-byte column, so inc l moves down. Arguments as zsdcc passes them: p,
 * then cells and colour (bytes)
 */
static void fill_cols(uint8_t *p, uint8_t cells, uint8_t colour) __naked
{
    (void)p;
    (void)cells;
    (void)colour;

    __asm
        ld hl, 2
        add hl, sp
        ld e, (hl)
        inc hl
        ld d, (hl)              ; de = p
        inc hl
        ld c, (hl)              ; c = cells
        inc hl
        ld a, (hl)              ; a = colour
        ex de, hl               ; hl = p
        ld d, 8                 ; d = pixel columns
    nge_layer2_fill_col:
        ld e, l                 ; the column start
        ld b, c
    nge_layer2_fill_cell:
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        djnz nge_layer2_fill_cell
        ld l, e
        inc h                   ; the next pixel column
        dec d
        jr nz, nge_layer2_fill_col
        ret
    __endasm;
}

/*
 * Scroll the 8 pixel columns from p up one cell: move cells cells (0 to
 * 31) up 8 bytes with LDIR, then fill the bottom cell with fill. The other
 * register set keeps the byte count, the fill and the columns left. Arguments
 * as zsdcc passes them: p, then cells and fill (bytes)
 */
static void scroll_cols(uint8_t *p, uint8_t cells, uint8_t fill) __naked
{
    (void)p;
    (void)cells;
    (void)fill;

    __asm
        ld hl, 2
        add hl, sp
        ld e, (hl)
        inc hl
        ld d, (hl)              ; de = p
        inc hl
        ld a, (hl)              ; a = cells to move
        inc hl
        ld b, (hl)              ; b = fill
        add a, a
        add a, a
        add a, a
        ld c, a                 ; c = bytes to move
        push bc
        exx
        pop bc                  ; the other set: b = fill, c = bytes
        ld d, 8                 ; and d = pixel columns
        exx
    nge_layer2_scroll_col:
        push de                 ; the column start
        ld l, e
        ld h, d
        exx
        ld a, c
        exx
        or a
        jr z, nge_layer2_scroll_fill
        ld c, a
        ld b, 0
        ld a, l
        add a, 8
        ld l, a                 ; hl = p + 8, in the same column
        ldir                    ; de = the bottom cell
    nge_layer2_scroll_fill:
        exx
        ld a, b
        exx
        ex de, hl
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        inc l
        ld (hl), a
        pop de
        inc d                   ; the next pixel column
        exx
        dec d
        exx
        jr nz, nge_layer2_scroll_col
        ret
    __endasm;
}
#else
/* The same in C, for builds without zsdcc */
static void fill_cols(uint8_t *p, uint8_t cells, uint8_t colour)
{
    uint8_t i;

    for (i = 0; i < CELL; i++) {
        memset(p, colour, (uint16_t)cells * CELL);
        p += COLUMN;
    }
}

static void scroll_cols(uint8_t *p, uint8_t cells, uint8_t fill)
{
    uint8_t i;

    for (i = 0; i < CELL; i++) {
        memmove(p, p + CELL, (uint16_t)cells * CELL);
        memset(p + (uint16_t)cells * CELL, fill, CELL);
        p += COLUMN;
    }
}
#endif

#ifdef __SDCC
/*
 * Draw the 8 columns of a glyph's bitmasks (mask) into the cell whose first
 * pixel column is at p: about 31 T-states a pixel, the 8 rows unrolled. Each pixel's colour is
 * chosen without a branch: the mask's top bit into the carry, then
 * sbc a,a (0 or $FF), and (ink ^ paper), xor paper. Registers: HL the
 * pixel, C the column's bits, D ink ^ paper, E paper;
 * the other set: HL the masks, B the columns left. A cell's 8 pixels
 * never cross a 256-byte column, so inc l moves down. The arguments are on
 * the stack as zsdcc passes them: p, mask (words), then ink, paper (bytes)
 */
static void glyph_cols(uint8_t *p, const uint8_t *mask, uint8_t ink, uint8_t paper) __naked
{
    (void)p;
    (void)mask;
    (void)ink;
    (void)paper;

    __asm
        ld hl, 2
        add hl, sp
        ld e, (hl)
        inc hl
        ld d, (hl)              ; de = p
        inc hl
        ld c, (hl)
        inc hl
        ld b, (hl)              ; bc = mask
        inc hl
        ld a, (hl)              ; a = ink
        inc hl
        ld l, (hl)              ; l = paper
        xor l
        ld h, a                 ; h = ink ^ paper
        push bc
        push de
        ld d, h
        ld e, l                 ; d = ink ^ paper, e = paper
        pop hl                  ; hl = p
        exx
        pop hl                  ; the other set: hl = mask
        ld b, 8                 ; and b = columns
    nge_layer2_glyph_col:
        ld a, (hl)
        inc hl
        exx
        ld c, a                 ; the column bits, top pixel first
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        inc l
        rlc c
        sbc a, a
        and d
        xor e
        ld (hl), a
        ld a, l
        sub 7
        ld l, a
        inc h                   ; the next pixel column, 256 bytes on
        exx
        djnz nge_layer2_glyph_col
        exx
        ret
    __endasm;
}
#else
/* The same in C, for builds without zsdcc */
static void glyph_cols(uint8_t *p, const uint8_t *mask, uint8_t ink, uint8_t paper)
{
    uint8_t *q;
    uint8_t bits;
    uint8_t x;
    uint8_t y;

    for (x = 0; x < CELL; x++) {
        bits = *mask++;
        q = p;
        for (y = 0; y < CELL; y++) {
            *q++ = (bits & 0x80) ? ink : paper;
            bits <<= 1;
        }
        p += COLUMN;
    }
}
#endif

/* Draw character c into the cell whose first pixel column is at p (its page mapped) */
static void glyph(uint8_t *p, uint8_t c, uint8_t ink, uint8_t paper)
{
    if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) {
        c = ' ';   /* not in the font */
    }
    glyph_cols(p, font[c - FONT_FIRST], ink, paper);
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
    uint8_t *p = NULL;

    nge_util_assert(row < NGE_LAYER2_ROWS);

    for (; *s != '\0' && col < NGE_LAYER2_COLS; s++, col++) {
        /* A page holds 4 cell columns: map it only on entering it */
        if (p == NULL || col % CELLS_PER_PAGE == 0) {
            p = map_col(col) + (uint16_t)row * CELL;
        }
        glyph(p, (uint8_t)*s, ink, paper);
        p += CELL * COLUMN;   /* the next cell column */
    }
    nge_mmu_map(SLOT, old);
}

void nge_layer2_scroll_up(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill) NGE_BANKED
{
    uint8_t old = nge_mmu_get(SLOT);
    uint8_t cells;
    uint8_t col;

    nge_util_assert(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);

    cells = (uint8_t)(y2 - y1);   /* every row but the top one moves up */
    for (col = x1; col <= x2; col++) {
        scroll_cols(map_col(col) + (uint16_t)y1 * CELL, cells, fill);
    }
    nge_mmu_map(SLOT, old);
}
