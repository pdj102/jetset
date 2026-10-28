#ifndef ATTRS_H
#define ATTRS_H

/**
 * @file attrs.h
 * @brief Attributes: the numbers and masks that equipment and effects
 *        change, numbered so effect definitions can name them.
 *
 * An attribute is a number (or a mask) that equipment and effects can
 * change: max hit points, AC, speed, the abilities, the damage masks.
 * Each has, per entity:
 *
 *   kind value  constant, in its kind record (health_kind_t.hp_max):
 *               sets the base when the entity is created
 *   base        changed only for good, by effects_set_base() (a potion
 *               of vitality): health_hp_max_base
 *   value       base + the modifiers of equipped items' worn effects and
 *               of active effects, clamped: health_hp_max. The one rules
 *               read; only effects_recompute() (game/effects/effects.h)
 *               writes it, and it is recomputed whenever a source changes
 *               (equip, an effect starts or ends, a base changes)
 *
 * A pool (hit points, later MP) also has a current amount (health_hp):
 * not an attribute, but a resource that damage and healing change, kept
 * within the value. A lower maximum takes it down; a higher one doesn't
 * heal.
 *
 * The tables live in the component each attribute belongs to, meaningful
 * only for an entity with that component; this file only numbers them
 * (ATTR_*). The components set the bases and values from the kind values
 * (health_add(), stats_add()); the effects module does the rest, finding
 * each attribute's base table through effects_game_attrs[]
 * (game/effects/effects_core.h).
 */

#include <stdint.h>

/* Attributes: numbers add up; masks (immune, resist, vuln) are ORed */
#define ATTR_HP_MAX  0   /**< health: health_hp_max, health_hp_max_base; 1-255. */
#define ATTR_AC      1   /**< health: health_ac, health_ac_base; 0-255. */
#define ATTR_SPEED   2   /**< stats: stats_speed, stats_speed_base, and the timer's period; 1-255. */
#define ATTR_IMMUNE  3   /**< health: health_immune, health_immune_base; DMG_BIT() mask. */
#define ATTR_RESIST  4   /**< health: health_resist, health_resist_base. */
#define ATTR_VULN    5   /**< health: health_vuln, health_vuln_base. */
#define ATTR_STR     6   /**< stats: stats_ability[ABIL_STR], stats_ability_base[ABIL_STR]; 1-30. */
#define ATTR_DEX     7
#define ATTR_CON     8
#define ATTR_INT     9
#define ATTR_WIS     10
#define ATTR_CHA     11  /**< ...stats_ability[ABIL_CHA]. */
#define ATTR_COUNT   12

/** The attribute of ability a (ABIL_*, game/components/stats.h): in the same order. */
#define ATTR_ABILITY(a)  ((uint8_t)(ATTR_STR + (a)))

#endif
