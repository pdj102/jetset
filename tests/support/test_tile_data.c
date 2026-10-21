/*
 * The map's palette (data/tile_data.h) for the PC tests: the real one is
 * assembly in page 133 (src/data/tile_palette_data.asm), which the PC can't
 * build. Only its address is used: nge_palette_load9() copies it to the
 * fake registers.
 */
#include "test.h"

#include "data/tile_data.h"

const nge_palette_entry_9_t tile_palette[TILE_PALETTE_ENTRIES];
