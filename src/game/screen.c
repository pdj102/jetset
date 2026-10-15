#include "game/screen.h"

#include <stddef.h>
#include <arch/zxn.h>

#include "core/nge_util.h"
#include "hw/nge_palette.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"
#include "tilemap/nge_tilemap.h"

const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 26 };
static const nge_rect_t stats_area = { .x1 = 28, .x2 = 39, .y1 = 0, .y2 = 26 };
static const nge_rect_t message_area = { .x1 = 0, .x2 = 39, .y1 = 27, .y2 = 31 };

NGE_FILE *screen_stats;
NGE_FILE *screen_messages;
char screen_name[SCREEN_NAME_MAX + 1] = "Rogue";

#define STATS_ROW_TURN  8    /* stats window row of the turn count */

/* Game graphics palette (bank 0); text colours are banks 12-15 (nge_tm_load_ink_palette) */
static const nge_palette_entry_9_t tile_palette[] = {
    { 0x00, 0x00 },
    { 0xE0, 0x00 },
    { 0x1F, 0x00 },
    { 0xFF, 0x00 },
};

/* Tile 0 is all pixel value 0, so it is fully transparent */
static const nge_tile_t empty_tile = {
    .tile_id = 0,
    .tile_attr = 0,
};

void screen_init(void)
{
    nge_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, 4);
    nge_tm_load_ink_palette();

    nge_tilemap_init();
    nge_tilemap_clear(&empty_tile);
    nge_tilemap_set_scroll(0, 0);

    screen_stats = nge_open_tilemap(&stats_area, 0);
    screen_messages = nge_open_tilemap(&message_area, 0);
    nge_util_assert(screen_stats != NULL && screen_messages != NULL);

    nge_tm_clip(1, screen_stats);
    nge_tm_cls(screen_stats);
    nge_tm_cls(screen_messages);
}

void screen_draw_stats(uint16_t turn)
{
    nge_tm_cls(screen_stats);
    nge_tm_ink(NGE_INK_YELLOW, screen_stats);
    nge_puts("Name: ", screen_stats);
    nge_puts(screen_name, screen_stats);
    nge_puts("\n\n", screen_stats);
    nge_tm_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("HP    12/12\n", screen_stats);
    nge_puts("Str      16\n", screen_stats);
    nge_puts("AC        7\n", screen_stats);
    nge_puts("Gold      0\n\n", screen_stats);
    nge_tm_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Depth     1\n", screen_stats);
    screen_show_turn(turn);
}

void screen_show_turn(uint16_t turn)
{
    nge_tm_at(STATS_ROW_TURN, 0, screen_stats);
    nge_tm_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Turn ", screen_stats);
    nge_putu16w(turn, 6, ' ', screen_stats);
}

void screen_message(const char *text)
{
    nge_tm_ink(NGE_INK_WHITE, screen_messages);
    nge_puts(text, screen_messages);
    nge_putc(NGE_CTRL_LF, screen_messages);
}
