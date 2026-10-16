/* nge-dataseg: PAGE_50 */
/*
 * Timer tables and the clock in slot 0 (the ECS data page, see
 * ecs/nge_ecs.h): saved with the other ECS data. Uninitialised: set by
 * nge_timer_init() and nge_timer_add(). See "ECS data in slot 0" in
 * docs/memory_map.md.
 */
#include "ecs/systems/nge_timer.h"

uint8_t nge_timer_count[NGE_ENTITY_MAX];
uint8_t nge_timer_period[NGE_ENTITY_MAX];
uint8_t nge_timer_cycles[NGE_ENTITY_MAX];
uint8_t nge_timer_kind[NGE_ENTITY_MAX];

uint16_t nge_timer_turns;
uint8_t  nge_timer_tick;
uint8_t  nge_timer_halted;
