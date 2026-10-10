/* zxn_tilemap.c: registers and tilemap memory operations. */
#include "test.h"

#define AT(x, y)  test_tilemap[(y) * ZXN_TILEMAP_WIDTH + (x)]

static void test_registers(void)
{
    static const zxn_rect_t clip = { 1, 2, 3, 4 };

    test_reg_reset();
    zxn_tilemap_init();
    CHECK(test_regs[0x6B] == 0x83);     /* enable, 512 tiles, on top of ULA */
    CHECK(test_regs[0x6E] == 0);        /* tilemap at 0x4000 */
    CHECK(test_regs[0x6F] == 10);       /* tile definitions at 0x4A00 */
    CHECK(test_regs[0x4C] == ZXN_TILEMAP_TRANSPARENT);

    zxn_tilemap_set_scroll(300, 7);
    CHECK(test_regs[0x2F] == 1);        /* x MSB */
    CHECK(test_regs[0x30] == 44);       /* x LSB */
    CHECK(test_regs[0x31] == 7);

    /* Clip window: reset the index, then X1, X2, Y1, Y2 in order */
    test_reg_reset();
    zxn_tilemap_set_clip(&clip);
    CHECK(test_reg_log_count == 5);
    CHECK(test_reg_log_reg[0] == 0x1C && test_reg_log_val[0] == 0x08);
    CHECK(test_reg_log_reg[1] == 0x1B && test_reg_log_val[1] == 1);
    CHECK(test_reg_log_reg[2] == 0x1B && test_reg_log_val[2] == 2);
    CHECK(test_reg_log_reg[3] == 0x1B && test_reg_log_val[3] == 3);
    CHECK(test_reg_log_reg[4] == 0x1B && test_reg_log_val[4] == 4);
}

static void test_memory_ops(void)
{
    static const zxn_tile_t dot = { '.', 0 };
    static const zxn_tile_t a = { 'A', 0x30 };
    static const zxn_rect_t box = { 2, 4, 1, 3 };
    char row[ZXN_TILEMAP_WIDTH + 1];
    int y;

    zxn_tilemap_clear(&dot);
    CHECK(AT(0, 0).tile_id == '.' && AT(39, 31).tile_id == '.');

    zxn_tilemap_set(5, 6, &a);
    CHECK(AT(5, 6).tile_id == 'A' && AT(5, 6).tile_attr == 0x30);
    zxn_tilemap_set_attr(5, 6, 0x50);
    CHECK(AT(5, 6).tile_id == 'A' && AT(5, 6).tile_attr == 0x50);

    /* Cell addresses: +1 is the next column, +width the next row */
    CHECK(zxn_tilemap_cell(0, 0) == &AT(0, 0));
    CHECK(zxn_tilemap_cell(39, 31) == &AT(39, 31));
    CHECK(zxn_tilemap_cell(5, 6) + 1 == &AT(6, 6));
    CHECK(zxn_tilemap_cell(5, 6) + ZXN_TILEMAP_WIDTH == &AT(5, 7));

    /* Fill touches only the area */
    zxn_tilemap_clear(&dot);
    zxn_tilemap_fill(&box, &a);
    test_tilemap_row(0, 0, 6, row);
    CHECK_STR(row, ".......");
    for (y = 1; y <= 3; y++) {
        test_tilemap_row(y, 0, 6, row);
        CHECK_STR(row, "..AAA..");
    }
    test_tilemap_row(4, 0, 6, row);
    CHECK_STR(row, ".......");

    /* Scroll up moves rows inside the area only and fills the bottom row */
    zxn_tilemap_clear(&dot);
    for (y = 1; y <= 3; y++) {
        zxn_tile_t t = { (uint8_t)('0' + y), 0 };
        zxn_tilemap_fill(&(zxn_rect_t){ 0, 6, (uint8_t)y, (uint8_t)y }, &t);
    }
    zxn_tilemap_scroll_up(&box, &(zxn_tile_t){ '_', 0 });
    test_tilemap_row(1, 0, 6, row);
    CHECK_STR(row, "1122211");
    test_tilemap_row(2, 0, 6, row);
    CHECK_STR(row, "2233322");
    test_tilemap_row(3, 0, 6, row);
    CHECK_STR(row, "33___33");
}

static void test_bounds_asserts(void)
{
    static const zxn_tile_t a = { 'A', 0 };

    TEST_EXPECT_HALT(zxn_tilemap_set(ZXN_TILEMAP_WIDTH, 0, &a));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_tilemap_cell(0, ZXN_TILEMAP_HEIGHT));
    CHECK(test_halt_count == 1);
}

int main(void)
{
    test_init();
    test_registers();
    test_memory_ops();
    test_bounds_asserts();
    return test_summary("tilemap");
}
