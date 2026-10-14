#ifndef NGE_ENTITY_H
#define NGE_ENTITY_H

/**
 * @file nge_entity.h
 * @brief Entities: IDs with a lifecycle state, flags and a component mask.
 *
 * Lifecycle: FREE -> ALIVE -> DYING -> FREE.
 * - nge_entity_create() takes a free ID and makes it alive.
 * - nge_entity_destroy() makes an alive entity dying and pushes it onto the
 *   dying stack; systems no longer see it in the alive list.
 * - Clean-up pops dying IDs and releases them back to the free stack:
 *
 *       while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
 *           ...clean up; may destroy other entities...
 *           nge_entity_release(id);
 *       }
 *
 * Every entity has an owner (ecs/nge_owner.h): a new entity is owned by
 * NGE_OWNER_WORLD, and an entity must own nothing when it is released.
 *
 * The macros read the tables directly, with no checks: use them in loops
 * that run every frame. The functions are the checked versions (asserts on
 * the ID and state).
 *
 * Iterate the alive list with NGE_ENTITY_FOR_EACH_ALIVE: it goes backwards,
 * so destroying the current entity and creating entities inside the loop
 * are both safe.
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
typedef uint16_t nge_component_mask_t; /**< One bit per component; uint32_t for up to 32. */

/* State: bits 0-1 of the flags byte */
#define NGE_ENTITY_STATE_MASK  0x03
#define NGE_ENTITY_FREE        0x00
#define NGE_ENTITY_ALIVE       0x01
#define NGE_ENTITY_DYING       0x02

/* Flags: bits 2-7 */
#define NGE_ENTITY_FLAG_MASK   0xFC
#define NGE_ENTITY_PERSISTENT  0x04   /**< nge_entity_destroy() leaves the entity alive. */
#define NGE_ENTITY_FLAG_USER   0x08   /**< First of five flags for the game: 0x08 to 0x80. */

/** Component n (0-15) as a mask bit. */
#define NGE_COMPONENT(n)  ((nge_component_mask_t)1 << (n))

/*
 * Component numbers 0-3 are nge's:
 *   0  location (ecs/nge_location.h, NGE_COMPONENT_LOCATION)
 *   1-3 reserved
 * The game's are NGE_COMPONENT_USER (4) to 15.
 */
#define NGE_COMPONENT_USER  4

/* Tables in slot 0 (nge_entity_cdata.c): read-only for game code, written by the functions below */
extern nge_entity_flags_t   nge_entity_flags[NGE_ENTITY_MAX];      /**< Per entity. */
extern nge_component_mask_t nge_entity_components[NGE_ENTITY_MAX]; /**< Per entity. */
extern nge_entity_id_t      nge_entity_alive[NGE_ENTITY_MAX];      /**< Alive IDs, in no order. */
extern uint8_t              nge_entity_alive_count;                /**< Entries in nge_entity_alive. */

/* ---- Unchecked macros ---- */

#define NGE_ENTITY_STATE(id)     (nge_entity_flags[id] & NGE_ENTITY_STATE_MASK)
#define NGE_ENTITY_IS_ALIVE(id)  (NGE_ENTITY_STATE(id) == NGE_ENTITY_ALIVE)

/** All of the flags f are set. */
#define NGE_ENTITY_HAS_FLAGS(id, f)      ((nge_entity_flags[id] & (f)) == (f))
/** Any of the flags f is set. */
#define NGE_ENTITY_HAS_ANY_FLAGS(id, f)  ((nge_entity_flags[id] & (f)) != 0)

/** All of the components in mask m are set. */
#define NGE_ENTITY_HAS_COMPONENTS(id, m)      ((nge_entity_components[id] & (m)) == (m))
/** Any of the components in mask m is set. */
#define NGE_ENTITY_HAS_ANY_COMPONENTS(id, m)  ((nge_entity_components[id] & (m)) != 0)

/**
 * Loop over the alive entities, last to first: i is a uint8_t loop counter,
 * id receives each entity ID. Destroying the current entity moves the last
 * entry (already visited) into its place; entities created in the loop are
 * appended after the start point and first visited next time.
 */
#define NGE_ENTITY_FOR_EACH_ALIVE(i, id) \
    for ((i) = nge_entity_alive_count; (i)-- > 0 && (((id) = nge_entity_alive[i]), 1); )

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
 * @brief Request an alive entity's destruction: it becomes dying, leaves the
 *        alive list and waits on the dying stack for clean-up.
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
 * @brief Return a dying entity, after clean-up, to the free stack.
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

/** @brief Add components to a live or dying entity. */
void nge_entity_set_components(nge_entity_id_t id, nge_component_mask_t m);

/** @brief Remove components from a live or dying entity. */
void nge_entity_clear_components(nge_entity_id_t id, nge_component_mask_t m);

/** @brief 1 if all of the components in mask m are set. */
uint8_t nge_entity_has_components(nge_entity_id_t id, nge_component_mask_t m);

#endif
