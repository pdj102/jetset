#ifndef ITEM_H
#define ITEM_H

/**
 * @file item.h
 * @brief The item component: an entity that can be carried, and where a
 *        carried one is: stowed in its holder's inventory or equipped in a
 *        slot.
 *
 * Where an item is follows from its owner (ecs/nge_owner.h), its location
 * and item_slot (see game/components/inventory.h):
 *
 *     on the floor   owned by an area, with a location
 *     stowed         owned by its holder, no location, SLOT_NONE
 *     equipped       owned by its holder, no location, a SLOT_* (later)
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

/** Component bit: the game's third component (see nge_entity.h). */
#define COMPONENT_ITEM  NGE_COMPONENT(NGE_COMPONENT_USER + 2)

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
#define SLOT_COUNT    11   /**< Slots, SLOT_NONE included: at most 16. */

/** Slot s (SLOT_WEAPON...) as a mask bit. */
#define SLOT_BIT(s)  ((uint16_t)1 << (s))

/** Every slot a humanoid has. */
#define SLOTS_HUMANOID \
    (SLOT_BIT(SLOT_WEAPON) | SLOT_BIT(SLOT_SHIELD) | SLOT_BIT(SLOT_HEAD) | SLOT_BIT(SLOT_BODY) | \
     SLOT_BIT(SLOT_HANDS) | SLOT_BIT(SLOT_FEET) | SLOT_BIT(SLOT_NECK) | SLOT_BIT(SLOT_RING_L) | \
     SLOT_BIT(SLOT_RING_R) | SLOT_BIT(SLOT_LIGHT))

typedef char item_slots_check[(SLOT_COUNT <= 16) ? 1 : -1];

/** The slot of each entity with COMPONENT_ITEM, in slot 0: SLOT_NONE unless equipped. Read-only for game code. */
extern uint8_t item_slot[NGE_ENTITY_MAX];

/** The entity has the item component. */
#define ITEM_IS(id)  NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_ITEM)

/** @brief Give an entity in use the item component: not equipped. */
void item_add(nge_entity_id_t id) NGE_BANKED;

#endif
