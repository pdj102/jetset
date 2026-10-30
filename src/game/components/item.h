#ifndef ITEM_H
#define ITEM_H

/**
 * @file item.h
 * @brief The item component: an entity that can be carried, where a carried
 *        one is (stowed in its holder's inventory or equipped in a slot),
 *        and its count.
 *
 * Where an item is follows from its owner (ecs/nge_owner.h), its location
 * and item_slot (see game/components/inventory.h):
 *
 *     on the floor   owned by an area, with a location
 *     stowed         owned by its holder, no location, SLOT_NONE
 *     equipped       owned by its holder, no location, a SLOT_* (later)
 *
 * A stack is one entity, as entities are few: its count is its missiles, or
 * a wand's charges, a lock pick's uses, a light's burn time or a treasure's
 * gold, by its subclass (game/gen/gen_kinds.h), 0 where it means nothing.
 * Merging and using up counts come later (docs/items.md, "Not built yet").
 *
 * Every entity gen_item() makes has it; effects, creatures and features
 * don't, so they can't be carried. The table is in slot 0 (item_cdata.c).
 *
 * Banking: item_add() is banked (NGE_BANKED, page 110, with the shared
 * generator helpers): it runs only when an item is created.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* Equipment slots (a holder's inv_slots and an item kind's fits are masks of SLOT_BIT()) */
#define SLOT_NONE     0    /**< Not equipped: stowed, or on the floor. */
#define SLOT_WEAPON   1
#define SLOT_SHIELD   2
#define SLOT_HEAD     3
#define SLOT_BODY     4
#define SLOT_HANDS    5
#define SLOT_FEET     6
#define SLOT_NECK     7
#define SLOT_RING_L   8
#define SLOT_RING_R   9
#define SLOT_LIGHT    10
#define SLOT_RANGED   11   /**< A launcher. */
#define SLOT_AMMO     12   /**< What it shoots, or what is thrown. */
#define SLOT_ARMS     13
#define SLOT_LEGS     14
#define SLOT_CLOAK    15
#define SLOT_COUNT    16   /**< Slots, SLOT_NONE included: at most 16, so the masks are full. */

/** Slot s (SLOT_WEAPON...) as a mask bit. */
#define SLOT_BIT(s)  ((uint16_t)1 << (s))

/** Every slot a humanoid has. */
#define SLOTS_HUMANOID \
    (SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_SHIELD) | SLOT_BIT(SLOT_HEAD) | SLOT_BIT(SLOT_BODY) | \
     SLOT_BIT(SLOT_HANDS) | SLOT_BIT(SLOT_FEET) | SLOT_BIT(SLOT_NECK) | SLOT_BIT(SLOT_RING_L) | \
     SLOT_BIT(SLOT_RING_R) | SLOT_BIT(SLOT_LIGHT) | SLOT_BIT(SLOT_RANGED) | SLOT_BIT(SLOT_AMMO) | \
     SLOT_BIT(SLOT_ARMS) | SLOT_BIT(SLOT_LEGS) | SLOT_BIT(SLOT_CLOAK))

typedef char item_slots_check[(SLOT_COUNT <= 16) ? 1 : -1];

/** The slot of each entity with COMPONENT_ITEM, in slot 0: SLOT_NONE unless equipped. Read-only for game code. */
extern uint8_t item_slot[NGE_ENTITY_MAX];

/** The count of each entity with COMPONENT_ITEM, in slot 0. */
extern uint8_t item_count[NGE_ENTITY_MAX];

/** The entity has the item component. */
#define ITEM_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_ITEM)

/** @brief Give an entity in use the item component: not equipped, with count. */
void item_add(nge_entity_id_t id, uint8_t count) NGE_BANKED;

#endif
