/*
 * ECS start-up: maps the ECS data page into slot 0 and keeps a guard area
 * against null pointer writes. In common memory: nge_ecs_init() maps slot 0
 * and must not be in it.
 */
#include "ecs/nge_ecs.h"

#include "core/nge_util.h"
#include "ecs/nge_entity.h"
#include "hw/nge_mmu.h"

#ifdef __SDCC
#define GUARD  ((volatile uint8_t *)0x0000)
#else
uint8_t nge_ecs_test_guard[NGE_ECS_GUARD_SIZE];
#define GUARD  nge_ecs_test_guard
#endif

void nge_ecs_init(void)
{
    uint8_t i = 0;

    nge_mmu_map(NGE_ECS_SLOT, NGE_ECS_PAGE);
    do {
        GUARD[i] = NGE_ECS_GUARD_BYTE(i);
    } while (++i != 0);

    nge_entity_init();
}

#ifndef NDEBUG
void nge_ecs_check_guard(void)
{
    uint8_t i = 0;

    do {
        if (GUARD[i] != NGE_ECS_GUARD_BYTE(i)) {
            nge_util_abort("ECS guard area at $0000 changed: write through a null pointer?");
        }
    } while (++i != 0);
}
#endif
