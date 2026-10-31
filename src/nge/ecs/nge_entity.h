#ifndef NGE_ENTITY_H
#define NGE_ENTITY_H

/**
 * @file nge_entity.h
 * @brief Entities: IDs with a lifecycle state, flags and components.
 *
 * Lifecycle: FREE -> ALIVE -> DYING -> FREE.
 * - nge_entity_create() takes a free ID and makes it alive.
 * - nge_entity_destroy() makes an alive entity dying and pushes it onto the
 *   dying stack. It stays in the used list, but the alive loops skip it, so
 *   systems no longer see it.
 * - Clean-up empties the dying stack, releasing each ID to the free stack,
 *   and only then does it leave the used list:
 *
 *       NGE_ENTITY_RELEASE_EACH_DYING(id) {
 *           ...clean up; may destroy other entities...
 *       }
 *
 * Every entity has an owner (ecs/nge_owner.h): a new entity is owned by
 * NGE_OWNER_WORLD, and an entity must own nothing when it is released.
 *
 * The macros read the tables directly, with no checks: use them in loops
 * that run every frame. The functions are the checked versions (asserts on
 * the ID and state).
 *
 * Loops over the used list:
 * - NGE_ENTITY_FOR_EACH_ALIVE: alive entities, what most loops want;
 * - NGE_ENTITY_FOR_EACH_IN_USE: alive and dying entities.
 *
 * Only release moves entries in the used list, so inside these loops any
 * entity may be destroyed and new ones created, and no entity is visited
 * twice. Never release inside one: only clean-up releases, outside them.
 * A body that calls something which may destroy the current entity tests
 * NGE_ENTITY_IS_ALIVE(id) before going on with it.
 *
 * Banking: the lifecycle functions (init, create, destroy, next_dying,
 * release, free_count, dying_count) are banked (NGE_BANKED, page 48, with
 * their bookkeeping in page 49): don't call them while game code has its own
 * page mapped in slot 6 or 7. The flag and component functions are in common
 * memory. The tables are in slot 0, in the ECS data page (ecs/nge_ecs.h):
 * call nge_ecs_init() first, which maps it and calls nge_entity_init().
 */

#include <stdint.h>

#include "core/nge_bank.h"

#ifndef NGE_ENTITY_MAX
/** Number of entities, at most 250 (checked at compile time). */
#define NGE_ENTITY_MAX  100
#endif

/** Not an entity: returned when there is none to give. */
#define NGE_ENTITY_INVALID  0xFF

typedef uint8_t  nge_entity_id_t;      /**< Entity ID, 0 to NGE_ENTITY_MAX-1. */
typedef uint8_t  nge_entity_flags_t;   /**< State (bits 0-1) and flags (bits 2-7). */
typedef uint16_t nge_component_t;      /**< One component: its table (high byte) and bit (low byte); 0 for none. */

#ifndef NGE_COMPONENT_TABLES
/** Component tables, 8 components each (ecs/components/nge_components.h). */
#define NGE_COMPONENT_TABLES  3
#endif

/* State: bits 0-1 of the flags byte */
#define NGE_ENTITY_STATE_MASK  0x03
#define NGE_ENTITY_FREE        0x00
#define NGE_ENTITY_ALIVE       0x01
#define NGE_ENTITY_DYING       0x02

/* Flags: bits 2-7 */
#define NGE_ENTITY_FLAG_MASK   0xFC
#define NGE_ENTITY_PERSISTENT  0x04   /**< nge_entity_destroy() leaves the entity alive. */
#define NGE_ENTITY_FLAG_USER   0x08   /**< First of five flags for the game: 0x08 to 0x80. */

/**
 * Component n's handle: table n / 8 in the high byte, bit n % 8 in the low
 * byte. A handle names one component: never OR two, as handles in different
 * tables would test the wrong one. The shifts are unsigned: zsdcc types
 * 1 << 7 as a signed char, -128, which would make table 255. The numbers,
 * nge's and the game's range, and what a component is, are in
 * ecs/components/nge_components.h.
 */
#define NGE_COMPONENT(n)  ((nge_component_t)(((nge_component_t)((n) >> 3) << 8) | ((nge_component_t)1 << ((n) & 7))))

/* Tables in slot 0 (nge_entity_cdata.c): read-only for game code, written by the functions below */
extern nge_entity_flags_t   nge_entity_flags[NGE_ENTITY_MAX];      /**< Per entity. */
extern uint8_t              nge_entity_components[NGE_COMPONENT_TABLES][NGE_ENTITY_MAX]; /**< Per table, per entity: use the macros. */
extern nge_entity_id_t      nge_entity_used[NGE_ENTITY_MAX];       /**< IDs in use (alive or dying), in no order. */
extern uint8_t              nge_entity_used_count;                 /**< Entries in nge_entity_used. */

/* ---- Unchecked macros ---- */

#define NGE_ENTITY_STATE(id)     (nge_entity_flags[id] & NGE_ENTITY_STATE_MASK)
#define NGE_ENTITY_IS_ALIVE(id)  (NGE_ENTITY_STATE(id) == NGE_ENTITY_ALIVE)

/** All of the flags f are set. */
#define NGE_ENTITY_HAS_FLAGS(id, f)      ((nge_entity_flags[id] & (f)) == (f))
/** Any of the flags f is set. */
#define NGE_ENTITY_HAS_ANY_FLAGS(id, f)  ((nge_entity_flags[id] & (f)) != 0)

/** The byte of entity id's table that holds component c. */
#define NGE_ENTITY_COMPONENT_BYTE(id, c)  (nge_entity_components[(c) >> 8][id])

/** The entity has component c. With a constant c, one byte test at a fixed address plus id. */
#define NGE_ENTITY_HAS_COMPONENT(id, c)     ((NGE_ENTITY_COMPONENT_BYTE(id, c) & (uint8_t)(c)) != 0)
/** Give the entity component c: for the component's add function. */
#define NGE_ENTITY_ADD_COMPONENT(id, c)     (NGE_ENTITY_COMPONENT_BYTE(id, c) |= (uint8_t)(c))
/** Take component c away: for the component's remove function. */
#define NGE_ENTITY_REMOVE_COMPONENT(id, c)  (NGE_ENTITY_COMPONENT_BYTE(id, c) &= (uint8_t)~(c))

/**
 * Loop over the entities in use, alive and dying, last to first: i is a
 * uint8_t loop counter, id receives each entity ID. Entries don't move during
 * the loop, as only release moves them; entities created in the loop are
 * appended after the start point and first visited next time.
 */
#define NGE_ENTITY_FOR_EACH_IN_USE(i, id) \
    for ((i) = nge_entity_used_count; (i)-- > 0 && (((id) = nge_entity_used[i]), 1); )

/**
 * Loop over the alive entities, as NGE_ENTITY_FOR_EACH_IN_USE. An entity
 * destroyed before the loop reaches it is not visited.
 */
#define NGE_ENTITY_FOR_EACH_ALIVE(i, id) \
    NGE_ENTITY_FOR_EACH_IN_USE(i, id) if (!NGE_ENTITY_IS_ALIVE(id)) {} else

/**
 * Clean-up: take each dying entity, run the body, and release it, until the
 * dying stack is empty. id receives each entity ID, last destroyed first. The
 * body may destroy entities, which this same loop then handles, and must
 * leave id owning nothing. continue is fine: the release is in the loop. Don't
 * break or return out of it, which would lose the entity taken.
 */
#define NGE_ENTITY_RELEASE_EACH_DYING(id) \
    for ((id) = nge_entity_next_dying(); (id) != NGE_ENTITY_INVALID; \
         nge_entity_release(id), (id) = nge_entity_next_dying())

/* ---- Lifecycle ---- */

/** @brief Make every entity free, with no flags or components, owned by the world. */
void nge_entity_init(void) NGE_BANKED;

/**
 * @brief Create an entity: alive, with no flags or components, owned by
 *        NGE_OWNER_WORLD.
 * @return Its ID, or NGE_ENTITY_INVALID if all NGE_ENTITY_MAX are in use.
 *         IDs are handed out 0, 1, 2... after nge_entity_init().
 */
nge_entity_id_t nge_entity_create(void) NGE_BANKED;

/**
 * @brief Request an alive entity's destruction: it becomes dying and waits on
 *        the dying stack for clean-up. The used list doesn't change.
 * @return 1 if it became dying; 0 if it was already dying or is persistent
 *         (clear NGE_ENTITY_PERSISTENT first to destroy it).
 */
uint8_t nge_entity_destroy(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief Take the next entity waiting for clean-up off the dying stack.
 * @return Its ID, or NGE_ENTITY_INVALID when none is waiting.
 */
nge_entity_id_t nge_entity_next_dying(void) NGE_BANKED;

/**
 * @brief Return a dying entity, after clean-up, to the free stack, taking it
 *        off the used list: the last entry moves into its place.
 *
 * The entity must have been taken with nge_entity_next_dying() and must own
 * no entities (nge_owner_count() is 0).
 */
void nge_entity_release(nge_entity_id_t id) NGE_BANKED;

/** @brief Number of free entities. */
uint8_t nge_entity_free_count(void) NGE_BANKED;

/** @brief Number of entities waiting on the dying stack. */
uint8_t nge_entity_dying_count(void) NGE_BANKED;

/* ---- Checked flag and component access ---- */

/** @brief Set flags (bits 2-7 only) on a live or dying entity. */
void nge_entity_set_flags(nge_entity_id_t id, nge_entity_flags_t f);

/** @brief Clear flags (bits 2-7 only) on a live or dying entity. */
void nge_entity_clear_flags(nge_entity_id_t id, nge_entity_flags_t f);

/** @brief 1 if all of the flags f are set. */
uint8_t nge_entity_has_flags(nge_entity_id_t id, nge_entity_flags_t f);

/** @brief Add component c to a live or dying entity. */
void nge_entity_set_component(nge_entity_id_t id, nge_component_t c);

/** @brief Remove component c from a live or dying entity. */
void nge_entity_clear_component(nge_entity_id_t id, nge_component_t c);

/** @brief 1 if the entity has component c. */
uint8_t nge_entity_has_component(nge_entity_id_t id, nge_component_t c);

#endif
