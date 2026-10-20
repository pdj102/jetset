/*
 * The tilemap's text colour banks: common memory. Apart from the tilemap
 * stream (nge_tm_stream.c), so loading the colours for the map's tiles
 * doesn't link the stream.
 */
#include "io/nge_tm_stream.h"

#include "hw/nge_palette.h"

#define INK_BANK_SIZE     16 /* palette entries per attribute palette offset */
#define INK_BANK_ENTRIES  4  /* entries 0-3 are all the font pixels use */

/* One row per NGE_INK_* bank: entry 0 is transparent, 1-3 are the ink colour.
   Colours are 9-bit: RRRGGGBB plus the low blue bit. */
static const nge_palette_entry_9_t ink_banks[][INK_BANK_ENTRIES] = {
    { {0x00, 0}, {0xFF, 1}, {0xFF, 1}, {0xFF, 1} }, /* NGE_INK_WHITE */
    { {0x00, 0}, {0xFC, 0}, {0xFC, 0}, {0xFC, 0} }, /* NGE_INK_YELLOW */
    { {0x00, 0}, {0x1F, 1}, {0x1F, 1}, {0x1F, 1} }, /* NGE_INK_CYAN */
    { {0x00, 0}, {0xE0, 0}, {0xE0, 0}, {0xE0, 0} }, /* NGE_INK_RED */
};

void nge_tm_load_ink_palette(void)
{
    uint8_t bank;

    for (bank = 0; bank < sizeof(ink_banks) / sizeof(ink_banks[0]); bank++) {
        nge_write_palette9(RPC_SELECT_TILEMAP_PALETTE_0,
                           (uint8_t)((NGE_INK_WHITE + bank) * INK_BANK_SIZE),
                           ink_banks[bank], INK_BANK_ENTRIES);
    }
}
