#include <stdio.h>
#include <arch/zxn.h>

#include "output/nge_cspect_stream.h"
#include "hw/nge_frame.h"
#include "hw/nge_hw.h"
#include "hw/nge_mmu.h"
#include "hw/nge_palette.h"
#include "output/nge_print.h"
#include "output/nge_printf.h"
#include "sprite/nge_sprite.h"
#include "tilemap/nge_tilemap.h"
#include "output/nge_tm_stream.h"
#include "output/nge_uart_stream.h"
#include "core/nge_util.h"

#include "data/sprite_data.h"

static const nge_palette_entry_9_t tile_palette[] = {
    { 0x00, 0x00 },
    { 0xE0, 0x00 },
    { 0x1F, 0x00 },
    { 0xFF, 0x00 },
};

/* Tile 0 is all pixel value 0, so it is fully transparent */
static const nge_tile_t empty_tile_entry = {
    .tile_id = 0,
    .tile_attr = 0,
};

/* A game graphics tile (palette bank 0) */
static const nge_tile_t game_tile_entry = {
    .tile_id = 2,
    .tile_attr = 0,
};

/* Sprite ids in the demo */
#define SPR_STANDING  0
#define SPR_WALKER    1
#define SPR_BIG       2
#define SPR_RECOLOUR  3

#define WALK_Y          NGE_SPRITE_FROM_TILE(20)  /* feet on the game area (row 22) */
#define WALK_X_MAX      (NGE_SPRITE_FROM_TILE(40) - 16)
#define WALK_FRAME_SHIFT 3                        /* 8 frames per animation step */

/* Load the demo sprite graphics from their banked page. */
static void load_sprite_graphics(void)
{
    uint8_t old = nge_mmu_map(DEMO_SPRITE_SLOT, DEMO_SPRITE_PAGE);

    nge_sprite_load_patterns(0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_write_palette9(RPC_SELECT_SPRITES_PALETTE_0, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);

    nge_mmu_map(DEMO_SPRITE_SLOT, old);
}

int main(void)
{
    static const nge_rect_t left_area = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 9 };
    static const nge_rect_t right_area = { .x1 = 20, .x2 = 39, .y1 = 10, .y2 = 20 };
    static const nge_rect_t game_area = { .x1 = 0, .x2 = 39, .y1 = 22, .y2 = 30 };
    static const nge_rect_t hud_area = { .x1 = 0, .x2 = 39, .y1 = 31, .y2 = 31 };
    NGE_FILE *left;
    NGE_FILE *right;
    NGE_FILE *uart;
    NGE_FILE *hud;
    uint8_t i;
    uint16_t walk_x = 0;
    int8_t walk_dx = 1;
    uint8_t frame = 0;

    /* Send asserts and debug output to the CSpect console (NULL on hardware) */
    nge_util_set_debug_stream(nge_open_cspect_debug());
    nge_util_debugf("jetset start, %u streams\n", NGE_FILE_MAX);

    nge_hw_init();
    nge_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, 4);
    nge_tm_load_ink_palette();

    nge_tilemap_init();
    nge_tilemap_clear(&empty_tile_entry);
    nge_tilemap_set_scroll(0, 0);

    /* Game graphics in their own area, next to the text windows */
    nge_tilemap_fill(&game_area, &game_tile_entry);

    left = nge_open_tilemap(&left_area, 0);
    right = nge_open_tilemap(&right_area, 0);
    uart = nge_open_uart();

    /* Clear the left window, then fill it past the bottom to force wrap and scroll */
    nge_tm_cls(left);
    nge_tm_ink(NGE_INK_YELLOW, left);
    for (i = 0; i < 12; i++) {
        nge_puts("Line wraps within the window ", left);
        nge_putc(NGE_CTRL_LF, left);
    }

    /* Change colour and position the cursor in the right window */
    nge_tm_cls(right);
    nge_tm_ink(NGE_INK_WHITE, right);
    nge_tm_at(3, 0, right);
    nge_puts("abcdefghijklmnopqrstuvwxyz", right);
    nge_tm_at(5, 3, right);
    nge_puts("AT 5,3 WHITE", right);

    /* Fixed-width decimal and hex output */
    nge_tm_at(7, 0, right);
    nge_tm_ink(NGE_INK_CYAN, right);
    nge_puts("SCORE ", right);
    nge_putu16w(420, 5, '0', right);
    nge_tm_at(8, 0, right);
    nge_puts("ADDR ", right);
    nge_puthex16(0x4A00, right);

    /* Formatted output */
    nge_tm_at(9, 0, right);
    nge_tm_ink(NGE_INK_RED, right);
    nge_printf(right, "%s %05u %04X %d", "HI", 420u, 0x4A00u, -7);

    nge_puts("uart\n", uart);

    /* Closing frees a slot that the next open reuses */
    nge_close(uart);

    /* Status bar: clip mode never wraps or scrolls; clear-to-EOL removes old digits */
    hud = nge_open_tilemap(&hud_area, 0);
    nge_util_assert(hud != NULL);
    nge_tm_clip(1, hud);
    nge_tm_ink(NGE_INK_CYAN, hud);
    nge_puts("LIVES 3  SCORE 123456", hud);
    /* Redraw the score as a shorter number; it is the last field on the line */
    nge_tm_at(0, 15, hud);
    nge_putu16(99, hud);
    nge_tm_clreol(hud);

    /* Sprites: graphics come from banked page 32, mapped in only while loading */
    nge_sprite_init();
    load_sprite_graphics();

    /* Standing on the game area, exactly on tile (4, 20) */
    nge_sprite_set(SPR_STANDING, NGE_SPRITE_FROM_TILE(4), WALK_Y, 0, 0);
    /* Double size, below the left text window */
    nge_sprite_set(SPR_BIG, NGE_SPRITE_FROM_TILE(8), NGE_SPRITE_FROM_TILE(12), 0, 0);
    nge_sprite_set_scale(SPR_BIG, NGE_SPRITE_SCALE_X2 | NGE_SPRITE_SCALE_Y2);
    /* Palette offset 1 recolours the same pattern; mirrored, in the left border area */
    nge_sprite_set(SPR_RECOLOUR, 0, NGE_SPRITE_FROM_TILE(17), 0,
                  NGE_SPRITE_PALETTE(1) | NGE_SPRITE_MIRROR_X);
    /* Walks back and forth along the game area */
    nge_sprite_set(SPR_WALKER, walk_x, WALK_Y, 0, 0);

    nge_util_info("init done");

    while (1)
    {
        nge_frame_wait_vblank();
        /* First thing in the blanking period, so no sprite changes mid-frame */
        nge_sprite_update();

        frame++;
        walk_x += walk_dx;
        if (walk_x == 0 || walk_x == WALK_X_MAX) {
            walk_dx = -walk_dx;
        }
        nge_sprite_move(SPR_WALKER, walk_x, WALK_Y);
        /* The pattern faces right; mirror it when walking left */
        nge_sprite_set_flags(SPR_WALKER, walk_dx < 0 ? NGE_SPRITE_MIRROR_X : 0);
        nge_sprite_set_pattern(SPR_WALKER, (uint8_t)(frame >> WALK_FRAME_SHIFT) & 1);
    }
}
