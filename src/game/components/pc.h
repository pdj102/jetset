#ifndef PC_H
#define PC_H

/**
 * @file pc.h
 * @brief The player character component (PC): what only the player has,
 *        their name, class, experience level, experience points (XP), gold
 *        and the deepest depth they reached. See docs/player_character.md.
 *
 * A single component: only the player has it, so each field is one
 * variable, not a table by entity (game/components/components.h). A value
 * goes here only if no other entity could ever have it; what a monster,
 * item or feature could have is in the shared components (stats, health,
 * mana, inventory, attack), even when only the player has it today.
 *
 * The creature generator gives it to the player's kind (CREATURE_PLAYER,
 * game/gen): level 1, a fighter, no XP or gold, depth 1, named "Rogue"
 * until pc_set_name(). PC_IS() tells the player from other entities in
 * components and actions.
 *
 * XP come from killing monsters (each kind's XP value, game/gen) and from
 * gold, one XP a gold piece, as in classic D&D. When they reach the level
 * table's figure for the next level (pc_xp_needed()), the player goes up a
 * level: maximum hit points rise by a roll of the hit die plus the
 * Constitution modifier, at least 1, and current hit points by the same.
 * The class is a placeholder: nothing reads it until there are classes.
 *
 * Events (game/events.h), each after its change: EV_XP_GAINED, then
 * EV_LEVEL_GAINED for each level; EV_GOLD_CHANGED, before the XP that gold
 * gives.
 *
 * The variables are in slot 0 (pc_cdata.c). Banking: the functions are
 * banked (NGE_BANKED, page 110, with the other component functions).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

#define PC_NAME_MAX   6    /**< Longest name: it follows "Name: " on one stats line. */
#define PC_LEVEL_MIN  1
#define PC_LEVEL_MAX  10
#define PC_XP_MAX     65535u   /**< XP stop here. */
#define PC_GOLD_MAX   65535u   /**< Gold stops here. */
#define PC_HIT_DIE    8        /**< The die rolled for each level's hit points (later: by class). */

/* Classes: values of pc_class, a placeholder until character generation */
#define PC_CLASS_FIGHTER  0
#define PC_CLASS_COUNT    1

/* Variables in slot 0 (pc_cdata.c): read-only outside the component; meaningful once the player has it */
extern char     pc_name[PC_NAME_MAX + 1];   /**< 1 to PC_NAME_MAX characters. */
extern uint8_t  pc_class;                   /**< PC_CLASS_*: not read yet. */
extern uint8_t  pc_level;                   /**< PC_LEVEL_MIN to PC_LEVEL_MAX. */
extern uint16_t pc_xp;
extern uint16_t pc_gold;
extern uint8_t  pc_depth_max;               /**< The deepest depth reached, 1 or more. */

/** Entity id is the player: it has the PC component. */
#define PC_IS(id)  NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_PC)

/** The player has at least n gold. */
#define PC_HAS_GOLD(n)  (pc_gold >= (n))

/**
 * @brief Give an entity in use (alive, with health and stats: the
 *        player) the PC component, as a new character: level 1, a
 *        fighter, no XP or gold, depth 1, named "Rogue".
 */
void pc_add(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief Name the player: name's first PC_NAME_MAX characters.
 * @param name 1 character or more; in common memory or slot 0.
 */
void pc_set_name(nge_entity_id_t id, const char *name) NGE_BANKED;

/**
 * @brief The XP level needs (PC_LEVEL_MIN to PC_LEVEL_MAX): 0 for level 1.
 */
uint16_t pc_xp_needed(uint8_t level) NGE_BANKED;

/**
 * @brief The player gains amount XP, up to PC_XP_MAX: posts EV_XP_GAINED
 *        (the XP gained, p1 low byte, p2 high byte) if any were, then goes
 *        up each level they reach, posting EV_LEVEL_GAINED (p1: the new level)
 *        for each.
 */
void pc_gain_xp(nge_entity_id_t id, uint16_t amount) NGE_BANKED;

/**
 * @brief The player gains amount gold, up to PC_GOLD_MAX: posts
 *        EV_GOLD_CHANGED if any was, then gains as many XP (pc_gain_xp()).
 */
void pc_gain_gold(nge_entity_id_t id, uint16_t amount) NGE_BANKED;

/**
 * @brief The player spends amount gold, which they have (the caller checks
 *        PC_HAS_GOLD(): asserted; without asserts it stops at 0). Posts
 *        EV_GOLD_CHANGED unless amount is 0. The XP stay.
 */
void pc_spend_gold(nge_entity_id_t id, uint16_t amount) NGE_BANKED;

/** @brief The player reached depth: pc_depth_max becomes the deeper of the two. */
void pc_reach_depth(nge_entity_id_t id, uint8_t depth) NGE_BANKED;

#endif
