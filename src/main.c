#include <stddef.h>
#include <arch/zxn.h>

#include "output/nge_cspect_stream.h"
#include "hw/nge_frame.h"
#include "hw/nge_hw.h"
#include "hw/nge_mmu.h"
#include "hw/nge_palette.h"
#include "map/nge_map.h"
#include "output/nge_print.h"
#include "output/nge_printf.h"
#include "sprite/nge_sprite.h"
#include "tilemap/nge_tilemap.h"
#include "output/nge_tm_stream.h"
#include "output/nge_uart_stream.h"
#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_ecs.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"

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

/* Demo map: 80 x 48 tiles, resident in slot 1 ($2000) with the ROM swapped out */
#define DEMO_MAP_SLOT    1
#define DEMO_MAP_PAGE    100
#define DEMO_MAP_W       80
#define DEMO_MAP_H       48
#define WALL_TILE        2       /* a game graphics tile (palette bank 0) */

/* The demo's sprites are entities in area 0, drawn by the render system */
#define DEMO_AREA        NGE_OWNER_AREA(0)
#define WALK_ROW         20      /* map row the walker's head is on */
#define WALK_FRAME_RATE  8       /* frames per animation step */
#define SPRITE_SIZE      16

/* Render layers: higher layers are drawn on top */
#define LAYER_SCENERY    0
#define LAYER_ITEMS      1
#define LAYER_PLAYER     2

static nge_map_t map;

/*
 * Fill the demo map: walls round the edge, and digit columns every 10 tiles
 * (the tens of the column number) with rows of dashes, coloured by row, so
 * camera movement is easy to follow. Tile 0 is transparent.
 */
static void build_demo_map(nge_tile_t *tiles)
{
    uint8_t x;
    uint8_t y;
    nge_tile_t *t;

    for (y = 0; y < DEMO_MAP_H; y++) {
        t = tiles + (uint16_t)y * NGE_MAP_STRIDE;
        for (x = 0; x < DEMO_MAP_W; x++, t++) {
            t->tile_attr = (uint8_t)((NGE_INK_WHITE + (y >> 2) % 4) << 4);
            if (x == 0 || y == 0 || x == DEMO_MAP_W - 1 || y == DEMO_MAP_H - 1) {
                t->tile_id = WALL_TILE;
                t->tile_attr = 0;
            } else if (x % 10 == 0) {
                t->tile_id = '0' + x / 10;
            } else if (y % 4 == 0) {
                t->tile_id = '-';
            } else {
                t->tile_id = 0;
            }
        }
    }
}

/* Create an entity in the demo area at tile (x, y), drawn with pattern 0 */
static nge_entity_id_t demo_entity(uint8_t x, uint8_t y, uint8_t flags, uint8_t mode, uint8_t layer)
{
    nge_entity_id_t id = nge_entity_create();

    nge_owner_set(id, DEMO_AREA);
    nge_location_set(id, x, y);
    nge_render_add(id, 0, flags, mode, layer);
    return id;
}

/* Load the demo sprite graphics: the loaders map their page into slot 7. */
static void load_sprite_graphics(void)
{
    nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);
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
    int8_t walk_dx = 1;
    nge_entity_id_t walker;
    uint16_t px;

    /* Disables interrupts and every interrupt source first (see nge_hw.h) */
    nge_hw_init();

    /* Send asserts and debug output to the CSpect console (NULL on hardware) */
    nge_util_set_debug_stream(nge_open_cspect_debug());

    /*
     * The map lives in slot 1 for as long as it is used, replacing the upper
     * 8K of the ROM. (Slot 0 keeps the ROM: CSpect's debug output needs it.)
     */
    nge_mmu_map(DEMO_MAP_SLOT, DEMO_MAP_PAGE);

    nge_util_debugf("jetset start 3, %u streams\n", NGE_FILE_MAX);
    nge_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, 4);
    nge_tm_load_ink_palette();

    nge_tilemap_init();
    nge_tilemap_clear(&empty_tile_entry);
    nge_tilemap_set_scroll(0, 0);

    /* The map is shown in the game area, below the text windows */
    build_demo_map((nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT));
    nge_map_init(&map, (nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT),
                 DEMO_MAP_W, DEMO_MAP_H, &game_area);

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

    /* Sprites: graphics come from page 101, mapped in only while loading */
    nge_sprite_init();
    load_sprite_graphics();

    /* Sprites are cut off at the edges of the map's window */
    nge_map_clip_sprites(&map);

    /* Entities: the render system draws those in the area shown by the map */
    nge_ecs_init();     /* ECS data page into slot 0; makes every entity free */
    nge_render_init();
    nge_render_set_view(DEMO_AREA, &map);
    demo_entity(4, WALK_ROW, 0, 0, LAYER_ITEMS);
    /* Double size, two rows up so it stands on the same floor */
    demo_entity(30, WALK_ROW - 2, 0, NGE_SPRITE_SCALE_X2 | NGE_SPRITE_SCALE_Y2, LAYER_SCENERY);
    /* Palette offset 1 recolours the same pattern; mirrored */
    demo_entity(50, WALK_ROW, NGE_SPRITE_PALETTE(1) | NGE_SPRITE_MIRROR_X, 0, LAYER_ITEMS);
    /* The walker: patterns 0 and 1, on top of everything */
    walker = demo_entity(1, WALK_ROW, 0, 0, LAYER_PLAYER);
    nge_render_set_anim(walker, 0, 2, WALK_FRAME_RATE);

    nge_util_info("init done");

    while (1)
    {
        nge_frame_wait_vblank();
        /* First thing in the blanking period, so nothing changes mid-frame */
        nge_render_upload();
        nge_map_update(&map);
        nge_ecs_check_guard();

        /* Walk until the tile ahead is a wall, then turn round */
        px = NGE_LOCATION_PX(walker);
        if (NGE_MAP_TILE(&map, (walk_dx > 0 ? px + SPRITE_SIZE : px - 1)
                               / NGE_MAP_TILE_PIXELS, WALK_ROW).tile_id == WALL_TILE) {
            walk_dx = -walk_dx;
            /* The pattern faces right; mirror it when walking left (only when turning) */
            nge_render_set_flags(walker, walk_dx < 0 ? NGE_SPRITE_MIRROR_X : 0);
        }
        nge_location_move(walker, walk_dx, 0);

        /* The camera follows the walker a tile at a time */
        nge_map_centre_camera(&map, (NGE_LOCATION_PX(walker) + SPRITE_SIZE / 2) / NGE_MAP_TILE_PIXELS,
                              WALK_ROW);

        /* Build the sprites for the next frame */
        nge_render_update();
    }
}
