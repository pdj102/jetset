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
    #define NGE_HW_CPU_3MHZ  0  /**< 3.5 MHz (standard Spectrum speed). */
    #define NGE_HW_CPU_7MHZ  1  /**< 7 MHz. */
    #define NGE_HW_CPU_14MHZ 2  /**< 14 MHz. */
    #define NGE_HW_CPU_28MHZ 3  /**< 28 MHz. */
    #define NGE_HW_CPU_COUNT 4   /**< Number of CPU speed settings. */

/**
 * @brief Initialize the hardware defaults required by the game.
 *
 * Disables interrupts and every interrupt source, sets the CPU to 28 MHz,
 * disables contention, and initializes video and audio.
 */
void nge_hw_init(void);

/**
 * @brief Disable interrupts (di) and every maskable interrupt source.
 *
 * Clears the interrupt enable registers $C4-$C6: expansion bus /INT, line
 * and ULA interrupts, CTC channels and UARTs. Even if code executes ei, no
 * maskable interrupt can arrive, so slot 0 may hold RAM instead of the ROM's
 * IM1 handler at $0038. NMI cannot be masked: with game data at $0066, an
 * NMI (the NMI button) crashes the game. An interrupt module will enable
 * the sources it handles, with its own IM2 handler.
 */
void nge_hw_disable_interrupts(void);

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
void nge_hw_set_cpu_speed(uint8_t speed);

#endif
