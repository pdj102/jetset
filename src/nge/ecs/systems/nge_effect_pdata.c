/* nge-dataseg: PAGE_59 */
/*
 * Effect system page data, in page 59 (slot 7 while the system's code in
 * page 58 runs): the effect component and the game's hook. Uninitialised:
 * nge_effect_init() and nge_effect_start() set it. See "Page data" in
 * docs/memory_map.md.
 */
#include "ecs/systems/nge_effect_internal.h"

uint8_t nge_effect_kinds[NGE_ENTITY_MAX];
int8_t  nge_effect_powers[NGE_ENTITY_MAX];

nge_effect_hook_fn nge_effect_game_hook;
