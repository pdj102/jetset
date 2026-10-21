#ifndef NGE_PALETTE_H
#define NGE_PALETTE_H

/**
 * @file nge_palette.h
 * @brief ZX Next palette-entry formats and palette helpers.
 */

#include <stdint.h>

/* select tilemap first palette*/
#define RPC_SELECT_TILEMAP_PALETTE_0  0x30
/* select tilemap second palette*/
#define RPC_SELECT_TILEMAP_PALETTE_1  0x70

/**
 * @brief Raw palette entry in the 8-bit format used by NextReg $41.
 *
 * The byte is stored as RRRGGGBB.
 */
typedef struct nge_palette_entry_8
{
    /** Packed red, green, and blue colour components in RRRGGGBB order. */
    uint8_t value;
} nge_palette_entry_8_t;


/**
 * @brief Raw palette entry in the 9-bit format used by NextReg $44.
 *
 * The first byte is RRRGGGBB. The second byte contains extra bits: bit 0 is
 * the third blue bit, and bit 7 is the priority-colour bit (Layer 2 only).
 */
typedef struct nge_palette_entry_9
{
    /** Packed red, green, and blue colour components in RRRGGGBB order. */
    uint8_t value;

    /** Extra colour bits, including the third blue bit and priority bit. */
    uint8_t extra;
} nge_palette_entry_9_t;

/** Page argument of nge_palette_load9(): the entries are in common memory ($6A00-$BFFF). */
#define NGE_PALETTE_DATA_COMMON  0

/**
 * @brief Write 9-bit palette entries to a selected ZX Next palette.
 * @param palette_select Palette-selection bits to apply to the palette control register e.g. RPC_SELECT_LAYER_2_PALETTE_0
 * @param first_index Index of the first entry to write (0-255).
 * @param palette_entries Pointer to the palette entries to write.
 * @param num_entries Number of palette entries to write.
 */
void nge_write_palette9(uint8_t palette_select, uint8_t first_index,
                        const nge_palette_entry_9_t *palette_entries, uint16_t num_entries);

/**
 * @brief Load 9-bit palette entries from a page into a selected palette:
 *        nge_write_palette9(), with the entries' page mapped in for the copy.
 *
 * Entries in a page (page = an odd page number) are assembled for slot
 * NGE_MMU_DATA_SLOT ($E000-$FFFF), for example in a SECTION PAGE_n with n
 * odd: the page is mapped into that slot, the entries copied, and the
 * previous page mapped back, so a banked caller's own data page comes back
 * too. Entries in common memory (page = NGE_PALETTE_DATA_COMMON): nothing is
 * mapped. In common memory, so it can be called from banked code and often
 * (fades, palette cycling).
 *
 * @param page Odd page holding the entries, or NGE_PALETTE_DATA_COMMON.
 * @param palette_select Palette-selection bits, as nge_write_palette9()'s.
 * @param first_index Index of the first entry to write (0-255).
 * @param palette_entries Address of the first entry ($E000-$FFFF for entries in a page).
 * @param num_entries Number of palette entries to write.
 */
void nge_palette_load9(uint8_t page, uint8_t palette_select, uint8_t first_index,
                       const nge_palette_entry_9_t *palette_entries, uint16_t num_entries);

#endif
