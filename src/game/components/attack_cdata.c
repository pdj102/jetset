/* nge-dataseg: PAGE_50 */
/*
 * The attack component's tables in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h). Uninitialised: set by attack_add(). See "ECS data in
 * slot 0" in docs/memory_map.md.
 */
#include "game/components/attack.h"

int8_t  attack_to_hit[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_dice[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_sides[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
int8_t  attack_bonus[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_type[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
uint8_t attack_range[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
