#ifndef ZXN_HW_H
#define ZXN_HW_H

/**
 * @file zxn_hw.h
 * @brief ZX Next hardware initialization and CPU configuration.
 */

#include <stdint.h>

/**
 * @brief CPU speed settings, matching the values of the turbo mode register.
 */
enum zxn_hw_cpu_speed
{
    ZXN_HW_CPU_3MHZ = 0,  /**< 3.5 MHz (standard Spectrum speed). */
    ZXN_HW_CPU_7MHZ = 1,  /**< 7 MHz. */
    ZXN_HW_CPU_14MHZ = 2, /**< 14 MHz. */
    ZXN_HW_CPU_28MHZ = 3  /**< 28 MHz. */
};

/**
 * @brief Initialize the hardware defaults required by the game.
 *
 * Sets the CPU to 28 MHz, disables contention, and initializes video and audio.
 */
void zxn_hw_init(void);

/**
 * @brief Clear the ULA display to black and disable unused video interrupts.
 */
void zxn_video_init(void);

/**
 * @brief Silence all AY sound channels.
 */
void zxn_audio_init(void);

/**
 * @brief Disable RAM contention, preserving the other Peripheral 3 settings.
 */
void zxn_hw_disable_contention(void);

/**
 * @brief Set the CPU speed.
 * @param speed Requested speed; values above ::ZXN_HW_CPU_28MHZ are ignored.
 */
void zxn_hw_set_cpu_speed(enum zxn_hw_cpu_speed speed);

#endif
