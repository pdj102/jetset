/* nge-dataseg: PAGE_111 */
/*
 * The attack component's tables, page data in page 111 (slot 7 while the
 * component functions in page 110 run), moved out of slot 0 as they are
 * read at most once a turn (docs/slot0_tables.md, D4). Other pages read an
 * attack with attack_get(). Uninitialised: set by attack_add(). See "Page
 * data" in docs/memory_map.md.
 */
#include "game/components/attack_internal.h"

int8_t  attack_to_hit[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_dice[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_sides[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
int8_t  attack_to_damage[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_type[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_range[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
