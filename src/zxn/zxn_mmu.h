#ifndef ZXN_MMU_H
#define ZXN_MMU_H

/**
 * @file zxn_mmu.h
 * @brief Mapping 8K RAM pages into the Z80's address space.
 *
 * The 64K address space is eight 8K slots; each shows one 8K page of RAM (or
 * the ROM). See docs/memory_map.md for which slots the game may change.
 */

#include <stdint.h>

#define ZXN_MMU_SLOT_BANKED_0  6     /**< Banked slot at $C000 (even PAGE_n sections). */
#define ZXN_MMU_SLOT_BANKED_1  7     /**< Banked slot at $E000 (odd PAGE_n sections). */
#define ZXN_MMU_SLOT_ADDR(slot)  ((uint16_t)(slot) << 13)  /**< First address of a slot. */
#define ZXN_MMU_PAGE_ROM       0xFF  /**< Page number that maps the ROM into slot 0 or 1. */

/**
 * @brief Map a page into a slot.
 *
 * Slots 2-5 hold video memory, the program and the stack and must not be
 * changed; doing so is an assert. Map the returned page back afterwards so
 * calls can nest:
 *
 *     uint8_t old = zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, 32);
 *     ...use $C000-$DFFF...
 *     zxn_mmu_map(ZXN_MMU_SLOT_BANKED_0, old);
 *
 * @param slot Slot 0, 1, 6 or 7.
 * @param page 8K page number, or ZXN_MMU_PAGE_ROM for slot 0 or 1.
 * @return The page that was mapped in the slot before.
 */
uint8_t zxn_mmu_map(uint8_t slot, uint8_t page);

/**
 * @brief Get the page currently mapped into a slot.
 * @param slot Slot 0-7.
 * @return The page number.
 */
uint8_t zxn_mmu_get(uint8_t slot);

#endif
