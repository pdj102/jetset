#include "game/screen.h"

#include <stddef.h>
#include <arch/zxn.h>

#include "core/nge_util.h"
#include "hw/nge_palette.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"
#include "tilemap/nge_tilemap.h"

#include "game/components/attrs.h"

const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 26 };
static const nge_rect_t stats_area = { .x1 = 28, .x2 = 39, .y1 = 0, .y2 = 26 };
static const nge_rect_t message_area = { .x1 = 0, .x2 = 39, .y1 = 27, .y2 = 30 };
static const nge_rect_t help_area = { .x1 = 0, .x2 = 39, .y1 = 31, .y2 = 31 };

NGE_FILE *screen_stats = NULL;
NGE_FILE *screen_messages = NULL;
NGE_FILE *screen_panel = NULL;
static NGE_FILE *help = NULL;
char screen_name[SCREEN_NAME_MAX + 1] = "Rogue";

#define STATS_ROW_HP      2    /* stats window row of the hit points */
#define STATS_ROW_AC      4    /* ...of the armour class */
#define STATS_ROW_TURN    8    /* ...of the turn count */
#define STATS_ROW_STATUS  10   /* ...of the statuses */

/* The message window's bottom line has text: the next message goes on a new line */
static uint8_t message_line_used;

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

    /* Called again, it starts afresh: close the streams it opened before */
    if (screen_stats != NULL) {
        nge_close(screen_stats);
    }
    if (screen_messages != NULL) {
        nge_close(screen_messages);
    }
    if (screen_panel != NULL) {
        nge_close(screen_panel);
    }
    if (help != NULL) {
        nge_close(help);
    }
    message_line_used = 0;
    screen_stats = nge_open_tilemap(&stats_area, 0);
    screen_messages = nge_open_tilemap(&message_area, 0);
    screen_panel = nge_open_tilemap(&screen_map_view, 0);
    help = nge_open_tilemap(&help_area, 0);
    nge_util_assert(help != NULL);   /* streams are handed out in turn: if one failed, the last did */

    nge_tm_clip(1, screen_stats);
    nge_tm_clip(1, screen_panel);
    nge_tm_clip(1, help);
    nge_tm_cls(screen_stats);
    nge_tm_cls(screen_messages);
    nge_tm_cls(help);
}

void screen_draw_stats(uint16_t turn, uint8_t hp, uint8_t hp_max, uint8_t ac, uint8_t status)
{
    nge_tm_cls(screen_stats);
    nge_tm_ink(NGE_INK_YELLOW, screen_stats);
    nge_puts("Name: ", screen_stats);
    nge_puts(screen_name, screen_stats);
    nge_puts("\n\n\n", screen_stats);       /* the hit points' row: screen_show_hp() */
    nge_tm_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("Str      16\n", screen_stats);
    nge_puts("\n", screen_stats);          /* the armour class's row: screen_show_ac() */
    nge_puts("Gold      0\n\n", screen_stats);
    nge_tm_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Depth     1\n", screen_stats);
    screen_show_hp(hp, hp_max);
    screen_show_ac(ac);
    screen_show_turn(turn);
    screen_show_status(status);
}

void screen_show_hp(uint8_t hp, uint8_t hp_max)
{
    nge_tm_at(STATS_ROW_HP, 0, screen_stats);
    nge_tm_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("HP ", screen_stats);
    nge_putu16w(hp, 4, ' ', screen_stats);
    nge_putc('/', screen_stats);
    nge_putu16w(hp_max, 3, ' ', screen_stats);
}

void screen_show_ac(uint8_t ac)
{
    nge_tm_at(STATS_ROW_AC, 0, screen_stats);
    nge_tm_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("AC ", screen_stats);
    nge_putu16w(ac, 8, ' ', screen_stats);
}

void screen_show_status(uint8_t status)
{
    nge_tm_at(STATS_ROW_STATUS, 0, screen_stats);
    nge_tm_clreol(screen_stats);
    if (status & STATUS_POISONED) {
        nge_tm_ink(NGE_INK_RED, screen_stats);
        nge_puts("Poisoned", screen_stats);
    }
}

void screen_show_turn(uint16_t turn)
{
    nge_tm_at(STATS_ROW_TURN, 0, screen_stats);
    nge_tm_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Turn ", screen_stats);
    nge_putu16w(turn, 6, ' ', screen_stats);
}

/*
 * The new line comes before each message, not after it: the newest message
 * stays on the bottom line, and all the window's lines show messages
 */
void screen_message_line(void)
{
    if (message_line_used) {
        nge_putc(NGE_CTRL_LF, screen_messages);   /* scrolls once at the bottom */
    }
    message_line_used = 1;
}

void screen_help(const char *text)
{
    nge_tm_cls(help);
    nge_tm_ink(NGE_INK_CYAN, help);
    nge_puts(text, help);
}

void screen_message(const char *text)
{
    screen_message_line();
    nge_tm_ink(NGE_INK_WHITE, screen_messages);
    nge_puts(text, screen_messages);
}
