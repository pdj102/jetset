#include "zxn_palette.h"

#include <arch/zxn.h>

#include "zxn_util.h"

#define ZXN_REG_PALETTE_VALUE_9  0x44 /* Use to set 9-bit (2-byte) colours of the Enhanced ULA palette, or to read second byte of colour. */

#define ZXN_REG_TILEMAP_TRANSPARENCY_INDEX 0x4C /* Tilemap transparency index register */

#define ZXN_TILE_PALETTE_SIZE_8_BIT 256 /* 256 bytes for 8-bit palette */
#define ZXN_TILE_PALETTE_SIZE_9_BIT 512 /* 512 bytes for 9-bit palette */

#include <arch/zxn.h>

void zxn_write_palette9(uint8_t palette_select, const zxn_palette_entry_9_t *palette_entries, uint16_t num_entries)
{
    zxn_util_assert(palette_entries != NULL);

    // Read the current value of the Palette Control Register and set the palette selection bits
    uint8_t reg = ZXN_READ_REG(REG_PALETTE_CONTROL) | palette_select;

    // Select the palette for writing
    ZXN_WRITE_REG(REG_PALETTE_CONTROL, reg);

    // Set the palette index to 0
    ZXN_WRITE_REG(REG_PALETTE_INDEX, 0); 

    // Write the palette entries
    while (num_entries--) {
        ZXN_WRITE_REG(ZXN_REG_PALETTE_VALUE_9, palette_entries->value);
        palette_entries++;
        ZXN_WRITE_REG(ZXN_REG_PALETTE_VALUE_9, palette_entries->extra);
        palette_entries++;
    }
}