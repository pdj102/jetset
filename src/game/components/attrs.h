#ifndef ATTRS_H
#define ATTRS_H

/**
 * @file attrs.h
 * @brief Attributes and statuses: the base values that effects modify,
 *        and the statuses effects grant.
 *
 * An attribute has a base value per entity, set from its kind's base
 * record when the entity is generated (health_add(), stats_add()) and
 * changed for good only by effects_set_base() (game/effects/effects.h),
 * and a derived value: the base plus the modifiers of the items the entity
 * has equipped and of its active effects. The derived values are the
 * component tables the rules read (health_hp_max, health_ac, the damage
 * masks, stats_speed); only effects_recompute() writes them. Hit points
 * are not an attribute: a resource that damage and healing change, kept
 * within the derived maximum.
 *
 * A status is a flag an effect grants while it is active (poisoned): the
 * entity's status mask is the OR of the statuses of its effects, so it
 * stays set while any effect grants it, and a cure ends those effects
 * instead of clearing a bit. Rules ask STATUS_HAS().
 *
 * The tables are in slot 0 (attrs_cdata.c), meaningful for an entity with
 * the components they belong to (health, stats); the status mask for an
 * entity with health. There are no functions here: the components set the
 * bases, the effects module the rest.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/* Attributes: numbers add up; masks (ATTR_IMMUNE and on) are ORed (game/effects/effects_game.c) */
#define ATTR_HP_MAX  0   /**< health_hp_max: 1-255. */
#define ATTR_AC      1   /**< health_ac: 0-255. */
#define ATTR_SPEED   2   /**< stats_speed, and the timer's period: 1-255. */
#define ATTR_IMMUNE  3   /**< health_immune: DMG_BIT() mask. */
#define ATTR_RESIST  4   /**< health_resist. */
#define ATTR_VULN    5   /**< health_vuln. */
#define ATTR_COUNT   6

/* Statuses: bits of status_mask */
#define STATUS_POISONED  0x01

/* Tables in slot 0 (attrs_cdata.c) */
extern uint8_t attr_base[ATTR_COUNT][NGE_ENTITY_MAX];   /**< Base values: read-only outside the components and effects. */
extern uint8_t status_mask[NGE_ENTITY_MAX];             /**< STATUS_* granted by its effects: read-only for game code. */

/** Entity id (with health) has status s. */
#define STATUS_HAS(id, s)  ((status_mask[id] & (s)) != 0)

#endif
