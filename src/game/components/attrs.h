#ifndef ATTRS_H
#define ATTRS_H

/**
 * @file attrs.h
 * @brief Attributes: the numbers and masks equipment and effects change;
 *        and statuses, the flags effects grant.
 *
 * An attribute is a number (or a mask) that equipment and effects can
 * change: max hit points, AC, speed, the abilities, the damage masks.
 * Each has, per entity:
 *
 *   kind value  constant, in its kind record (health_kind_t.hp_max):
 *               sets the base when the entity is created
 *   base        changed only for good, by effects_set_base() (a potion
 *               of vitality): attr_base[ATTR_HP_MAX]
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
 * A status is a flag an effect grants while it is active (poisoned): the
 * entity's status mask is the OR of the statuses of its effects, so it
 * stays set while any effect grants it, and a cure ends those effects
 * instead of clearing a bit. Rules ask STATUS_HAS().
 *
 * The values are the components' tables (health.h, stats.h). The bases
 * and the status mask are here, in slot 0 (attrs_cdata.c), meaningful for
 * an entity with the component each attribute belongs to; the status mask
 * for an entity with health. There are no functions here: the components
 * set the bases, the effects module the rest.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/* Attributes: numbers add up; masks (immune, resist, vuln) are ORed (game/effects/effects_pdata.c) */
#define ATTR_HP_MAX  0   /**< health_hp_max: 1-255. */
#define ATTR_AC      1   /**< health_ac: 0-255. */
#define ATTR_SPEED   2   /**< stats_speed, and the timer's period: 1-255. */
#define ATTR_IMMUNE  3   /**< health_immune: DMG_BIT() mask. */
#define ATTR_RESIST  4   /**< health_resist. */
#define ATTR_VULN    5   /**< health_vuln. */
#define ATTR_STR     6   /**< stats_ability[ABIL_STR]: 1-30 (game/components/stats.h). */
#define ATTR_DEX     7
#define ATTR_CON     8
#define ATTR_INT     9
#define ATTR_WIS     10
#define ATTR_CHA     11  /**< ...stats_ability[ABIL_CHA]. */
#define ATTR_COUNT   12

/** The attribute of ability a (ABIL_*, game/components/stats.h): in the same order. */
#define ATTR_ABILITY(a)  ((uint8_t)(ATTR_STR + (a)))

/* Statuses: bits of status_mask */
#define STATUS_POISONED  0x01

/* Tables in slot 0 (attrs_cdata.c) */
extern uint8_t attr_base[ATTR_COUNT][NGE_ENTITY_MAX];   /**< Base values: read-only outside the components and effects. */
extern uint8_t status_mask[NGE_ENTITY_MAX];             /**< STATUS_* granted by its effects: read-only for game code. */

/** Entity id (with health) has status s. */
#define STATUS_HAS(id, s)  ((status_mask[id] & (s)) != 0)

#endif
