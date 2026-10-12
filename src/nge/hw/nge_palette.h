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

/**
 * @brief Write 9-bit palette entries to a selected ZX Next palette.
 * @param palette_select Palette-selection bits to apply to the palette control register e.g. RPC_SELECT_LAYER_2_PALETTE_0
 * @param first_index Index of the first entry to write (0-255).
 * @param palette_entries Pointer to the palette entries to write.
 * @param num_entries Number of palette entries to write.
 */
void nge_write_palette9(uint8_t palette_select, uint8_t first_index,
                        const nge_palette_entry_9_t *palette_entries, uint16_t num_entries);

#endif
