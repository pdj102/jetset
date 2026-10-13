#include "hw/nge_hw.h"

#include <arch/zxn.h>

/*
 * Interrupt enable registers (core 3.1.5+; not named by z88dk). Clearing a
 * bit puts that device in polled mode: it no longer raises interrupts.
 */
#define NGE_REG_INT_EN_0  0xC4  /* bit 7 expansion bus /INT, bit 1 line, bit 0 ULA (reset $81) */
#define NGE_REG_INT_EN_1  0xC5  /* bits 7-0: CTC channels 7-0 (reset $00) */
#define NGE_REG_INT_EN_2  0xC6  /* bits 6,5,4: UART1 Tx empty, Rx near full, Rx available;
                                   bits 2,1,0: the same for UART0 (reset $00) */

/* Disable interrupts and every maskable interrupt source. */
void nge_hw_disable_interrupts(void)
{
#ifdef __SDCC
    __asm
        di
    __endasm;
#endif
    ZXN_WRITE_REG(NGE_REG_INT_EN_0, 0);
    ZXN_WRITE_REG(NGE_REG_INT_EN_1, 0);
    ZXN_WRITE_REG(NGE_REG_INT_EN_2, 0);
}

/* Initialize the hardware defaults required by the game. */
void nge_hw_init(void)
{
    nge_hw_disable_interrupts();
    nge_hw_set_cpu_speed(NGE_HW_CPU_28MHZ);
    nge_hw_disable_contention();
    nge_video_init();
    nge_audio_init();
}

#define NGE_REG_ULA_CONTROL     0x68
#define NGE_RUC_DISABLE_ULA     0x80 /* bit 7 = 1 to stop displaying the ULA screen */

/* Set the layer order, hide the ULA screen and disable unused video interrupts. */
void nge_video_init(void)
{
    /*
     * Sprites over Layer 2 over ULA/tilemap: sprites draw over the room tiles.
     * Sprites stay hidden until the sprite code enables RSLS_SPRITES_VISIBLE.
     */
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM, RSLS_LAYER_PRIORITY_SLU);

    /*
     * The ULA screen memory at 0x4000 holds the tilemap and tile definitions,
     * so the ULA would show them as garbage through transparent tilemap pixels.
     */
    ZXN_WRITE_REG(NGE_REG_ULA_CONTROL, ZXN_READ_REG(NGE_REG_ULA_CONTROL) | NGE_RUC_DISABLE_ULA);

    /* Colour shown where every layer is transparent (RRRGGGBB): black */
    ZXN_WRITE_REG(REG_FALLBACK_COLOR, 0x00);

    ZXN_WRITE_REG(REG_LINE_INTERRUPT_CONTROL, RLIC_DISABLE_ULA_INTERRUPT);
}

/* Silence the AY channels until the game starts playing audio. */
void nge_audio_init(void)
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
void nge_hw_disable_contention(void)
{
    ZXN_WRITE_REG(REG_PERIPHERAL_3,
                  (ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION));
}

/* Set the CPU speed, ignoring values outside the supported range. */
void nge_hw_set_cpu_speed(enum nge_hw_cpu_speed speed)
{
    if (speed > NGE_HW_CPU_28MHZ)
    {
        return;
    }

    ZXN_WRITE_REG(REG_TURBO_MODE, (uint8_t)speed);
}