/* zxn_hw.c: hardware set-up registers. */
#include "test.h"

#include "zxn_hw.h"

int main(void)
{
    test_init();

    /* Video: layer order, ULA screen off (other bits kept), black fallback */
    test_regs[0x68] = 0x01;
    zxn_video_init();
    CHECK(test_regs[0x15] == 0x00);             /* SLU, sprites not yet visible */
    CHECK(test_regs[0x68] == 0x81);
    CHECK(test_regs[0x4A] == 0x00);

    /* CPU speed, ignoring values out of range */
    zxn_hw_set_cpu_speed(ZXN_HW_CPU_14MHZ);
    CHECK(test_regs[0x07] == 2);
    zxn_hw_set_cpu_speed((enum zxn_hw_cpu_speed)7);
    CHECK(test_regs[0x07] == 2);

    /* Contention off, other Peripheral 3 bits kept */
    test_regs[0x08] = 0x12;
    zxn_hw_disable_contention();
    CHECK(test_regs[0x08] == (0x12 | 0x40));

    return test_summary("hw");
}
