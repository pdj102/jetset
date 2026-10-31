#ifndef NGE_OWNER_H
#define NGE_OWNER_H

/**
 * @file nge_owner.h
 * @brief Ownership: every entity in use has exactly one owner.
 *
 * An owner is one of:
 * - an entity (0 to NGE_ENTITY_MAX-1), e.g. a player's inventory or a chest;
 * - NGE_OWNER_WORLD: in the game but not placed, e.g. a quest item that has
 *   yet to spawn; new entities start here;
 * - NGE_OWNER_AREA(a): placed in area a, e.g. a level or room. A position in
 *   the area is a separate component.
 *
 * An owner may own any number of entities. Ownership is stored once, as the
 * owner of each entity: there are no lists of owned entities. Find them by
 * scanning the used list with NGE_OWNER_FOR_EACH_OWNED, cheap for
 * NGE_ENTITY_MAX entities and done only when something changes hands.
 *
 * Ownership never forms a cycle: nge_owner_set() and nge_owner_move_all()
 * refuse to make an entity own itself or one of its owners.
 *
 * An entity must own nothing when it is released: in the clean-up loop,
 * destroy what it owns or move it on, for example to its own owner (which
 * works even when that owner is dying too, whatever the release order):
 *
 *       nge_owner_move_all(id, NGE_OWNER_OF(id));
 *       nge_entity_release(id);
 *
 * Banking: the functions that change or search ownership are banked
 * (NGE_BANKED, page 48, with the entity lifecycle). nge_owner_get() is in
 * common memory; the table is in slot 0, in the ECS data page
 * (ecs/nge_ecs.h).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

#ifndef NGE_OWNER_AREA_MAX
/** Number of areas: NGE_ENTITY_MAX + 1 + NGE_OWNER_AREA_MAX is at most 255 (checked at compile time). */
#define NGE_OWNER_AREA_MAX  64
#endif

typedef uint8_t nge_owner_t;   /**< An entity ID, NGE_OWNER_WORLD or NGE_OWNER_AREA(a). */

/** In the game but not placed in an area. */
#define NGE_OWNER_WORLD     ((nge_owner_t)NGE_ENTITY_MAX)
/** Area a, 0 to NGE_OWNER_AREA_MAX-1. */
#define NGE_OWNER_AREA(a)   ((nge_owner_t)(NGE_ENTITY_MAX + 1 + (a)))
/** Number of owner values: valid owners are below this. */
#define NGE_OWNER_COUNT     (NGE_ENTITY_MAX + 1 + NGE_OWNER_AREA_MAX)

#define NGE_OWNER_IS_ENTITY(o)  ((o) < NGE_ENTITY_MAX)
#define NGE_OWNER_IS_AREA(o)    ((o) > NGE_OWNER_WORLD && (o) < NGE_OWNER_COUNT)
/** The area number of an area owner. */
#define NGE_OWNER_AREA_OF(o)    ((uint8_t)((o) - (NGE_ENTITY_MAX + 1)))

/** Owner of each entity in use, in slot 0 (nge_owner_cdata.c): read-only for game code. Not meaningful for free entities. */
extern nge_owner_t nge_owner_of[NGE_ENTITY_MAX];

/* ---- Unchecked macros ---- */

/** The owner of entity id. */
#define NGE_OWNER_OF(id)  (nge_owner_of[id])

/**
 * Loop over the alive entities owned directly by owner, last to first: i is
 * a uint8_t loop counter, id receives each entity ID. The same rules as
 * NGE_ENTITY_FOR_EACH_ALIVE apply; dying entities are not visited.
 */
#define NGE_OWNER_FOR_EACH_OWNED(i, id, owner) \
    NGE_ENTITY_FOR_EACH_ALIVE(i, id) if (nge_owner_of[id] != (owner)) {} else

/* ---- Functions ---- */

/** @brief The owner of an entity in use (alive or dying). */
nge_owner_t nge_owner_get(nge_entity_id_t id);

/**
 * @brief Give an entity in use (alive or dying) a new owner.
 *
 * The owner must be valid and, if it is an entity, in use (alive or dying:
 * a dying owner must pass the entity on before it is released), and neither id
 * itself nor owned by id (directly or indirectly): asserts otherwise.
 */
void nge_owner_set(nge_entity_id_t id, nge_owner_t owner) NGE_BANKED;

/**
 * @brief 1 if owner owns entity id directly or through other entities
 *        (an item in a bag carried by the player is owned by the player).
 */
uint8_t nge_owner_owns(nge_owner_t owner, nge_entity_id_t id) NGE_BANKED;

/** @brief Number of entities in use (alive or dying) owned directly by owner. */
uint8_t nge_owner_count(nge_owner_t owner) NGE_BANKED;

/**
 * @brief Move every entity in use (alive or dying) owned directly by from
 *        to to, e.g. a destroyed chest's contents into its area.
 *
 * to follows the rules of nge_owner_set(): valid, in use if an entity, and
 * neither from nor owned by from.
 * @return The number of entities moved.
 */
uint8_t nge_owner_move_all(nge_owner_t from, nge_owner_t to) NGE_BANKED;

#endif
