#include <stdio.h>
#include <arch/zxn.h>

#include "zxn/zxn_cspect_stream.h"
#include "zxn/zxn_frame.h"
#include "zxn/zxn_hw.h"
#include "zxn/zxn_palette.h"
#include "zxn/zxn_print.h"
#include "zxn/zxn_printf.h"
#include "zxn/zxn_tilemap.h"
#include "zxn/zxn_tm_stream.h"
#include "zxn/zxn_uart_stream.h"
#include "zxn/zxn_util.h"

static const zxn_palette_entry_9_t tile_palette[] = {
    { 0x00, 0x00 },
    { 0xE0, 0x00 },
    { 0x1F, 0x00 },
    { 0xFF, 0x00 },
};

/* Tile 0 is all pixel value 0, so it is fully transparent */
static const zxn_tile_t empty_tile_entry = {
    .tile_id = 0,
    .tile_attr = 0,
};

/* A game graphics tile (palette bank 0) */
static const zxn_tile_t game_tile_entry = {
    .tile_id = 2,
    .tile_attr = 0,
};

int main(void)
{
    static const zxn_rect_t left_area = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 9 };
    static const zxn_rect_t right_area = { .x1 = 20, .x2 = 39, .y1 = 10, .y2 = 20 };
    static const zxn_rect_t game_area = { .x1 = 0, .x2 = 39, .y1 = 22, .y2 = 30 };
    static const zxn_rect_t hud_area = { .x1 = 0, .x2 = 39, .y1 = 31, .y2 = 31 };
    ZXN_FILE *left;
    ZXN_FILE *right;
    ZXN_FILE *uart;
    ZXN_FILE *hud;
    uint8_t i;

    /* Send asserts and debug output to the CSpect console (NULL on hardware) */
    zxn_util_set_debug_stream(zxn_open_cspect_debug());
    zxn_util_debugf("jetset start, %u streams\n", ZXN_FILE_MAX);

    zxn_hw_init();
    zxn_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, 4);
    zxn_tm_load_ink_palette();

    zxn_tilemap_init();
    zxn_tilemap_clear(&empty_tile_entry);
    zxn_tilemap_set_scroll(0, 0);

    /* Game graphics in their own area, next to the text windows */
    zxn_tilemap_fill(&game_area, &game_tile_entry);

    left = zxn_open_tilemap(&left_area, 0);
    right = zxn_open_tilemap(&right_area, 0);
    uart = zxn_open_uart();

    /* Clear the left window, then fill it past the bottom to force wrap and scroll */
    zxn_tm_cls(left);
    zxn_tm_ink(ZXN_INK_YELLOW, left);
    for (i = 0; i < 12; i++) {
        zxn_puts("Line wraps within the window ", left);
        zxn_putc(ZXN_CTRL_LF, left);
    }

    /* Change colour and position the cursor in the right window */
    zxn_tm_cls(right);
    zxn_tm_ink(ZXN_INK_WHITE, right);
    zxn_tm_at(3, 0, right);
    zxn_puts("abcdefghijklmnopqrstuvwxyz", right);
    zxn_tm_at(5, 3, right);
    zxn_puts("AT 5,3 WHITE", right);

    /* Fixed-width decimal and hex output */
    zxn_tm_at(7, 0, right);
    zxn_tm_ink(ZXN_INK_CYAN, right);
    zxn_puts("SCORE ", right);
    zxn_putu16w(420, 5, '0', right);
    zxn_tm_at(8, 0, right);
    zxn_puts("ADDR ", right);
    zxn_puthex16(0x4A00, right);

    /* Formatted output */
    zxn_tm_at(9, 0, right);
    zxn_tm_ink(ZXN_INK_RED, right);
    zxn_printf(right, "%s %05u %04X %d", "HI", 420u, 0x4A00u, -7);

    zxn_puts("uart\n", uart);

    /* Closing frees a slot that the next open reuses */
    zxn_close(uart);

    /* Status bar: clip mode never wraps or scrolls; clear-to-EOL removes old digits */
    hud = zxn_open_tilemap(&hud_area, 0);
    zxn_util_assert(hud != NULL);
    zxn_tm_clip(1, hud);
    zxn_tm_ink(ZXN_INK_CYAN, hud);
    zxn_puts("LIVES 3  SCORE 123456", hud);
    /* Redraw the score as a shorter number; it is the last field on the line */
    zxn_tm_at(0, 15, hud);
    zxn_putu16(99, hud);
    zxn_tm_clreol(hud);

    zxn_util_info("init done");

    /* DEMO: remove - shows the assert output format, then halts here */
    zxn_util_assert(ZXN_FILE_MAX == 0);

    while (1)
    {
        zxn_frame_wait_vblank();
    }
}
