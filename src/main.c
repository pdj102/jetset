#include <stdio.h>
#include <arch/zxn.h>

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

static const zxn_tile_t wall_tile_entry = {
    .tile_id = 0,
    .tile_attr = 0,
};

static const zxn_tile_t blank_tile_entry = {
    .tile_id = 1,
    .tile_attr = 0,
};

int main(void)
{
    static const zxn_rect_t left_area = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 9 };
    static const zxn_rect_t right_area = { .x1 = 20, .x2 = 39, .y1 = 10, .y2 = 20 };
    ZXN_FILE *left;
    ZXN_FILE *right;
    ZXN_FILE *uart;
    uint8_t i;

    zxn_hw_init();
    zxn_write_palette9( RPC_SELECT_TILEMAP_PALETTE_0, tile_palette, 4);

    zxn_tilemap_init();
    zxn_tilemap_clear(&blank_tile_entry);
    zxn_tilemap_set_scroll(0, 0);

    left = zxn_open_tilemap(&left_area, 0);
    right = zxn_open_tilemap(&right_area, 0);
    uart = zxn_open_uart();

    /* Clear the left window, then fill it past the bottom to force wrap and scroll */
    zxn_tm_cls(left);
    zxn_tm_ink(2, left);
    for (i = 0; i < 12; i++) {
        zxn_puts("Line wraps within the window ", left);
        zxn_putc(ZXN_CTRL_LF, left);
    }

    /* Change colour and position the cursor in the right window */
    zxn_tm_cls(right);
    zxn_tm_ink(2, right);
    zxn_tm_at(5, 3, right);
    zxn_puts("AT 5,3 INK 1", right);

    /* Fixed-width decimal and hex output */
    zxn_tm_at(7, 0, right);
    zxn_puts("SCORE ", right);
    zxn_putu16w(420, 5, '0', right);
    zxn_tm_at(8, 0, right);
    zxn_puts("ADDR ", right);
    zxn_puthex16(0x4A00, right);

    /* Formatted output */
    zxn_tm_at(9, 0, right);
    zxn_printf(right, "%s %05u %04X %d", "HI", 420u, 0x4A00u, -7);

    zxn_puts("uart\n", uart);

    /* Closing frees a slot that the next open reuses */
    zxn_close(uart);
    uart = zxn_open_uart();
    zxn_util_assert(uart != NULL);

    while (1)
    {
        zxn_frame_wait_vblank();
    }
}
