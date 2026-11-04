#ifndef FEATURE_H
#define FEATURE_H

/**
 * @file feature.h
 * @brief The feature component: a fixed, interactive part of a level (a
 *        door, stairs; later chests, fountains, traps), and its state.
 *
 * A feature is an entity of CLASS_FEATURE (game/components/kind.h),
 * generated from its kind record (gen_feature(), game/gen/gen.h).
 * What a kind can do and how it looks are fixed by kind, in the record;
 * the component holds what is per entity: its state (FEAT_*).
 *
 * A feature is owned by an area and placed in it with feature_place(),
 * which gives it a location and draws it: features are drawn as map tiles
 * (its kind's look for its state), not sprites, so they use no hardware
 * sprites; the map's own tiles under a feature block nothing, and the
 * feature blocks through the block component (a closed door blocks
 * walking, so a move into it finds the door as the blocker). When a placed
 * feature is destroyed, the game draws its cell's look again (level_redraw()).
 * Features never move or change owner (they aren't items).
 *
 * feature_open() and feature_close() change the state without checking
 * the rules, keeping the block component and the tile in step: the actions
 * (act_open(), act_close(), game/actions/act.h) check the rules first.
 *
 * A closed feature can be locked. The lock's kind (KEY_*) is in the state,
 * and so are the rules of what opens it: a key that fits, or a pick that
 * beats FEAT_PICK_DC. feature_lock() and feature_unlock() don't change how
 * it looks, and a locked door looks like any closed door. The level
 * generator locks side rooms only (game/level/level_gen.c).
 *
 * The tables are in slot 0 (feature_cdata.c). Banking: the functions are
 * banked (NGE_BANKED, page 132, a pair of its own).
 * Features are drawn into the one map (nge_map): the current level's.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_block.h"
#include "event/nge_event.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* State: bits of feature_state */
#define FEAT_OPEN    0x01   /**< Open (a door, a chest); else closed. */
#define FEAT_LOCKED  0x02   /**< Locked: it won't open. */
#define FEAT_HIDDEN  0x04   /**< Not yet found by the player: drawn as its cell's own look. */
#define FEAT_SPENT   0x08   /**< Used, with FCAN_ONCE: drawn with its used look. */
#define FEAT_KEY_SHIFT  4      /**< The lock's KEY_*, while locked: 3 bits. */
#define FEAT_KEY_MASK   0x70

/* What a lock pick must reach: 1d20 + its bonus + the picker's Dexterity modifier */
#define FEAT_PICK_DC  15

/*
 * What a kind can do: bits of its kind record's `can` (docs/features.md 4.2).
 * Written as constants: zsdcc types 1 << 7 as a signed char
 */
#define FCAN_OPEN      0x01   /**< Can be opened and closed. */
#define FCAN_ASCEND    0x02   /**< Leads up a level: stood on (act_ascend()). */
#define FCAN_DESCEND   0x04   /**< Leads down a level: stood on (act_descend()), or a trap that drops what it fires at. */
#define FCAN_OPERATE   0x08   /**< Can be operated: its use hook applies to the operator. */
#define FCAN_TRAP      0x10   /**< Fires at a walker that moves onto it: its attack, then its hit hook. */
#define FCAN_DISARM    0x20   /**< A trap that can be disarmed. */
#define FCAN_CONTAINER 0x40   /**< Holds items: they drop on its tile when it is opened or broken. */
#define FCAN_ONCE      0x80   /**< Spent after one use (FEAT_SPENT). */

/* What the messages say a feature is used by: values of its kind record's `verb` */
#define FVERB_NONE   0
#define FVERB_PRAY   1   /**< "You pray at the shrine." */
#define FVERB_DRINK  2   /**< "You drink from the fountain." */
#define FVERB_TOUCH  3   /**< "You touch the altar." */
#define FVERB_STEP   4   /**< A trap: "You step on a dart trap!" */
#define FVERB_FALL   5   /**< A trap: "You fall into a pit!" */
#define FVERB_COUNT  6

/* Tables in slot 0 (feature_cdata.c): read-only for game code; meaningful only with COMPONENT_FEATURE */
extern uint8_t feature_state[NGE_ENTITY_MAX];   /**< FEAT_* bits. */

/** The entity is a feature. */
#define FEATURE_IS(id)       NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_FEATURE)
/** A feature is open. */
#define FEATURE_IS_OPEN(id)  ((feature_state[id] & FEAT_OPEN) != 0)
/** A feature is locked. */
#define FEATURE_IS_LOCKED(id)  ((feature_state[id] & FEAT_LOCKED) != 0)
/** A feature is hidden: the player hasn't found it. */
#define FEATURE_IS_HIDDEN(id)  ((feature_state[id] & FEAT_HIDDEN) != 0)
/** A feature is spent: used, and it can be used only once. */
#define FEATURE_IS_SPENT(id)   ((feature_state[id] & FEAT_SPENT) != 0)

/** @brief Give an entity in use the feature component, in state (FEAT_*): not yet placed or drawn. */
void feature_add(nge_entity_id_t id, uint8_t state) NGE_BANKED;

/**
 * @brief Place a feature, owned by an area, on tile (x, y) of the map:
 *        its location, its blocking for its state, and its tile drawn.
 */
void feature_place(nge_entity_id_t id, uint8_t x, uint8_t y) NGE_BANKED;

/** @brief Open a placed feature: it blocks nothing, and is drawn open. */
void feature_open(nge_entity_id_t id) NGE_BANKED;

/** @brief Close a placed feature: it blocks what its kind blocks when closed, and is drawn closed. */
void feature_close(nge_entity_id_t id) NGE_BANKED;

/** @brief Draw a placed feature again, as its state says: over its cell's terrain, redrawn (level_redraw()). */
void feature_redraw(nge_entity_id_t id) NGE_BANKED;

/** @brief A hidden placed feature is found: drawn with its own look. */
void feature_find(nge_entity_id_t id) NGE_BANKED;

/** @brief A placed feature is spent: drawn with its used look, blocking what it did. */
void feature_spend(nge_entity_id_t id) NGE_BANKED;

/** @brief Lock a closed feature, with a lock of kind key (KEY_IRON to KEY_GOLD). */
void feature_lock(nge_entity_id_t id, uint8_t key) NGE_BANKED;

/** @brief Unlock a feature: it stays closed. */
void feature_unlock(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief The features' reactions to events (docs/features.md 4.7): a
 *        container opened (EV_OPENED) drops what it holds on its tile.
 *        A broken one needs nothing: the clean-up drops a dying holder's
 *        items. game_on_event() calls it only for the events it handles.
 */
void feature_on_event(const nge_event_t *e) NGE_BANKED;

/** @brief 1 if item is a key that fits locked feature id: its KEY_*, or KEY_ANY. */
uint8_t feature_key_fits(nge_entity_id_t id, nge_entity_id_t item) NGE_BANKED;

/** @brief The first item holder has stowed that fits locked feature id, or NGE_ENTITY_INVALID. */
nge_entity_id_t feature_fitting_key(nge_entity_id_t id, nge_entity_id_t holder) NGE_BANKED;

/** @brief 1 if a lock pick with d20 roll beats a lock, for actor: its Dexterity modifier counts, if it has stats. */
uint8_t feature_pick_works(nge_entity_id_t actor, nge_entity_id_t pick, uint8_t roll) NGE_BANKED;

#endif
