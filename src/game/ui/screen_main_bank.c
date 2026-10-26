/*
 * The main screen's set-up and its stats: banked in page 116 with the other
 * screens. They pass their literals only to common code (the output path).
 */
#ifdef __SDCC
#pragma codeseg PAGE_116
#pragma constseg PAGE_116
#endif

#include "game/ui/screen_main.h"

#include <stddef.h>
#include <arch/zxn.h>

#include "core/nge_util.h"
#include "hw/nge_palette.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "layer2/nge_layer2.h"
#include "tilemap/nge_tilemap.h"
#include "ui/nge_panel.h"

#include "data/tile_data.h"
#include "game/components/attrs.h"
#include "game/components/stats.h"
#include "game/ui/screen_internal.h"

/* The main screen's panels, by PANEL_* */
#define PANEL_MAP       0   /* a hole: the map and the sprites show through */
#define PANEL_STATS     1
#define PANEL_MESSAGES  2
#define PANEL_HELP      3
#define PANEL_COUNT     4

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_MAP]      = { { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 }, NGE_LAYER2_TRANSPARENT, NGE_PANEL_CLIP },
    [PANEL_STATS]    = { { .x1 = 28, .x2 = 39, .y1 = 0, .y2 = 25 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_MESSAGES] = { { .x1 = 0, .x2 = 39, .y1 = 26, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_SCROLL },
    [PANEL_HELP]     = { { .x1 = 0, .x2 = 39, .y1 = SCREEN_HELP_ROW, .y2 = SCREEN_HELP_ROW }, SCREEN_PAPER,
                         NGE_PANEL_CLIP },
};

#define STATS_ROW_HP         2    /* stats window row of the hit points */
#define STATS_ROW_AC         3    /* ...of the armour class */
#define STATS_ROW_ABILITIES  5    /* ...of the first ability: one row each */
#define STATS_ROW_GOLD       12   /* ...of the gold and, under it, the depth */
#define STATS_ROW_TURN       14   /* ...of the turn count */
#define STATS_ROW_STATUS     16   /* ...of the statuses */

/* The abilities' names, by ABIL_* */
static const char ability_names[ABIL_COUNT][4] = { "Str", "Dex", "Con", "Int", "Wis", "Cha" };

/* Tile 0 is all pixel value 0, so it is fully transparent */
static const nge_tile_t empty_tile = {
    .tile_id = 0,
    .tile_attr = 0,
};

void screen_init(void) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t b;

    nge_util_assert(panels[PANEL_MAP].area.x2 == screen_map_view.x2 &&
                    panels[PANEL_MAP].area.y2 == screen_map_view.y2);

    /* The tilemap: the map's layer */
    /* The map's colours: its metatiles' banks, from the tileset (data/tile_data.h) */
    nge_palette_load9(TILE_PALETTE_PAGE, RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, TILE_PALETTE_ENTRIES);
    nge_tilemap_init();
    nge_tilemap_clear(&empty_tile);
    nge_tilemap_set_scroll(0, 0);

    /* Layer 2: every buffer transparent, buffer 0 shown, before the panels are drawn on it */
    nge_layer2_init();

    /* Called again, it starts afresh: close the streams it opened before */
    nge_close(screen_stats);
    nge_close(screen_messages);
    for (b = 0; b < 2; b++) {
        nge_close(screen_help_lines[b]);
        screen_help_lines[b] = NULL;
    }
    screen_depth = 0;
    screen_message_line_used = 0;
    screen_message_stream = NULL;
    nge_sprite_clear(screen_overlay, SCREEN_OVERLAY_COUNT);

    b = nge_panel_open_all(SCREEN_BUFFER_MAIN, panels, PANEL_COUNT, streams);
    nge_util_assert(b);
    screen_stats = streams[PANEL_STATS];
    screen_messages = streams[PANEL_MESSAGES];
    screen_help_lines[SCREEN_BUFFER_MAIN] = streams[PANEL_HELP];
}

void screen_draw_stats(void) NGE_BANKED
{
    nge_text_cls(screen_stats);
    nge_text_ink(NGE_INK_YELLOW, screen_stats);
    nge_puts("Name: ", screen_stats);
    nge_puts(screen_name, screen_stats);
    nge_text_at(STATS_ROW_GOLD, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("Gold      0", screen_stats);
}

void screen_show_depth(uint8_t depth) NGE_BANKED
{
    nge_text_at(STATS_ROW_GOLD + 1, 0, screen_stats);
    nge_text_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Depth ", screen_stats);
    nge_putu16w(depth, 5, ' ', screen_stats);
}

void screen_show_hp(uint8_t hp, uint8_t hp_max) NGE_BANKED
{
    nge_text_at(STATS_ROW_HP, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("HP ", screen_stats);
    nge_putu16w(hp, 4, ' ', screen_stats);
    nge_putc('/', screen_stats);
    nge_putu16w(hp_max, 3, ' ', screen_stats);
}

void screen_show_ac(uint8_t ac) NGE_BANKED
{
    nge_text_at(STATS_ROW_AC, 0, screen_stats);
    nge_text_ink(NGE_INK_WHITE, screen_stats);
    nge_puts("AC ", screen_stats);
    nge_putu16w(ac, 8, ' ', screen_stats);
}

void screen_show_abilities(const uint8_t *scores) NGE_BANKED
{
    uint8_t a;

    nge_text_ink(NGE_INK_WHITE, screen_stats);
    for (a = 0; a < ABIL_COUNT; a++) {
        nge_text_at((uint8_t)(STATS_ROW_ABILITIES + a), 0, screen_stats);
        nge_puts(ability_names[a], screen_stats);
        nge_putu16w(scores[a], 8, ' ', screen_stats);
    }
}

void screen_show_status(uint8_t status) NGE_BANKED
{
    nge_text_at(STATS_ROW_STATUS, 0, screen_stats);
    nge_text_clreol(screen_stats);
    if (status & STATUS_POISONED) {
        nge_text_ink(NGE_INK_RED, screen_stats);
        nge_puts("Poisoned", screen_stats);
    }
}

void screen_show_turn(uint16_t turn) NGE_BANKED
{
    nge_text_at(STATS_ROW_TURN, 0, screen_stats);
    nge_text_ink(NGE_INK_CYAN, screen_stats);
    nge_puts("Turn ", screen_stats);
    nge_putu16w(turn, 6, ' ', screen_stats);
}
