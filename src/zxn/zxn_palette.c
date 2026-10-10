#include "zxn_palette.h"

#include <arch/zxn.h>
#include <stddef.h>

#include "zxn_util.h"

#define ZXN_REG_PALETTE_VALUE_9  0x44 /* Use to set 9-bit (2-byte) colours of the Enhanced ULA palette, or to read second byte of colour. */

#define ZXN_TILE_PALETTE_SIZE_8_BIT 256 /* 256 bytes for 8-bit palette */
#define ZXN_TILE_PALETTE_SIZE_9_BIT 512 /* 512 bytes for 9-bit palette */

#include <arch/zxn.h>

/* Palette control register bits 6-4 select the palette that writes go to;
   bit 7 = 1 would disable index auto-increment */
#define ZXN_RPC_SELECT_MASK  0xF0

void zxn_write_palette9(uint8_t palette_select, uint8_t first_index,
                        const zxn_palette_entry_9_t *palette_entries, uint16_t num_entries)
{
    zxn_util_assert(palette_entries != NULL);
    zxn_util_assert(first_index + num_entries <= 256);

    /* Replace the previous selection and keep auto-increment on; the low
       bits (which palettes are displayed) are preserved */
    uint8_t reg = (ZXN_READ_REG(REG_PALETTE_CONTROL) & (uint8_t)~ZXN_RPC_SELECT_MASK) | palette_select;

    ZXN_WRITE_REG(REG_PALETTE_CONTROL, reg);
    ZXN_WRITE_REG(REG_PALETTE_INDEX, first_index);

    /* Each entry is two writes; the index advances after the second */
    while (num_entries--) {
        ZXN_WRITE_REG(ZXN_REG_PALETTE_VALUE_9, palette_entries->value);
        ZXN_WRITE_REG(ZXN_REG_PALETTE_VALUE_9, palette_entries->extra);
        palette_entries++;
    }
}
