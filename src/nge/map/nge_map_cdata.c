/* nge-dataseg: PAGE_51 */
/*
 * The map's data in page 51, permanently in slot 1 ($2000-$3FFF) once
 * nge_map_init() has mapped it: the block table, the state and the tiles,
 * at fixed addresses. Uninitialised: nge_map_init() sets the state, the
 * game fills in the tiles and the block table. See "Always-mapped data
 * pages" in docs/memory_map.md.
 */
#include "map/nge_map.h"

/* Everything fits in the 8K page (also checked by scripts/check-banks.sh) */
typedef char nge_map_page_check[(NGE_MAP_BLOCK_ENTRIES + sizeof(nge_map_t) +
                                 NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT * sizeof(nge_tile_t)) <= 8192 ? 1 : -1];

nge_block_t nge_map_blocks[NGE_MAP_BLOCK_ENTRIES];
nge_map_t   nge_map;
nge_tile_t  nge_map_tiles[NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT];
