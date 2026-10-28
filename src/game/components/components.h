#ifndef COMPONENTS_H
#define COMPONENTS_H

/**
 * @file components.h
 * @brief The game's components: their numbers, in one list, and the shape
 *        each one follows.
 *
 * What a component is, and how nge keeps its data, is in
 * ecs/components/nge_components.h. A game component is:
 *
 * - a number in the list below, NGE_COMPONENT_USER + n, and its handle
 *   COMPONENT_<NAME>, one component (never OR two: test each with
 *   NGE_ENTITY_HAS_COMPONENT()). A new one takes the next number, COMPONENT_COUNT,
 *   and COMPONENT_COUNT goes up by one; the compiler stops the game going
 *   past NGE_COMPONENT_MAX;
 * - a header, game/components/<name>.h, which includes this file and
 *   declares its tables, its macros and its functions;
 * - its tables, one per field and [NGE_ENTITY_MAX], in slot 0
 *   (components/<name>_cdata.c) when common code or several systems read
 *   them, or in its module's page data when only that module does (the
 *   active effects' kinds, game/effects). Read-only outside the component.
 *   A single component, which only one entity ever has (the player's,
 *   pc.h), has one variable per field instead;
 * - <name>_add(), which gives an entity the component, from its kind's
 *   kind record (game/gen) or a start state;
 * - functions that hold its rules (health_damage(), attack_hits()),
 *   called by every action and effect that needs them, banked in page 110
 *   (the component's .c file) and asserting on their entity;
 * - a macro in its header that tests for it (ITEM_IS(), FEATURE_IS(),
 *   INV_HAS()), used by other code instead of the handle.
 *
 * Model components: game/components/health.h (kind record, rules,
 * events) and game/components/feature.h (state, kept in step with the
 * map).
 */

#include "ecs/components/nge_components.h"

/* The game's components: numbers NGE_COMPONENT_USER + 0 to + COMPONENT_COUNT-1 */
#define COMPONENT_KIND       NGE_COMPONENT(NGE_COMPONENT_USER + 0)   /**< The class and kind it was generated from (kind.h). */
#define COMPONENT_STATS      NGE_COMPONENT(NGE_COMPONENT_USER + 1)   /**< Speed and the six abilities (stats.h). */
#define COMPONENT_ITEM       NGE_COMPONENT(NGE_COMPONENT_USER + 2)   /**< Can be carried and equipped (item.h). */
#define COMPONENT_INVENTORY  NGE_COMPONENT(NGE_COMPONENT_USER + 3)   /**< Can hold items (inventory.h). */
#define COMPONENT_HEALTH     NGE_COMPONENT(NGE_COMPONENT_USER + 4)   /**< Hit points, armour class, damage masks (health.h). */
#define COMPONENT_MELEE      NGE_COMPONENT(NGE_COMPONENT_USER + 5)   /**< A melee attack (attack.h). */
#define COMPONENT_RANGED     NGE_COMPONENT(NGE_COMPONENT_USER + 6)   /**< A ranged attack (attack.h). No kind has one yet. */
#define COMPONENT_EFFECT     NGE_COMPONENT(NGE_COMPONENT_USER + 7)   /**< An active effect (game/effects/effects.h). */
#define COMPONENT_FEATURE    NGE_COMPONENT(NGE_COMPONENT_USER + 8)   /**< A fixed, interactive part of a level (feature.h). */
#define COMPONENT_MANA       NGE_COMPONENT(NGE_COMPONENT_USER + 9)   /**< Mana points, spent on spells (mana.h). */
#define COMPONENT_PC         NGE_COMPONENT(NGE_COMPONENT_USER + 10)  /**< The player character: name, level, XP, gold (pc.h). */
#define COMPONENT_AI         NGE_COMPONENT(NGE_COMPONENT_USER + 11)  /**< The game decides its turns: monsters (ai.h). */
#define COMPONENT_COUNT      12  /**< The game's components: the next is NGE_COMPONENT_USER + COMPONENT_COUNT. */

/* Every game component has a number below NGE_COMPONENT_MAX */
typedef char components_count_check[(NGE_COMPONENT_USER + COMPONENT_COUNT <= NGE_COMPONENT_MAX) ? 1 : -1];

#endif
