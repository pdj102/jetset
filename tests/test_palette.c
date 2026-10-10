/* zxn_palette.c: palette register writes. */
#include "test.h"

#include "zxn_palette.h"

int main(void)
{
    static const zxn_palette_entry_9_t three[] = { {0x11, 1}, {0x22, 0}, {0x33, 1} };
    int i;
    int all_0x44 = 1;

    test_init();

    /* Layer 2 palette 1 was selected for writing; display bits set */
    test_regs[0x43] = 0x5F;
    zxn_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 5, three, 3);

    /* Selection replaced, display bits kept, auto-increment left on */
    CHECK(test_reg_log_reg[0] == 0x43 && test_reg_log_val[0] == 0x3F);
    CHECK(test_reg_log_reg[1] == 0x40 && test_reg_log_val[1] == 5);

    /* Each entry once: its own value byte, then its own extra byte */
    CHECK(test_reg_log_count == 2 + 3 * 2);
    for (i = 2; i < test_reg_log_count; i++) {
        all_0x44 = all_0x44 && test_reg_log_reg[i] == 0x44;
    }
    CHECK(all_0x44);
    CHECK(test_reg_log_val[2] == 0x11 && test_reg_log_val[3] == 1);
    CHECK(test_reg_log_val[4] == 0x22 && test_reg_log_val[5] == 0);
    CHECK(test_reg_log_val[6] == 0x33 && test_reg_log_val[7] == 1);

    /* Writing past entry 255 is caught */
    TEST_EXPECT_HALT(zxn_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0, 254, three, 3));
    CHECK(test_halt_count == 1);

    return test_summary("palette");
}
