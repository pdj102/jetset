/*
 * The screen's layout and its windows: common memory. The windows are
 * Layer 2 streams (io/nge_l2_stream.h) over the tilemap, which holds only
 * the map.
 */
#include "game/screen.h"

#include <stddef.h>
#include <arch/zxn.h>

#include "core/nge_util.h"
#include "hw/nge_palette.h"
#include "io/nge_l2_stream.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "io/nge_tm_stream.h"
#include "layer2/nge_layer2.h"
#include "sprite/nge_sprite.h"
#include "tilemap/nge_tilemap.h"

#include "game/components/attrs.h"
#include "game/components/stats.h"

/* The map view is whole map tiles: 2 x 2 hardware tiles each (map/nge_map.h) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };
static const nge_rect_t stats_area = { .x1 = 28, .x2 = 39, .y1 = 0, .y2 = 25 };
static const nge_rect_t message_area = { .x1 = 0, .x2 = 39, .y1 = 26, .y2 = 30 };
static const nge_rect_t help_area = { .x1 = 0, .x2 = 39, .y1 = 31, .y2 = 31 };

NGE_FILE *screen_stats = NULL;
NGE_FILE *screen_messages = NULL;
NGE_FILE *screen_panel = NULL;
static NGE_FILE *help = NULL;
char screen_name[SCREEN_NAME_MAX + 1] = "Rogue";

#define STATS_ROW_HP         2    /* stats window row of the hit points */
#define STATS_ROW_AC         3    /* ...of the armour class */
#define STATS_ROW_ABILITIES  5    /* ...of the first ability: one row each */
#define STATS_ROW_GOLD       12   /* ...of the gold and, under it, the depth */
#define STATS_ROW_TURN       14   /* ...of the turn count */
#define STATS_ROW_STATUS     16   /* ...of the statuses */

/* The abilities' names, by ABIL_* */
static const char ability_names[ABIL_COUNT][4] = { "Str", "Dex", "Con", "Int", "Wis", "Cha" };

#define PAPER  0x00   /* the windows' background on Layer 2 (RRRGGGBB): black */

/* The message window's bottom line has text: the next message goes on a new line */
static uint8_t message_line_used;

/* The overlay sprites' records: common memory, so the upload can read them from any caller */
static nge_sprite_attr_t overlay[SCREEN_OVERLAY_COUNT];

/* Game graphics palette (bank 0); the map's text colours are banks 12-15 (nge_tm_load_ink_palette) */
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
    /* The tilemap: the map's layer */
    nge_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, 4);
    nge_tm_load_ink_palette();
    nge_tilemap_init();
    nge_tilemap_clear(&empty_tile);
    nge_tilemap_set_scroll(0, 0);

    /* Layer 2: the windows' layer, all transparent, before the windows are drawn on it */
    nge_layer2_init();

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
    nge_sprite_clear(overlay, SCREEN_OVERLAY_COUNT);
    screen_stats = nge_open_layer2(&stats_area, PAPER);
    screen_messages = nge_open_layer2(&message_area, PAPER);
    screen_panel = nge_open_layer2(&screen_map_view, PAPER);
    help = nge_open_layer2(&help_area, PAPER);
    nge_util_assert(help != NULL);   /* streams are handed out in turn: if one failed, the last did */

    nge_text_clip(1, screen_stats);
    nge_text_clip(1, screen_panel);
    nge_text_clip(1, help);
    nge_text_cls(screen_stats);
    nge_text_cls(screen_messages);
    nge_text_cls(help);
}

void screen_show_map(void)
{
    nge_layer2_fill(screen_map_view.x1, screen_map_view.y1, screen_map_view.x2, screen_map_view.y2,
                    NGE_LAYER2_TRANSPARENT);
}

void screen_draw_stats(void)
{
    nge_text_cls(screen_stats);
    nge_text_ink(NGE_INK_YELLOW, screen_stats);
    nge_puts("Name: ", screen_stats);
    nge_puts(screen_name, screen_stats);
    nge_text_at(STATS_ROW_GOLD, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("Gold      0", screen_stats);
}

void screen_show_depth(uint8_t depth)
{
    nge_text_at(STATS_ROW_GOLD + 1, 0, screen_stats);
    nge_text_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Depth ", screen_stats);
    nge_putu16w(depth, 5, ' ', screen_stats);
}

void screen_show_hp(uint8_t hp, uint8_t hp_max)
{
    nge_text_at(STATS_ROW_HP, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("HP ", screen_stats);
    nge_putu16w(hp, 4, ' ', screen_stats);
    nge_putc('/', screen_stats);
    nge_putu16w(hp_max, 3, ' ', screen_stats);
}

void screen_show_ac(uint8_t ac)
{
    nge_text_at(STATS_ROW_AC, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("AC ", screen_stats);
    nge_putu16w(ac, 8, ' ', screen_stats);
}

void screen_show_abilities(const uint8_t *scores)
{
    uint8_t a;

    nge_text_ink(NGE_INK_WHITE, screen_stats);
    for (a = 0; a < ABIL_COUNT; a++) {
        nge_text_at((uint8_t)(STATS_ROW_ABILITIES + a), 0, screen_stats);
        nge_puts(ability_names[a], screen_stats);
        nge_putu16w(scores[a], 8, ' ', screen_stats);
    }
}

void screen_show_status(uint8_t status)
{
    nge_text_at(STATS_ROW_STATUS, 0, screen_stats);
    nge_text_clreol(screen_stats);
    if (status & STATUS_POISONED) {
        nge_text_ink(NGE_INK_RED, screen_stats);
        nge_puts("Poisoned", screen_stats);
    }
}

void screen_show_turn(uint16_t turn)
{
    nge_text_at(STATS_ROW_TURN, 0, screen_stats);
    nge_text_ink(NGE_INK_CYAN, screen_stats);
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

void screen_overlay_set(uint8_t n, uint16_t x, uint16_t y, uint8_t pattern)
{
    nge_util_assert(n < SCREEN_OVERLAY_COUNT);

    nge_sprite_set(&overlay[n], x, y, pattern, 0, NGE_SPRITE_SCALE_1);
}

void screen_overlay_hide(uint8_t n)
{
    nge_util_assert(n < SCREEN_OVERLAY_COUNT);

    nge_sprite_clear(&overlay[n], 1);
}

void screen_overlay_upload(void)
{
    nge_sprite_upload(SCREEN_SPRITE_OVERLAY, overlay, SCREEN_OVERLAY_COUNT);
}

void screen_help(const char *text)
{
    nge_text_cls(help);
    nge_text_ink(NGE_INK_CYAN, help);
    nge_puts(text, help);
}

void screen_message(const char *text)
{
    screen_message_line();
    nge_text_ink(NGE_INK_WHITE, screen_messages);
    nge_puts(text, screen_messages);
}
