#ifndef ATTACK_INTERNAL_H
#define ATTACK_INTERNAL_H

/*
 * The attack component's tables, in page 111 (attack_pdata.c): readable
 * only by the component's functions in page 110 (attack.c), and the
 * tests. Other code calls attack_get(). [use][entity], meaningful only
 * with the use's component.
 */

#include "game/components/attack.h"

extern int8_t  attack_to_hit[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_dice[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_sides[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern int8_t  attack_bonus[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_type[ATTACK_USE_COUNT][NGE_ENTITY_MAX];
extern uint8_t attack_range[ATTACK_USE_COUNT][NGE_ENTITY_MAX];

#endif
