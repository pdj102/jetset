/* nge_map.c: camera, dirty flag, drawing, tile access and screen coordinates. */
#include "test.h"

#include <string.h>

#include <arch/zxn.h>
#include <stdlib.h>

#include "map/nge_map.h"

#define MARK_ID   0xEE   /* tilemap cells outside the window keep this */

/* Distinct tile for every map position: IDs 0-79 (64 and up set the hardware ID's bit 8), odd and even attributes */
static void fill_map(void)
{
    uint16_t i;

    for (i = 0; i < NGE_MAP_STRIDE * NGE_MAP_HEIGHT_MAX; i++) {
        nge_map_tiles[i].tile_id = (uint8_t)(i % NGE_MAP_STRIDE);
        nge_map_tiles[i].tile_attr = (uint8_t)(i / NGE_MAP_STRIDE);
    }
}

static void mark_tilemap(void)
{
    int i;

    for (i = 0; i < NGE_TILEMAP_WIDTH * NGE_TILEMAP_HEIGHT; i++) {
        test_tilemap[i].tile_id = MARK_ID;
        test_tilemap[i].tile_attr = MARK_ID;
    }
}

/*
 * The hardware tile at window offset (dx, dy) shows: quarter (dx & 1, dy & 1)
 * of the map tile under it, hardware ID 4m + quarter (9 bits: the low 8 in
 * the ID, bit 8 in attribute bit 0) and the map tile's other attribute bits
 */
static int shows(const nge_tile_t *t, int dx, int dy)
{
    const nge_tile_t *m = &nge_map_tiles[(nge_map.cam_y + dy / 2) * NGE_MAP_STRIDE + nge_map.cam_x + dx / 2];
    int hw = m->tile_id * 4 + (dy & 1) * 2 + (dx & 1);

    return t->tile_id == (hw & 0xFF) && t->tile_attr == ((m->tile_attr & 0xFE) | (hw >> 8));
}

/*
 * The window shows the view at the camera, with a map tile column and row
 * more (two hardware tiles) where the map has them and the tilemap has room
 * (for a fine offset), and nothing else changed
 */
static int tilemap_matches(void)
{
    int x;
    int y;
    int x2 = nge_map.window.x2;
    int y2 = nge_map.window.y2;

    if (nge_map.cam_x + nge_map.view_w < nge_map.width && x2 + 2 < NGE_TILEMAP_WIDTH) {
        x2 += 2;
    }
    if (nge_map.cam_y + nge_map.view_h < nge_map.height && y2 + 2 < NGE_TILEMAP_HEIGHT) {
        y2 += 2;
    }
    for (y = 0; y < NGE_TILEMAP_HEIGHT; y++) {
        for (x = 0; x < NGE_TILEMAP_WIDTH; x++) {
            const nge_tile_t *t = &test_tilemap[y * NGE_TILEMAP_WIDTH + x];
            int in = x >= nge_map.window.x1 && x <= x2 && y >= nge_map.window.y1 && y <= y2;

            if (in) {
                if (!shows(t, x - nge_map.window.x1, y - nge_map.window.y1)) {
                    return 0;
                }
            } else if (t->tile_id != MARK_ID || t->tile_attr != MARK_ID) {
                return 0;
            }
        }
    }
    return 1;
}

static int clamp_model(int v, int max)
{
    return v < 0 ? 0 : (v > max ? max : v);
}

/* Random camera operations on random maps, against a reference model */
static void random_camera(void)
{
    nge_rect_t w;
    int run;
    int op;
    int ok = 1;

    srand(12345);
    for (run = 0; run < 300 && ok; run++) {
        int vw;
        int vh;
        int mw;
        int mh;

        /* Whole map tiles: an even number of hardware tiles each way */
        w.x1 = rand() % (NGE_TILEMAP_WIDTH - 1);
        vw = 1 + rand() % ((NGE_TILEMAP_WIDTH - w.x1) / 2);
        w.x2 = w.x1 + vw * 2 - 1;
        w.y1 = rand() % (NGE_TILEMAP_HEIGHT - 1);
        vh = 1 + rand() % ((NGE_TILEMAP_HEIGHT - w.y1) / 2);
        w.y2 = w.y1 + vh * 2 - 1;
        if (vh > NGE_MAP_HEIGHT_MAX) {
            continue;
        }
        mw = vw + rand() % (NGE_MAP_STRIDE - vw + 1);
        mh = vh + rand() % (NGE_MAP_HEIGHT_MAX - vh + 1);

        nge_map_init((uint8_t)mw, (uint8_t)mh, &w);
        fill_map();
        mark_tilemap();
        nge_map_update();
        ok = ok && tilemap_matches() && !nge_map.dirty;

        for (op = 0; op < 40 && ok; op++) {
            int ex = nge_map.cam_x;
            int ey = nge_map.cam_y;
            int a = rand() % 256;
            int b = rand() % 256;
            uint8_t moved;

            switch (rand() % 3) {
            case 0:
                moved = nge_map_set_camera((uint8_t)a, (uint8_t)b);
                ex = clamp_model(a, mw - vw);
                ey = clamp_model(b, mh - vh);
                break;
            case 1:
                a = (int8_t)a;
                b = (int8_t)b;
                moved = nge_map_move_camera((int8_t)a, (int8_t)b);
                ex = clamp_model(ex + a, mw - vw);
                ey = clamp_model(ey + b, mh - vh);
                break;
            default:
                moved = nge_map_centre_camera((uint8_t)a, (uint8_t)b);
                ex = clamp_model(a - vw / 2, mw - vw);
                ey = clamp_model(b - vh / 2, mh - vh);
                break;
            }
            ok = ok && nge_map.cam_x == ex && nge_map.cam_y == ey;
            ok = ok && moved == nge_map.dirty;
            if (nge_map.dirty) {
                mark_tilemap();   /* a draw at the map's edge leaves the last extra column or row: under the panels */
            }
            nge_map_update();
            ok = ok && tilemap_matches() && !nge_map.dirty;
        }
    }
    CHECK(ok);
}

int main(void)
{
    static const nge_rect_t window = { .x1 = 2, .x2 = 33, .y1 = 22, .y2 = 29 };   /* 32 x 8: a view of 16 x 4 tiles */
    static const nge_rect_t full = { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 31 };
    static const nge_tile_t gem = { .tile_id = 0x49, .tile_attr = 0x10 };   /* 73: hardware tiles 292-295 */
    uint16_t sx;
    uint16_t sy;

    test_init();
    fill_map();
    memset(nge_map_blocks, NGE_BLOCK_SOLID, sizeof(nge_map_blocks));

    /* ---- init: an empty map, whatever was there ---- */
    nge_map_init(60, 40, &window);
    CHECK(nge_map_tiles[0].tile_id == 0 && nge_map_tiles[0].tile_attr == 0);
    CHECK(nge_map_tiles[NGE_MAP_STRIDE * NGE_MAP_HEIGHT_MAX - 1].tile_id == 0);
    CHECK(nge_map_blocks[0] == NGE_BLOCK_NONE && nge_map_blocks[255] == NGE_BLOCK_NONE);
    fill_map();
    CHECK(nge_map.view_w == 16 && nge_map.view_h == 4);
    CHECK(nge_map.cam_x == 0 && nge_map.cam_y == 0 && nge_map.dirty);
    /* Page 51 is mapped into slot 1 */
    CHECK(test_regs[REG_MMU0 + NGE_MAP_SLOT] == NGE_MAP_PAGE && NGE_MAP_PAGE == 51 && NGE_MAP_SLOT == 1);

    /* ---- drawing: only when dirty, only the window ---- */
    mark_tilemap();
    nge_map_update();
    CHECK(tilemap_matches());
    CHECK(!nge_map.dirty);
    mark_tilemap();
    nge_map_update();   /* not dirty: nothing drawn */
    CHECK(test_tilemap[22 * NGE_TILEMAP_WIDTH + 2].tile_id == MARK_ID);
    nge_map_draw();     /* draw always draws */
    CHECK(tilemap_matches());

    /* ---- camera clamping ---- */
    CHECK(nge_map_set_camera(10, 5) == 1 && nge_map.cam_x == 10 && nge_map.cam_y == 5 && nge_map.dirty);
    nge_map_update();
    CHECK(nge_map_set_camera(10, 5) == 0 && !nge_map.dirty);        /* no move, no redraw */
    CHECK(nge_map_set_camera(255, 255) == 1);
    CHECK(nge_map.cam_x == 60 - 16 && nge_map.cam_y == 40 - 4);                /* right and bottom edges */
    CHECK(nge_map_move_camera(1, 1) == 0);                     /* already at the edge */
    CHECK(nge_map_move_camera(-128, -128) == 1 && nge_map.cam_x == 0 && nge_map.cam_y == 0);
    CHECK(nge_map_move_camera(-1, 0) == 0);
    CHECK(nge_map_move_camera(3, 2) == 1 && nge_map.cam_x == 3 && nge_map.cam_y == 2);
    CHECK(nge_map_centre_camera(30, 20) == 1 && nge_map.cam_x == 30 - 8 && nge_map.cam_y == 20 - 2);
    CHECK(nge_map_centre_camera(1, 1) == 1 && nge_map.cam_x == 0 && nge_map.cam_y == 0);
    CHECK(nge_map_centre_camera(59, 39) == 1 && nge_map.cam_x == 44 && nge_map.cam_y == 36);
    mark_tilemap();
    nge_map_update();
    CHECK(tilemap_matches());

    /* A map exactly the size of the view cannot move */
    nge_map_init(16, 4, &window);
    nge_map_update();
    CHECK(nge_map_move_camera(5, 5) == 0 && nge_map_centre_camera(15, 3) == 0);
    CHECK(!nge_map.dirty);

    /* The whole tilemap as a window */
    nge_map_init(NGE_MAP_STRIDE, NGE_MAP_HEIGHT_MAX, &full);
    nge_map_set_camera(255, 255);
    CHECK(nge_map.view_w == 20 && nge_map.view_h == 16);
    CHECK(nge_map.cam_x == NGE_MAP_STRIDE - 20 && nge_map.cam_y == NGE_MAP_HEIGHT_MAX - 16);
    mark_tilemap();
    nge_map_update();
    CHECK(tilemap_matches());

    /* ---- invalidate: the next update redraws over whatever is in the window ---- */
    test_tilemap[22 * NGE_TILEMAP_WIDTH + 2].tile_id = 0xEE;
    nge_map_update();
    CHECK(!nge_map.dirty && test_tilemap[22 * NGE_TILEMAP_WIDTH + 2].tile_id == 0xEE);   /* not dirty: left */
    nge_map_invalidate();
    CHECK(nge_map.dirty);
    nge_map_update();
    CHECK(!nge_map.dirty && tilemap_matches());

    /* ---- tile access ---- */
    nge_map_init(60, 40, &window);
    fill_map();
    nge_map_set_camera(10, 10);
    nge_map_update();
    CHECK(nge_map_get(7, 3)->tile_id == 7 && nge_map_get(7, 3)->tile_attr == 3);
    CHECK(&NGE_MAP_TILE(59, 39) == nge_map_get(59, 39));
    CHECK(&NGE_MAP_TILE(0, 1) == &nge_map_tiles[NGE_MAP_STRIDE]);

    nge_map_set(0, 0, &gem);              /* off-screen: changed, no redraw */
    CHECK(nge_map_tiles[0].tile_id == 0x49 && !nge_map.dirty);
    nge_map_set(9, 12, &gem);             /* just left of the view (columns 10-25, rows 10-13) */
    CHECK(!nge_map.dirty);
    nge_map_set(26, 12, &gem);            /* just right of the view */
    CHECK(!nge_map.dirty);
    nge_map_set(25, 13, &gem);            /* bottom-right corner of the view */
    CHECK(nge_map.dirty);
    nge_map_update();
    /* Its four hardware tiles: 292-295, bit 8 in attribute bit 0 */
    CHECK(test_tilemap[28 * NGE_TILEMAP_WIDTH + 32].tile_id == (292 & 0xFF) &&
          test_tilemap[28 * NGE_TILEMAP_WIDTH + 32].tile_attr == 0x11);
    CHECK(test_tilemap[28 * NGE_TILEMAP_WIDTH + 33].tile_id == (293 & 0xFF));
    CHECK(test_tilemap[29 * NGE_TILEMAP_WIDTH + 32].tile_id == (294 & 0xFF));
    CHECK(test_tilemap[29 * NGE_TILEMAP_WIDTH + 33].tile_id == (295 & 0xFF) &&
          test_tilemap[29 * NGE_TILEMAP_WIDTH + 33].tile_attr == 0x11);
    nge_map_set(25, 14, &gem);            /* just below the view */
    CHECK(!nge_map.dirty);
    nge_map_set(10, 10, &gem);            /* top-left corner of the view */
    CHECK(nge_map.dirty);

    /* ---- screen coordinates: camera (10,10), 16 pixels a tile; window at hardware tile (2,22) ---- */
    CHECK(nge_map_to_screen(160, 160, &sx, &sy) == 1);     /* view's top-left pixel */
    CHECK(sx == 16 && sy == 176);
    CHECK(nge_map_to_screen(160 + 255, 160 + 63, &sx, &sy) == 1);  /* bottom-right pixel: 256 x 64 */
    CHECK(sx == 16 + 255 && sy == 176 + 63);
    CHECK(nge_map_to_screen(160 + 256, 170, &sx, &sy) == 0);       /* right of the view */
    CHECK(nge_map_to_screen(170, 160 + 64, &sx, &sy) == 0);        /* below the view */
    /* Left of the view: placed just off the left of the window (9-bit wrap) */
    CHECK(nge_map_to_screen(150, 160, &sx, &sy) == 0);
    CHECK(sx == 6);
    nge_map_init(60, 40, &full);
    nge_map_set_camera(4, 0);
    CHECK(nge_map_to_screen(56, 0, &sx, &sy) == 0);
    CHECK((sx & 0x1FF) == 512 - 8);

    /* ---- sprite clip window ---- */
    nge_map_init(60, 40, &window);
    test_reg_log_count = 0;
    nge_map_clip_sprites();
    CHECK(test_reg_log_reg[1] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[1] == 2 * 4);
    CHECK(test_reg_log_val[2] == 33 * 4 + 3);
    CHECK(test_reg_log_val[3] == 22 * 8 && test_reg_log_val[4] == 29 * 8 + 7);

    /* ---- randomised camera and drawing ---- */
    fill_map();
    random_camera();

    /* ---- block table: flags by tile ID ---- */
    CHECK(sizeof(nge_map_blocks) + sizeof(nge_map) + sizeof(nge_map_tiles) <= 8192);   /* one page */
    nge_map_init(60, 40, &window);
    fill_map();                                              /* tile ID = column */
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    CHECK(nge_map_blocks[0] == NGE_BLOCK_SOLID && nge_map_blocks[255] == NGE_BLOCK_SOLID);
    CHECK(nge_map_tiles[NGE_MAP_STRIDE * NGE_MAP_HEIGHT_MAX - 1].tile_id == NGE_MAP_STRIDE - 1);   /* tiles untouched */
    nge_map_set_block(7, NGE_BLOCK_NONE);
    nge_map_set_block(9, NGE_BLOCK_NO_FLOOR | NGE_BLOCK_USER);
    CHECK(nge_map_block(7, 0) == NGE_BLOCK_NONE && nge_map_block(7, 39) == NGE_BLOCK_NONE);
    CHECK(nge_map_block(9, 5) == (NGE_BLOCK_NO_FLOOR | NGE_BLOCK_USER));
    CHECK(nge_map_block(8, 5) == NGE_BLOCK_SOLID && NGE_MAP_BLOCK(9, 5) == nge_map_blocks[9]);
    /* Outside the map: the edge, whatever the table says */
    CHECK(nge_map_block(60, 0) == NGE_BLOCK_EDGE && nge_map_block(0, 40) == NGE_BLOCK_EDGE);
    CHECK(nge_map_block(255, 255) == NGE_BLOCK_EDGE);
    /* Init again (a new level): the table and the tiles are cleared */
    nge_map_init(NGE_MAP_STRIDE, NGE_MAP_HEIGHT_MAX, &window);
    CHECK(nge_map_block(9, NGE_MAP_HEIGHT_MAX - 1) == NGE_BLOCK_NONE);
    CHECK(nge_map_get(9, 5)->tile_id == 0);
    CHECK(nge_map_block(NGE_MAP_STRIDE, 0) == NGE_BLOCK_EDGE);
    nge_map_init(60, 40, &window);

    /* ---- the range test: dx*dx + dy*dy <= r*r + r, from (40, 20) ---- */
    CHECK(nge_map_within(40, 20, 40, 20, 0));                                   /* r = 0: the tile itself */
    CHECK(!nge_map_within(40, 20, 41, 20, 0));
    CHECK(nge_map_within(40, 20, 41, 21, 1) && nge_map_within(40, 20, 39, 19, 1));   /* r = 1: the 8 neighbours */
    CHECK(nge_map_within(40, 20, 41, 19, 1) && nge_map_within(40, 20, 40, 21, 1));
    CHECK(!nge_map_within(40, 20, 42, 20, 1));
    CHECK(nge_map_within(40, 20, 45, 20, 5));                                   /* r = 5: limit 30 */
    CHECK(nge_map_within(40, 20, 45, 22, 5));                                   /* 29 */
    CHECK(nge_map_within(40, 20, 44, 23, 5));                                   /* 25 */
    CHECK(!nge_map_within(40, 20, 46, 20, 5));                                  /* 36 */
    CHECK(!nge_map_within(40, 20, 45, 23, 5));                                  /* 34 */
    CHECK(!nge_map_within(40, 20, 44, 24, 5));                                  /* 32 */
    CHECK(nge_map_within(35, 18, 40, 20, 5) && nge_map_within(40, 20, 35, 18, 5));   /* either order, left and above */
    CHECK(!nge_map_within(36, 17, 40, 20, 4) && !nge_map_within(40, 20, 36, 17, 4)); /* 25 > 20 */
    CHECK(nge_map_within(0, 0, NGE_MAP_STRIDE - 1, NGE_MAP_HEIGHT_MAX - 1, 255));     /* any tile, at 255 */
    CHECK(nge_map_within(NGE_MAP_STRIDE - 1, 0, 0, NGE_MAP_HEIGHT_MAX - 1, 255));

    /* ---- the pixel camera: a tile and a fine offset (window 32 x 8 at (2, 22): room right and below) ---- */
    nge_map_init(60, 40, &window);
    fill_map();
    nge_map_update();
    CHECK(nge_map_set_camera_px(163, 85) == 1);
    CHECK(nge_map.cam_x == 10 && nge_map.fine_x == 3 && nge_map.cam_y == 5 && nge_map.fine_y == 5);
    CHECK(NGE_MAP_CAMERA_PX() == 163 && NGE_MAP_CAMERA_PY() == 85);
    CHECK(nge_map.dirty && nge_map.scroll_dirty);
    mark_tilemap();
    nge_map_update();
    CHECK(!nge_map.dirty && !nge_map.scroll_dirty);
    CHECK(test_regs[0x30] == 3 && test_regs[0x2F] == 0 && test_regs[0x31] == 5);   /* the tilemap's scroll */
    CHECK(tilemap_matches());   /* the view, and the column and row past it */
    /* The column past the view: map tile (26, 5) in hardware columns 34-35; the row: (10, 9) in rows 30-31 */
    CHECK(test_tilemap[22 * NGE_TILEMAP_WIDTH + 35].tile_id == 26 * 4 + 1);
    CHECK(test_tilemap[31 * NGE_TILEMAP_WIDTH + 2].tile_id == 10 * 4 + 2 && test_tilemap[31 * NGE_TILEMAP_WIDTH + 2].tile_attr == 8);
    /* Within the tile: only the scroll changes, up to 15 */
    CHECK(nge_map_set_camera_px(164, 85) == 1 && !nge_map.dirty && nge_map.scroll_dirty);
    nge_map_update();
    CHECK(test_regs[0x30] == 4);
    CHECK(nge_map_set_camera_px(164, 85) == 0 && !nge_map.scroll_dirty);   /* no move */
    CHECK(nge_map_set_camera_px(175, 95) == 1 && !nge_map.dirty && nge_map.fine_x == 15 && nge_map.fine_y == 15);
    CHECK(nge_map_set_camera_px(176, 96) == 1 && nge_map.dirty && nge_map.cam_x == 11 && nge_map.fine_x == 0 &&
          nge_map.cam_y == 6 && nge_map.fine_y == 0);
    /* Clamped: the view's far edge on the map's, with no fine offset there: (60 - 16) * 16, (40 - 4) * 16 */
    CHECK(nge_map_set_camera_px(5000, 5000) == 1);
    CHECK(nge_map.cam_x == 44 && nge_map.fine_x == 0 && nge_map.cam_y == 36 && nge_map.fine_y == 0);
    mark_tilemap();
    nge_map_update();
    CHECK(tilemap_matches());   /* no column or row past the map's edge */
    CHECK(test_tilemap[22 * NGE_TILEMAP_WIDTH + 34].tile_id == MARK_ID);
    /* The tile functions set the fine offset to 0 */
    nge_map_set_camera_px(163, 85);
    nge_map_update();
    CHECK(nge_map_set_camera(10, 5) == 1 && nge_map.fine_x == 0 && nge_map.fine_y == 0);
    CHECK(!nge_map.dirty && nge_map.scroll_dirty);
    nge_map_update();
    CHECK(test_regs[0x30] == 0 && test_regs[0x31] == 0);
    /* Screen coordinates through the fine offset */
    nge_map_set_camera_px(163, 85);
    CHECK(nge_map_to_screen(173, 89, &sx, &sy) && sx == 2 * 8 + 10 && sy == 22 * 8 + 4);
    CHECK(!nge_map_to_screen(162, 89, &sx, &sy) && sx == (uint16_t)(2 * 8 - 1));   /* a pixel left of the view */
    /* Centring on a pixel: as centring on a tile (8 x 2 tiles to the view's top-left), between tiles too */
    CHECK(nge_map_centre_camera_px(30 * 16, 20 * 16) == 1 && nge_map.cam_x == 30 - 8 && nge_map.fine_x == 0 &&
          nge_map.cam_y == 20 - 2 && nge_map.fine_y == 0);
    CHECK(nge_map_centre_camera_px(30 * 16 + 3, 20 * 16 - 2) == 1 && NGE_MAP_CAMERA_PX() == (30 - 8) * 16 + 3 &&
          NGE_MAP_CAMERA_PY() == (20 - 2) * 16 - 2);
    /* Clamped at each edge */
    CHECK(nge_map_centre_camera_px(5, 7) == 1 && NGE_MAP_CAMERA_PX() == 0 && NGE_MAP_CAMERA_PY() == 0);
    CHECK(nge_map_centre_camera_px(8 * 16 + 1, 2 * 16 + 1) == 1 && NGE_MAP_CAMERA_PX() == 1 && NGE_MAP_CAMERA_PY() == 1);
    CHECK(nge_map_centre_camera_px(59 * 16 + 15, 39 * 16 + 15) == 1 && nge_map.cam_x == 44 && nge_map.fine_x == 0 &&
          nge_map.cam_y == 36 && nge_map.fine_y == 0);
    /* A fine offset needs room past the window: none with the whole tilemap */
    nge_map_init(60, 40, &full);
    TEST_EXPECT_HALT(nge_map_set_camera_px(3, 0));
    CHECK(test_halt_count == 1);
    CHECK(nge_map_set_camera_px(16, 0) == 1 && nge_map.cam_x == 1);   /* whole tiles are fine */
    /* Room for one hardware tile past the window isn't enough: a map tile needs two */
    {
        static const nge_rect_t tight = { .x1 = 1, .x2 = 38, .y1 = 0, .y2 = 29 };   /* column 39 free */

        nge_map_init(60, 40, &tight);
        TEST_EXPECT_HALT(nge_map_set_camera_px(3, 0));
        CHECK(test_halt_count == 1);
        nge_map_init(60, 40, &window);
        CHECK(nge_map_set_camera_px(3, 0) == 1);
    }

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(nge_map_get(60, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_set(0, 40, &gem));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_init(15, 40, &window));       /* narrower than the view */
    CHECK(test_halt_count == 1);
    {
        static const nge_rect_t odd_w = { .x1 = 0, .x2 = 2, .y1 = 0, .y2 = 1 };
        static const nge_rect_t odd_h = { .x1 = 0, .x2 = 1, .y1 = 0, .y2 = 2 };

        TEST_EXPECT_HALT(nge_map_init(60, 40, &odd_w));    /* not whole map tiles */
        CHECK(test_halt_count == 1);
        TEST_EXPECT_HALT(nge_map_init(60, 40, &odd_h));
        CHECK(test_halt_count == 1);
    }
    TEST_EXPECT_HALT(nge_map_init(NGE_MAP_STRIDE + 1, 40, &window));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_init(60, NGE_MAP_HEIGHT_MAX + 1, &window));
    CHECK(test_halt_count == 1);

    return test_summary("map");
}
