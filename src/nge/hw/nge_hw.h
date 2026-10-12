#ifndef NGE_HW_H
#define NGE_HW_H

/**
 * @file nge_hw.h
 * @brief ZX Next hardware initialization and CPU configuration.
 */

#include <stdint.h>

/**
 * @brief CPU speed settings, matching the values of the turbo mode register.
 */
enum nge_hw_cpu_speed
{
    NGE_HW_CPU_3MHZ = 0,  /**< 3.5 MHz (standard Spectrum speed). */
    NGE_HW_CPU_7MHZ = 1,  /**< 7 MHz. */
    NGE_HW_CPU_14MHZ = 2, /**< 14 MHz. */
    NGE_HW_CPU_28MHZ = 3  /**< 28 MHz. */
};

/**
 * @brief Initialize the hardware defaults required by the game.
 *
 * Sets the CPU to 28 MHz, disables contention, and initializes video and audio.
 */
void nge_hw_init(void);

/**
 * @brief Clear the ULA display to black and disable unused video interrupts.
 */
void nge_video_init(void);

/**
 * @brief Silence all AY sound channels.
 */
void nge_audio_init(void);

/**
 * @brief Disable RAM contention, preserving the other Peripheral 3 settings.
 */
void nge_hw_disable_contention(void);

/**
 * @brief Set the CPU speed.
 * @param speed Requested speed; values above ::NGE_HW_CPU_28MHZ are ignored.
 */
void nge_hw_set_cpu_speed(enum nge_hw_cpu_speed speed);

#endif
