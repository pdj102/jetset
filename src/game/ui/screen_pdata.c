/* nge-dataseg: PAGE_117 */
/*
 * The ui module's page data, in page 117 (slot 7 while its code in page
 * 116 runs): the inventory screen's list. Uninitialised: the screen sets
 * it up each time it opens. See "Page data" in docs/memory_map.md.
 */
#include "game/ui/screen_internal.h"

nge_entity_id_t screen_inv_ids[SCREEN_INV_MAX];
nge_list_t screen_inv_list;
NGE_FILE *screen_inv_streams[SCREEN_INV_PANELS];
