/* nge_map.c: camera, dirty flag, drawing, tile access and screen coordinates. */
#include "test.h"

#include <arch/zxn.h>
#include <stdlib.h>

#include "map/nge_map.h"

static nge_tile_t map_mem[NGE_MAP_BUFFER_BYTES / sizeof(nge_tile_t)];

#define MARK_ID   0xEE   /* tilemap cells outside the window keep this */

/* Distinct tile for every map position */
static void fill_map(void)
{
    uint16_t i;

    for (i = 0; i < NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT; i++) {
        map_mem[i].tile_id = (uint8_t)(i % NGE_MAP_STRIDE);
        map_mem[i].tile_attr = (uint8_t)(i / NGE_MAP_STRIDE);
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

/* The window shows the view at the camera, and nothing else changed */
static int tilemap_matches(const nge_map_t *m)
{
    int x;
    int y;

    for (y = 0; y < NGE_TILEMAP_HEIGHT; y++) {
        for (x = 0; x < NGE_TILEMAP_WIDTH; x++) {
            const nge_tile_t *t = &test_tilemap[y * NGE_TILEMAP_WIDTH + x];
            int in = x >= m->window.x1 && x <= m->window.x2 && y >= m->window.y1 && y <= m->window.y2;

            if (in) {
                const nge_tile_t *e = &map_mem[(m->cam_y + y - m->window.y1) * NGE_MAP_STRIDE
                                               + m->cam_x + x - m->window.x1];
                if (t->tile_id != e->tile_id || t->tile_attr != e->tile_attr) {
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
    nge_map_t m;
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

        w.x1 = rand() % NGE_TILEMAP_WIDTH;
        w.x2 = w.x1 + rand() % (NGE_TILEMAP_WIDTH - w.x1);
        w.y1 = rand() % NGE_TILEMAP_HEIGHT;
        w.y2 = w.y1 + rand() % (NGE_TILEMAP_HEIGHT - w.y1);
        vw = w.x2 - w.x1 + 1;
        vh = w.y2 - w.y1 + 1;
        if (vh > NGE_MAP_MAX_HEIGHT) {
            continue;
        }
        mw = vw + rand() % (NGE_MAP_STRIDE - vw + 1);
        mh = vh + rand() % (NGE_MAP_MAX_HEIGHT - vh + 1);

        nge_map_init(&m, map_mem, (uint8_t)mw, (uint8_t)mh, &w);
        mark_tilemap();
        nge_map_update(&m);
        ok = ok && tilemap_matches(&m) && !m.dirty;

        for (op = 0; op < 40 && ok; op++) {
            int ex = m.cam_x;
            int ey = m.cam_y;
            int a = rand() % 256;
            int b = rand() % 256;
            uint8_t moved;

            switch (rand() % 3) {
            case 0:
                moved = nge_map_set_camera(&m, (uint8_t)a, (uint8_t)b);
                ex = clamp_model(a, mw - vw);
                ey = clamp_model(b, mh - vh);
                break;
            case 1:
                a = (int8_t)a;
                b = (int8_t)b;
                moved = nge_map_move_camera(&m, (int8_t)a, (int8_t)b);
                ex = clamp_model(ex + a, mw - vw);
                ey = clamp_model(ey + b, mh - vh);
                break;
            default:
                moved = nge_map_centre_camera(&m, (uint8_t)a, (uint8_t)b);
                ex = clamp_model(a - vw / 2, mw - vw);
                ey = clamp_model(b - vh / 2, mh - vh);
                break;
            }
            ok = ok && m.cam_x == ex && m.cam_y == ey;
            ok = ok && moved == m.dirty;
            nge_map_update(&m);
            ok = ok && tilemap_matches(&m) && !m.dirty;
        }
    }
    CHECK(ok);
}

int main(void)
{
    static const nge_rect_t window = { .x1 = 2, .x2 = 33, .y1 = 22, .y2 = 30 };   /* 32 x 9 */
    static const nge_rect_t full = { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 31 };
    static const nge_tile_t gem = { .tile_id = 0x99, .tile_attr = 0x10 };
    nge_map_t m;
    uint16_t sx;
    uint16_t sy;

    test_init();
    fill_map();

    /* ---- init ---- */
    nge_map_init(&m, map_mem, 60, 40, &window);
    CHECK(m.view_w == 32 && m.view_h == 9);
    CHECK(m.cam_x == 0 && m.cam_y == 0 && m.dirty);
    CHECK(NGE_MAP_MAX_BYTES <= 8192);

    /* ---- drawing: only when dirty, only the window ---- */
    mark_tilemap();
    nge_map_update(&m);
    CHECK(tilemap_matches(&m));
    CHECK(!m.dirty);
    mark_tilemap();
    nge_map_update(&m);   /* not dirty: nothing drawn */
    CHECK(test_tilemap[22 * NGE_TILEMAP_WIDTH + 2].tile_id == MARK_ID);
    nge_map_draw(&m);     /* draw always draws */
    CHECK(tilemap_matches(&m));

    /* ---- camera clamping ---- */
    CHECK(nge_map_set_camera(&m, 10, 5) == 1 && m.cam_x == 10 && m.cam_y == 5 && m.dirty);
    nge_map_update(&m);
    CHECK(nge_map_set_camera(&m, 10, 5) == 0 && !m.dirty);        /* no move, no redraw */
    CHECK(nge_map_set_camera(&m, 255, 255) == 1);
    CHECK(m.cam_x == 60 - 32 && m.cam_y == 40 - 9);                /* right and bottom edges */
    CHECK(nge_map_move_camera(&m, 1, 1) == 0);                     /* already at the edge */
    CHECK(nge_map_move_camera(&m, -128, -128) == 1 && m.cam_x == 0 && m.cam_y == 0);
    CHECK(nge_map_move_camera(&m, -1, 0) == 0);
    CHECK(nge_map_move_camera(&m, 3, 2) == 1 && m.cam_x == 3 && m.cam_y == 2);
    CHECK(nge_map_centre_camera(&m, 30, 20) == 1 && m.cam_x == 30 - 16 && m.cam_y == 20 - 4);
    CHECK(nge_map_centre_camera(&m, 1, 1) == 1 && m.cam_x == 0 && m.cam_y == 0);
    CHECK(nge_map_centre_camera(&m, 59, 39) == 1 && m.cam_x == 28 && m.cam_y == 31);
    mark_tilemap();
    nge_map_update(&m);
    CHECK(tilemap_matches(&m));

    /* A map exactly the size of the window cannot move */
    nge_map_init(&m, map_mem, 32, 9, &window);
    nge_map_update(&m);
    CHECK(nge_map_move_camera(&m, 5, 5) == 0 && nge_map_centre_camera(&m, 31, 8) == 0);
    CHECK(!m.dirty);

    /* The whole tilemap as a window */
    nge_map_init(&m, map_mem, NGE_MAP_STRIDE, NGE_MAP_MAX_HEIGHT, &full);
    nge_map_set_camera(&m, 255, 255);
    CHECK(m.cam_x == 40 && m.cam_y == 16);
    mark_tilemap();
    nge_map_update(&m);
    CHECK(tilemap_matches(&m));

    /* ---- tile access ---- */
    fill_map();
    nge_map_init(&m, map_mem, 60, 40, &window);
    nge_map_set_camera(&m, 10, 10);
    nge_map_update(&m);
    CHECK(nge_map_get(&m, 7, 3)->tile_id == 7 && nge_map_get(&m, 7, 3)->tile_attr == 3);
    CHECK(&NGE_MAP_TILE(&m, 59, 39) == nge_map_get(&m, 59, 39));
    CHECK(&NGE_MAP_TILE(&m, 0, 1) == &map_mem[NGE_MAP_STRIDE]);

    nge_map_set(&m, 0, 0, &gem);              /* off-screen: changed, no redraw */
    CHECK(map_mem[0].tile_id == 0x99 && !m.dirty);
    nge_map_set(&m, 9, 12, &gem);             /* just left of the view */
    CHECK(!m.dirty);
    nge_map_set(&m, 42, 12, &gem);            /* just right of the view */
    CHECK(!m.dirty);
    nge_map_set(&m, 41, 18, &gem);            /* bottom-right corner of the view */
    CHECK(m.dirty);
    nge_map_update(&m);
    CHECK(test_tilemap[30 * NGE_TILEMAP_WIDTH + 33].tile_id == 0x99);
    nge_map_set(&m, 41, 19, &gem);            /* just below the view */
    CHECK(!m.dirty);
    nge_map_set(&m, 10, 10, &gem);            /* top-left corner of the view */
    CHECK(m.dirty);

    /* ---- screen coordinates: camera (10,10), window at tile (2,22) ---- */
    CHECK(nge_map_to_screen(&m, 80, 80, &sx, &sy) == 1);       /* view's top-left pixel */
    CHECK(sx == 16 && sy == 176);
    CHECK(nge_map_to_screen(&m, 80 + 255, 80 + 71, &sx, &sy) == 1);  /* bottom-right pixel */
    CHECK(sx == 16 + 255 && sy == 176 + 71);
    CHECK(nge_map_to_screen(&m, 80 + 256, 100, &sx, &sy) == 0);      /* right of the view */
    CHECK(nge_map_to_screen(&m, 100, 80 + 72, &sx, &sy) == 0);       /* below the view */
    /* Left of the view: placed just off the left of the window (9-bit wrap) */
    CHECK(nge_map_to_screen(&m, 70, 80, &sx, &sy) == 0);
    CHECK(sx == 6);
    nge_map_init(&m, map_mem, 60, 40, &full);
    nge_map_set_camera(&m, 4, 0);
    CHECK(nge_map_to_screen(&m, 24, 0, &sx, &sy) == 0);
    CHECK((sx & 0x1FF) == 512 - 8);

    /* ---- sprite clip window ---- */
    nge_map_init(&m, map_mem, 60, 40, &window);
    test_reg_log_count = 0;
    nge_map_clip_sprites(&m);
    CHECK(test_reg_log_reg[1] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[1] == 2 * 4);
    CHECK(test_reg_log_val[2] == 33 * 4 + 3);
    CHECK(test_reg_log_val[3] == 22 * 8 && test_reg_log_val[4] == 30 * 8 + 7);

    /* ---- randomised camera and drawing ---- */
    fill_map();
    random_camera();

    /* ---- block table: after the largest map's tiles, flags by tile ID ---- */
    CHECK(NGE_MAP_BUFFER_BYTES == NGE_MAP_MAX_BYTES + 256 && NGE_MAP_BUFFER_BYTES <= 8192);
    fill_map();                                              /* tile ID = column */
    nge_map_init(&m, map_mem, 60, 40, &window);
    CHECK((uint8_t *)m.block == (uint8_t *)map_mem + NGE_MAP_MAX_BYTES);
    nge_map_clear_blocks(&m, NGE_BLOCK_SOLID);
    CHECK(m.block[0] == NGE_BLOCK_SOLID && m.block[255] == NGE_BLOCK_SOLID);
    CHECK(map_mem[NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT - 1].tile_id == NGE_MAP_STRIDE - 1);   /* tiles untouched */
    nge_map_set_block(&m, 7, NGE_BLOCK_NONE);
    nge_map_set_block(&m, 9, NGE_BLOCK_NO_FLOOR | NGE_BLOCK_USER);
    CHECK(nge_map_block(&m, 7, 0) == NGE_BLOCK_NONE && nge_map_block(&m, 7, 39) == NGE_BLOCK_NONE);
    CHECK(nge_map_block(&m, 9, 5) == (NGE_BLOCK_NO_FLOOR | NGE_BLOCK_USER));
    CHECK(nge_map_block(&m, 8, 5) == NGE_BLOCK_SOLID && NGE_MAP_BLOCK(&m, 9, 5) == m.block[9]);
    /* Outside the map: the edge, whatever the table says */
    CHECK(nge_map_block(&m, 60, 0) == NGE_BLOCK_EDGE && nge_map_block(&m, 0, 40) == NGE_BLOCK_EDGE);
    CHECK(nge_map_block(&m, 255, 255) == NGE_BLOCK_EDGE);
    /* The table follows its own buffer, whatever the map's size */
    nge_map_init(&m, map_mem, NGE_MAP_STRIDE, NGE_MAP_MAX_HEIGHT, &window);
    CHECK(nge_map_block(&m, 9, NGE_MAP_MAX_HEIGHT - 1) == (NGE_BLOCK_NO_FLOOR | NGE_BLOCK_USER));
    CHECK(nge_map_block(&m, NGE_MAP_STRIDE - 1, 0) == NGE_BLOCK_SOLID);
    CHECK(nge_map_block(&m, NGE_MAP_STRIDE, 0) == NGE_BLOCK_EDGE);
    nge_map_init(&m, map_mem, 60, 40, &window);

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(nge_map_get(&m, 60, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_set(&m, 0, 40, &gem));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_init(&m, map_mem, 31, 40, &window));       /* narrower than the window */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_init(&m, map_mem, NGE_MAP_STRIDE + 1, 40, &window));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_map_init(&m, map_mem, 60, NGE_MAP_MAX_HEIGHT + 1, &window));
    CHECK(test_halt_count == 1);

    return test_summary("map");
}
