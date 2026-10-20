/*
 * The fake Layer 2 (layer2/nge_layer2.h) for the PC tests: the real drawing
 * writes to slot 7 by address, so it is left out of the test build
 * (LIB_EXCLUDE in tests/Makefile). This records what would be drawn, a
 * character and its colours per cell.
 */
#include "test.h"

#include "layer2/nge_layer2.h"

#define FONT_FIRST  32
#define FONT_END    128

/* A drawing call's cells are in range: counted only when they aren't, so a test's check count is its own */
#define IN_RANGE(cond)  do { if (!(cond)) { CHECK(cond); } } while (0)

test_l2_cell_t test_layer2[NGE_LAYER2_ROWS][NGE_LAYER2_COLS];
uint8_t test_layer2_shown;

void nge_layer2_init(void)
{
    nge_layer2_fill(0, 0, NGE_LAYER2_COLS - 1, NGE_LAYER2_ROWS - 1, NGE_LAYER2_TRANSPARENT);
    nge_layer2_show(1);
}

void nge_layer2_show(uint8_t on)
{
    test_layer2_shown = on;
}

void nge_layer2_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour)
{
    uint8_t x;
    uint8_t y;

    IN_RANGE(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);
    for (y = y1; y <= y2 && y < NGE_LAYER2_ROWS; y++) {
        for (x = x1; x <= x2 && x < NGE_LAYER2_COLS; x++) {
            test_layer2[y][x].c = ' ';
            test_layer2[y][x].ink = colour;
            test_layer2[y][x].paper = colour;
        }
    }
}

void nge_layer2_glyph(uint8_t col, uint8_t row, uint8_t c, uint8_t ink, uint8_t paper)
{
    IN_RANGE(col < NGE_LAYER2_COLS && row < NGE_LAYER2_ROWS);
    if (col < NGE_LAYER2_COLS && row < NGE_LAYER2_ROWS) {
        test_layer2[row][col].c = (c >= FONT_FIRST && c < FONT_END) ? c : ' ';   /* as the real font */
        test_layer2[row][col].ink = ink;
        test_layer2[row][col].paper = paper;
    }
}

void nge_layer2_text(uint8_t col, uint8_t row, const char *s, uint8_t ink, uint8_t paper)
{
    for (; *s != '\0' && col < NGE_LAYER2_COLS; s++, col++) {
        nge_layer2_glyph(col, row, (uint8_t)*s, ink, paper);
    }
}

void nge_layer2_scroll_up(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill)
{
    uint8_t x;
    uint8_t y;

    IN_RANGE(x1 <= x2 && x2 < NGE_LAYER2_COLS && y1 <= y2 && y2 < NGE_LAYER2_ROWS);
    for (y = y1; y < y2; y++) {
        for (x = x1; x <= x2; x++) {
            test_layer2[y][x] = test_layer2[y + 1][x];
        }
    }
    for (x = x1; x <= x2; x++) {
        test_layer2[y2][x].c = ' ';
        test_layer2[y2][x].ink = fill;
        test_layer2[y2][x].paper = fill;
    }
}

void test_layer2_row(uint8_t y, uint8_t x1, uint8_t x2, char *out)
{
    uint8_t x;

    for (x = x1; x <= x2; x++) {
        *out++ = (char)test_layer2[y][x].c;
    }
    *out = 0;
}
