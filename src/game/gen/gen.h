#ifndef GEN_H
#define GEN_H

/**
 * @file gen.h
 * @brief Entity generators: create an entity of a given kind, with all
 *        its components, from its base data.
 *
 * Each class of entity (creatures, items; features later) has a table of
 * base entities: constant records, one per kind (gen_kinds.h), defined as
 * data in the class's page data file (gen_<class>_pdata.c). A generator
 * creates an entity, gives it its owner and its kind (game/components/
 * kind.h), adds every component the kind needs from the record (its
 * values the same for every entity of the kind), and returns it:
 *
 *     id = gen_creature(CR_RAT, LEVEL_AREA);
 *     if (id != NGE_ENTITY_INVALID) {
 *         nge_location_set(id, x, y);      // placing it is the caller's job
 *     }
 *
 * The owner is required: an area, an entity (a chest, the player's pack) or
 * NGE_OWNER_WORLD. A generator never sets a location. The _random versions
 * choose a kind that appears at the depth, weighted by its rarity.
 *
 * Base data that never changes and isn't needed every turn stays in the
 * table: read it through functions like gen_name(), which copies into the
 * caller's buffer (on the stack, in common memory).
 *
 * Banking: each class's generator is banked with its table (creatures: code
 * page 106, table page 107; items: 108/109). Don't call them while game
 * code has its own page mapped in slot 6 or 7. gen_name() is in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "game/gen/gen_kinds.h"

/**
 * @brief Create a creature of kind (CR_*), owned by owner: sprite, block,
 *        stats, health, timer (the player's or a monster's turn) and, if it
 *        can carry things, inventory; then its innate effect, if any
 *        (GEN_HOOK_INNATE). The player is persistent: at 0 hp it isn't
 *        destroyed (game over instead).
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free.
 */
nge_entity_id_t gen_creature(uint8_t kind, nge_owner_t owner) NGE_BANKED;

/**
 * @brief Create a random creature that appears at depth, weighted by rarity.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free or no kind appears there.
 */
nge_entity_id_t gen_creature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED;

/**
 * @brief Create an item of kind (IT_*), owned by owner: its sprite, the
 *        item component (not equipped) and, if its kind has hit points,
 *        health.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free.
 */
nge_entity_id_t gen_item(uint8_t kind, nge_owner_t owner) NGE_BANKED;

/** @brief Create a random item that appears at depth, weighted by rarity (see gen_creature_random()). */
nge_entity_id_t gen_item_random(uint8_t depth, nge_owner_t owner) NGE_BANKED;

/**
 * @brief The equipment slots an item kind (IT_*) can go in: a mask of
 *        SLOT_BIT() (game/components/item.h), 0 if it can't be equipped.
 *        Banked with the item table (page 108).
 */
uint16_t gen_item_fits(uint8_t kind) NGE_BANKED;

/* Hooks: the effect (EF_*, game/effects/effects.h) a kind's base record names for each */
#define GEN_HOOK_USE     0   /**< Items: applied to the user when used up (drunk, eaten: act_use()). */
#define GEN_HOOK_HIT     1   /**< Weapons and creatures: applied to what a hit leaves alive (act_melee()). */
#define GEN_HOOK_WORN    2   /**< Items: its modifiers and statuses count while equipped. */
#define GEN_HOOK_INNATE  3   /**< Creatures: applied when generated. */
#define GEN_HOOKS        4

/**
 * @brief The effect an entity's kind names for hook (GEN_HOOK_*): EF_NONE
 *        (0) if none, or if the entity has no kind. In common memory: it
 *        calls its class's banked copy (pages 106, 108).
 */
uint8_t gen_hook(nge_entity_id_t id, uint8_t hook);

/** @brief An item kind's (IT_*) subclass (ITEM_POTION...). Banked with the item table (page 108). */
uint8_t gen_item_subclass(uint8_t kind) NGE_BANKED;

/**
 * @brief Copy the name of an entity's kind into buf: at most size-1
 *        characters and a NUL ("something" for an entity with no kind).
 * @param size 1 or more.
 */
void gen_name(nge_entity_id_t id, char *buf, uint8_t size);

#endif
