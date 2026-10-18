#ifndef FEATURE_H
#define FEATURE_H

/**
 * @file feature.h
 * @brief The feature component: a fixed, interactive part of a level (a
 *        door; later chests, fountains, traps, stairs), and its state.
 *
 * A feature is an entity of CLASS_FEATURE (game/components/kind.h),
 * generated from its kind's base record (gen_feature(), game/gen/gen.h).
 * What a kind can do and how it looks are fixed by kind, in the record;
 * the component holds what is per entity: its state (FEAT_*).
 *
 * A feature is owned by an area and placed in it with feature_place(),
 * which gives it a location and draws it: features are drawn as map tiles
 * (its kind's look for its state), not sprites, so they use no hardware
 * sprites; the map's own tiles under a feature block nothing, and the
 * feature blocks through the block component (a closed door blocks
 * walking, so a move into it finds the door as the blocker). When a placed
 * feature is destroyed, the game puts back floor (level_put_floor()).
 * Features never move or change owner (they aren't items).
 *
 * feature_open() and feature_close() change the state without checking
 * the rules, keeping the block component and the tile in step: the actions
 * (act_open(), act_close(), game/actions/act.h) check the rules first.
 *
 * The tables are in slot 0 (feature_cdata.c). Banking: the functions are
 * banked (NGE_BANKED, page 110, with the other component functions).
 * Features are drawn into the one map (nge_map): the current level's.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_block.h"
#include "ecs/nge_entity.h"

/** Component bit: the game's ninth component (see nge_entity.h). */
#define COMPONENT_FEATURE  NGE_COMPONENT(NGE_COMPONENT_USER + 8)

/* State: bits of feature_state */
#define FEAT_OPEN    0x01   /**< Open (a door); else closed. */
/* Reserved: FEAT_LOCKED 0x02, FEAT_HIDDEN 0x04, FEAT_SPENT 0x08 */

/* What a kind can do: bits of its base record's `can` */
#define FCAN_OPEN    0x01   /**< Can be opened and closed. */

/* Tables in slot 0 (feature_cdata.c): read-only for game code; meaningful only with COMPONENT_FEATURE */
extern uint8_t feature_state[NGE_ENTITY_MAX];   /**< FEAT_* bits. */

/** The entity is a feature. */
#define FEATURE_IS(id)       NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_FEATURE)
/** A feature is open. */
#define FEATURE_IS_OPEN(id)  ((feature_state[id] & FEAT_OPEN) != 0)

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

#endif
