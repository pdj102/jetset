#ifndef ATTRS_INTERNAL_H
#define ATTRS_INTERNAL_H

/*
 * The attributes module's internals (attrs_bank.c, page 138): not for use
 * outside it (except the tests).
 */

#include "ecs/nge_entity.h"
#include "game/attrs/attrs.h"

/* Where each attribute's base is, and how its modifiers combine */
typedef struct attrs_attr {
    uint8_t         *base;        /* the component's base table, [NGE_ENTITY_MAX], in slot 0 */
    nge_component_t  component;   /* an entity has the attribute only with it */
    uint8_t          is_mask;     /* modifiers ORed (1) or added (0) */
} attrs_attr_t;

/* One row per attribute (ATTR_*), constant in page 138 */
extern const attrs_attr_t attrs_table[ATTR_COUNT];

#endif
