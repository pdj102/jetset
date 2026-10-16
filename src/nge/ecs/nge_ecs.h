#ifndef NGE_ECS_H
#define NGE_ECS_H

/**
 * @file nge_ecs.h
 * @brief The ECS data page in slot 0, and ECS start-up.
 *
 * The ECS tables read every frame by several systems (entity flags,
 * components and alive list, owners, locations) are in page NGE_ECS_PAGE,
 * which nge_ecs_init() maps permanently into slot 0. They are declared in
 * the modules' slot 0 data files (nge_<module>_cdata.c, with the marker
 * nge-dataseg: PAGE_50) and start at $0100. See "ECS data in slot 0" in
 * docs/memory_map.md.
 *
 * $0000-$00FF is a guard area: with RAM in slot 0, a write through a null
 * pointer no longer crashes but silently changes memory. nge_ecs_init()
 * fills the guard area with a pattern and nge_ecs_check_guard() (debug
 * builds) halts if it has changed. Call it once a frame.
 *
 * Slot 0 is mapped out (the ROM is in) during CSpect debug output and
 * NextZXOS/esxDOS calls: code that runs then must not read ECS data.
 */

#include <stdint.h>

#define NGE_ECS_PAGE        50      /**< Page holding the ECS data (src/banks.inc). */
#define NGE_ECS_SLOT        0       /**< Slot it is mapped into: $0000-$1FFF. */
#define NGE_ECS_GUARD_SIZE  0x100   /**< Guard area at the bottom of the slot. */

/** The guard pattern at offset i: varies, so that writing one value over it is caught. */
#define NGE_ECS_GUARD_BYTE(i)  ((uint8_t)((i) ^ 0x5A))

#ifndef __SDCC
/* PC tests: the guard area is an array, since address 0 can't be used */
extern uint8_t nge_ecs_test_guard[NGE_ECS_GUARD_SIZE];
#endif

/**
 * @brief Map the ECS data page into slot 0, fill the guard area, and make
 *        every entity free (nge_entity_init()).
 *
 * Call once, after nge_hw_init() (interrupts must be disabled while RAM is
 * in slot 0) and before any other ECS function.
 */
void nge_ecs_init(void);

#ifdef NDEBUG
#define nge_ecs_check_guard() ((void)0)
#else
/** @brief Halt (abort) if the guard area has changed: a write through a null pointer. */
void nge_ecs_check_guard(void);
#endif

#endif
