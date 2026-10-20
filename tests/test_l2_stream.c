/*
 * nge_l2_stream.c: Layer 2 cell streams, on the fake Layer 2 (support/test_layer2.c).
 * Their text must behave exactly as a tilemap stream's, so random sequences
 * of characters and control codes are sent to both and the characters
 * compared; the colours are checked directly.
 */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "io/nge_l2_stream.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"
#include "layer2/nge_layer2.h"

#define BLACK   0x00
#define BLUE    0x02
#define WHITE   0xFF
#define YELLOW  0xFC

static char row[NGE_LAYER2_COLS + 1];

/* ---- Colours: the ink table, raw colours, the paper ---- */

static void test_colours(void)
{
    static const nge_rect_t box = { 2, 9, 3, 5 };   /* 8 wide, 3 rows */
    NGE_FILE *s;

    nge_layer2_init();
    s = nge_open_layer2(&box, BLUE);
    CHECK(s != NULL);
    CHECK(test_layer2[3][2].paper == NGE_LAYER2_TRANSPARENT);   /* opening doesn't clear */

    /* Clearing fills the cells with the paper and homes the cursor */
    nge_putc('x', s);
    nge_text_cls(s);
    CHECK(test_layer2[3][2].paper == BLUE && test_layer2[5][9].paper == BLUE && test_layer2[3][2].c == ' ');
    CHECK(test_layer2[2][2].paper == NGE_LAYER2_TRANSPARENT && test_layer2[3][10].paper == NGE_LAYER2_TRANSPARENT);

    /* White to start with, on the paper */
    nge_putc('A', s);
    CHECK(test_layer2[3][2].c == 'A' && test_layer2[3][2].ink == WHITE && test_layer2[3][2].paper == BLUE);

    /* A text colour through the table; anything else white; ATTR a raw colour */
    nge_text_ink(NGE_INK_YELLOW, s);
    nge_putc('B', s);
    CHECK(test_layer2[3][3].ink == YELLOW);
    nge_text_ink(NGE_INK_RED, s);
    nge_putc('C', s);
    CHECK(test_layer2[3][4].ink == 0xE0);
    nge_text_ink(3, s);
    nge_putc('D', s);
    CHECK(test_layer2[3][5].ink == WHITE);
    nge_text_attr(0x1C, s);
    nge_putc('E', s);
    CHECK(test_layer2[3][6].ink == 0x1C);

    /* Clear to the end of the line: the paper, the cursor kept */
    nge_text_at(0, 2, s);
    nge_text_clreol(s);
    nge_putc('Z', s);
    test_layer2_row(3, 2, 9, row);
    CHECK_STR(row, "ABZ     ");
    CHECK(test_layer2[3][9].paper == BLUE);

    /* Scrolling at the bottom: the rows move up, the new one is paper */
    nge_text_at(2, 0, s);
    nge_puts("bottom", s);
    nge_putc('\n', s);
    test_layer2_row(4, 2, 9, row);
    CHECK_STR(row, "bottom  ");
    test_layer2_row(5, 2, 9, row);
    CHECK_STR(row, "        ");
    CHECK(test_layer2[5][2].paper == BLUE);

    /* Characters outside the font show as a space */
    nge_putc(0xA0, s);
    CHECK(test_layer2[5][2].c == ' ');

    nge_close(s);
}

static void test_open_asserts(void)
{
    static const nge_rect_t too_wide = { 0, NGE_LAYER2_COLS, 0, 0 };
    static const nge_rect_t too_tall = { 0, 0, 0, NGE_LAYER2_ROWS };

    TEST_EXPECT_HALT(nge_open_layer2(&too_wide, BLACK));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_open_layer2(&too_tall, BLACK));
    CHECK(test_halt_count == 1);
}

/* ---- Randomised comparison with a tilemap stream ---- */

/*
 * The same character on Layer 2 as on the tilemap, in every cell. A tile
 * number past the font (128 up, which a byte left over from a control
 * code's parameter can write) shows on Layer 2 as a space
 */
static int same_text(void)
{
    int x;
    int y;
    uint8_t t;

    for (y = 0; y < NGE_LAYER2_ROWS; y++) {
        for (x = 0; x < NGE_LAYER2_COLS; x++) {
            t = test_tilemap[y * NGE_TILEMAP_WIDTH + x].tile_id;
            if (test_layer2[y][x].c != ((t >= 0x80) ? ' ' : t)) {
                return 0;
            }
        }
    }
    return 1;
}

static void both(NGE_FILE *l2, NGE_FILE *tm, int c)
{
    nge_putc((uint8_t)c, l2);
    nge_putc((uint8_t)c, tm);
}

static void test_random(void)
{
    /* Three separate windows (one a single column), or one full-screen window */
    static const nge_rect_t areas[4] = { {0, 19, 0, 9}, {20, 39, 10, 20}, {5, 5, 25, 31}, {0, 39, 0, 31} };
    static const nge_tile_t blank = { 0x20, 0 };
    int seed;
    int i;
    int w;
    int mismatches = 0;

    for (seed = 1; seed <= 300; seed++) {
        int nw = (seed % 4 == 0) ? 1 : 3;
        NGE_FILE *l2[3];
        NGE_FILE *tm[3];

        srand(seed);
        nge_layer2_init();            /* every cell a space */
        nge_tilemap_clear(&blank);    /* and every tile */
        for (w = 0; w < nw; w++) {
            const nge_rect_t *a = (nw == 1) ? &areas[3] : &areas[w];

            l2[w] = nge_open_layer2(a, BLACK);
            tm[w] = nge_open_tilemap(a, 0);
        }
        for (i = 0; i < 3000; i++) {
            int k = rand() % 100;

            w = rand() % nw;
            if (k < 68)      both(l2[w], tm[w], 0x20 + rand() % 96);
            else if (k < 75) both(l2[w], tm[w], 0x0A);
            else if (k < 78) both(l2[w], tm[w], 0x0D);
            else if (k < 82) both(l2[w], tm[w], 0x08);
            else if (k < 83) both(l2[w], tm[w], 0x0C);
            else if (k < 85) both(l2[w], tm[w], 0x05);
            else if (k < 87) { both(l2[w], tm[w], 0x10); both(l2[w], tm[w], rand() % 16); }
            else if (k < 89) { both(l2[w], tm[w], 0x11); both(l2[w], tm[w], rand() % 256); }
            else if (k < 91) { both(l2[w], tm[w], 0x12); both(l2[w], tm[w], rand() % 3); }
            else if (k < 96) { both(l2[w], tm[w], 0x16); both(l2[w], tm[w], rand() % 40); both(l2[w], tm[w], rand() % 50); }
            else             both(l2[w], tm[w], rand() % 32);
        }
        if (!same_text()) {
            mismatches++;
        }
        for (w = 0; w < nw; w++) {
            nge_close(l2[w]);
            nge_close(tm[w]);
        }
    }
    CHECK(mismatches == 0);
}

int main(void)
{
    test_init();
    test_colours();
    test_open_asserts();
    test_random();
    return test_summary("l2_stream");
}
