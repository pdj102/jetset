#ifndef KIND_H
#define KIND_H

/**
 * @file kind.h
 * @brief The kind component: which kind an entity was generated
 *        from (game/gen/gen.h), as a class and an index into its table.
 *
 * Set by the generators; read to find an entity's cold kind data (its name,
 * through gen_name()) and to tell creatures from items. The tables are in
 * slot 0 (the ECS data page, kind_cdata.c): read directly by common code.
 *
 * Banking: kind_add() is banked (NGE_BANKED, page 110, with the shared
 * generator helpers): it runs only when an entity is created.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"
#include "game/gen/gen_kinds.h"

/* Classes of entity: one generator and one table each */
#define CLASS_NONE      0
#define CLASS_CREATURE  1   /**< Monsters and the player (gen_creature()). */
#define CLASS_ITEM      2   /**< Weapons, armour, potions... (gen_item()). */
#define CLASS_FEATURE   3   /**< Doors, stairs; later chests, fountains, traps (gen_feature()). */

/* Tables in slot 0 (kind_cdata.c): meaningful only with COMPONENT_KIND */
extern uint8_t kind_class[NGE_ENTITY_MAX];   /**< CLASS_*. */
extern uint8_t kind_index[NGE_ENTITY_MAX];   /**< Index into the class's table (game/gen/gen_kinds.h). */

/** The entity has the kind component and is of class c. */
#define KIND_IS(id, c) \
    (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_KIND) && kind_class[id] == (c))

/** @brief Give an entity in use the kind component: class c, index i in its table. */
void kind_add(nge_entity_id_t id, uint8_t c, uint8_t i) NGE_BANKED;

#endif
