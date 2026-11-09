#ifndef ATTRS_INTERNAL_H
#define ATTRS_INTERNAL_H

/*
 * The attributes module's internals (attrs_bank.c, page 138): not for use
 * outside it (except the tests).
 */

#include "ecs/nge_entity.h"
#include "game/attrs/attrs.h"
#include "game/components/stats.h"

/*
 * The attributes' bases, in page 139 (attrs_pdata.c): readable only while
 * this module's pages are mapped, so other code calls effects_base() and
 * effects_put_base(). Meaningful only with the attribute's component.
 */
extern uint8_t health_hp_max_base[NGE_ENTITY_MAX];
extern uint8_t health_immune_base[NGE_ENTITY_MAX];
extern uint8_t health_resist_base[NGE_ENTITY_MAX];
extern uint8_t health_vuln_base[NGE_ENTITY_MAX];
extern uint8_t health_ac_base[NGE_ENTITY_MAX];
extern uint8_t stats_speed_base[NGE_ENTITY_MAX];
extern uint8_t stats_ability_base[ABIL_COUNT][NGE_ENTITY_MAX];
extern uint8_t mana_mp_max_base[NGE_ENTITY_MAX];

/* Where each attribute's base is, and how its modifiers combine */
typedef struct attrs_attr {
    uint8_t         *base;        /* the attribute's base table, [NGE_ENTITY_MAX], in page 139 */
    nge_component_t  component;   /* an entity has the attribute only with it */
    uint8_t          is_mask;     /* modifiers ORed (1) or added (0) */
} attrs_attr_t;

/* One row per attribute (ATTR_*), constant in page 138 */
extern const attrs_attr_t attrs_table[ATTR_COUNT];

#endif
