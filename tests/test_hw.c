/* nge_hw.c: hardware set-up registers. */
#include "test.h"

#include "hw/nge_hw.h"

int main(void)
{
    test_init();

    /* Video: layer order, ULA screen off (other bits kept), black fallback */
    test_regs[0x68] = 0x01;
    nge_video_init();
    CHECK(test_regs[0x15] == 0x00);             /* SLU, sprites not yet visible */
    CHECK(test_regs[0x68] == 0x81);
    CHECK(test_regs[0x4A] == 0x00);

    /* CPU speed, ignoring values out of range */
    nge_hw_set_cpu_speed(NGE_HW_CPU_14MHZ);
    CHECK(test_regs[0x07] == 2);
    nge_hw_set_cpu_speed(7);
    CHECK(test_regs[0x07] == 2);

    /* Contention off, other Peripheral 3 bits kept */
    test_regs[0x08] = 0x12;
    nge_hw_disable_contention();
    CHECK(test_regs[0x08] == (0x12 | 0x40));

    /* Interrupt sources: all off, including the expansion bus and ULA (on after reset) */
    test_regs[0xC4] = 0x81;
    test_regs[0xC5] = 0xFF;
    test_regs[0xC6] = 0x77;
    test_reg_log_count = 0;
    nge_hw_init();
    CHECK(test_regs[0xC4] == 0 && test_regs[0xC5] == 0 && test_regs[0xC6] == 0);
    /* ...first, before anything else is set up */
    CHECK(test_reg_log_reg[0] == 0xC4 && test_reg_log_reg[1] == 0xC5 && test_reg_log_reg[2] == 0xC6);

    return test_summary("hw");
}
