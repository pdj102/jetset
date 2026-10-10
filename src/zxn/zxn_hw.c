#include "zxn_hw.h"

#include <arch/zxn.h>

/* Initialize the hardware defaults required by the game. */
void zxn_hw_init(void)
{
    zxn_hw_set_cpu_speed(ZXN_HW_CPU_28MHZ);
    zxn_hw_disable_contention();
    zxn_video_init();
    zxn_audio_init();
}

/* Enable sprite layer and disable unused video interrupt sources. */
void zxn_video_init(void)
{
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM, 0);
    ZXN_WRITE_REG(REG_LINE_INTERRUPT_CONTROL, RLIC_DISABLE_ULA_INTERRUPT);
}

/* Silence the AY channels until the game starts playing audio. */
void zxn_audio_init(void)
{
    IO_AY_REG = 7;
    IO_AY_DAT = 0x3f;
    IO_AY_REG = 8;
    IO_AY_DAT = 0;
    IO_AY_REG = 9;
    IO_AY_DAT = 0;
    IO_AY_REG = 10;
    IO_AY_DAT = 0;
}

/* Disable Spectrum Next RAM contention while preserving other settings. */
void zxn_hw_disable_contention(void)
{
    ZXN_WRITE_REG(REG_PERIPHERAL_3,
                  (ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION));
}

/* Set the CPU speed, ignoring values outside the supported range. */
void zxn_hw_set_cpu_speed(enum zxn_hw_cpu_speed speed)
{
    if (speed > ZXN_HW_CPU_28MHZ)
    {
        return;
    }

    ZXN_WRITE_REG(REG_TURBO_MODE, (uint8_t)speed);
}