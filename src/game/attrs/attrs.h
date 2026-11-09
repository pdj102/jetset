#ifndef ATTRS_H
#define ATTRS_H

/**
 * @file attrs.h
 * @brief Attributes: the numbers and masks that equipment and effects
 *        change, numbered so effect definitions can name them, and the
 *        attributes module, which derives their values.
 *
 * An attribute is a number (or a mask) that equipment and effects can
 * change: max hit points, AC, speed, the abilities, the damage masks.
 * Each has, per entity:
 *
 *   kind value  constant, in its kind record (health_kind_t.hp_max):
 *               sets the base when the entity is created
 *   base        changed only for good, by effects_set_base() (a potion
 *               of vitality): health_hp_max_base, in this module's
 *               page 139; other code reads it with effects_base()
 *   value       base + the modifiers of equipped items' worn effects and
 *               of active effects, clamped: health_hp_max. The one rules
 *               read; only effects_recompute() writes it, and it is recomputed whenever a source changes
 *               (equip, an effect starts or ends, a base changes)
 *
 * A pool (hit points, MP) also has a current amount (health_hp):
 * not an attribute, but a resource that damage and healing change, kept
 * within the value. A lower maximum takes it down; a higher one doesn't
 * heal.
 *
 * The value tables live in slot 0 with the component each attribute
 * belongs to, meaningful only for an entity with that component; this
 * file numbers them (ATTR_*). The bases are read every turn only here, so
 * they are this module's page data (attrs_pdata.c, page 139;
 * docs/slot0_tables.md). The components set the values from the kind
 * values and the bases through effects_put_base() (health_add(),
 * stats_add()); the attributes module does the rest (attrs_bank.c),
 * finding each attribute's base table through its own table
 * (attrs_internal.h). It reads each source's statuses and modifiers
 * from the effect definitions (game/effects/effects_defs.h), mapping their
 * page in by hand: an equipped item's worn hook (game/gen/gen.h) and each
 * active effect (ecs/systems/nge_effect.h).
 *
 * Its functions keep the effects_ prefix they had when derivation was part
 * of the effects module, so their callers didn't change.
 *
 * Banking: the functions are banked (NGE_BANKED, page 138, with the bases
 * in page 139; page 121 is mapped into slot 7 over them while they copy a
 * definition, and back). Don't call them while game code has its own page
 * mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

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
#define ATTR_MP_MAX  12  /**< mana: mana_mp_max, mana_mp_max_base; 1-255. */
#define ATTR_COUNT   13

/** The attribute of ability a (ABIL_*, game/components/stats.h): in the same order. */
#define ATTR_ABILITY(a)  ((uint8_t)(ATTR_STR + (a)))

/**
 * @brief Work out an entity's derived attributes and statuses from its
 *        bases, its equipped items' worn effects and its active effects,
 *        clamped to their ranges; keep hp within the maximum and an
 *        actor's timer period equal to its speed. Posts EV_ATTRS_CHANGED
 *        if anything changed. Does nothing to an entity that isn't alive
 *        or has no health.
 */
void effects_recompute(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief 1 if an entity's derived attributes and statuses are what
 *        effects_recompute() would make them (for asserts and tests).
 */
uint8_t effects_check(nge_entity_id_t id) NGE_BANKED;

/** @brief Set attribute attr's base for good, then recompute (an alive entity with health and the attribute's component). */
void effects_set_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED;

/** @brief Add amount (maybe negative) to attribute attr's base, kept within 0-255, then recompute (as effects_set_base()). */
void effects_raise_base(nge_entity_id_t id, uint8_t attr, int8_t amount) NGE_BANKED;

/** @brief Attribute attr's base for id, which has the attribute's component (asserted). */
uint8_t effects_base(nge_entity_id_t id, uint8_t attr) NGE_BANKED;

/**
 * @brief Store attribute attr's base for id without recomputing: for the
 *        components' add functions, which set the values themselves. id
 *        is in use and has the attribute's component (asserted).
 */
void effects_put_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED;

#endif
