#ifndef MANA_H
#define MANA_H

/**
 * @file mana.h
 * @brief The mana component: an entity with mana points (MP), a resource
 *        spent to cast spells (the player). See docs/mana.md.
 *
 * MP are a pool: each has current MP (mp, 0 to mp_max) and a maximum
 * (mp_max), set from the kind value in its kind record (mana_kind_t,
 * game/gen). The maximum doesn't change yet: it isn't an attribute
 * (game/components/attrs.h) until an item or effect needs to change it
 * (docs/mana.md, D2). An entity starts full.
 *
 * A spell's caller checks there are enough (MANA_HAS()) before spending
 * them: mana_spend() asserts it, and stops at 0 when asserts are compiled
 * out. mana_restore() gives MP back up to the maximum. Each posts an event
 * (EV_MANA_SPENT, EV_MANA_RESTORED, game/events.h) when MP changed, so the
 * stats window, and later the messages, react.
 *
 * The tables are in slot 0 (mana_cdata.c). Banking: the functions are
 * banked (NGE_BANKED, page 110, with the other component functions).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/** Kind mana: the mana part of a kind record. */
typedef struct mana_kind {
    uint8_t mp_max;   /**< Maximum MP, the same for every entity of the kind; 0 for none. */
} mana_kind_t;

/** A kind record's mana part gives the component: it has some MP. */
#define MANA_KIND_HAS(k)  ((k)->mp_max != 0)

/* Tables in slot 0 (mana_cdata.c): read-only for game code; meaningful only with COMPONENT_MANA */
extern uint8_t mana_mp[NGE_ENTITY_MAX];       /**< Current MP, 0 to mana_mp_max. */
extern uint8_t mana_mp_max[NGE_ENTITY_MAX];   /**< The most MP it can have, 1 or more. */

/** Entity id has the mana component. */
#define MANA_IS(id)      NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_MANA)

/** Entity id (with mana) has at least n MP. */
#define MANA_HAS(id, n)  (mana_mp[id] >= (n))

/**
 * @brief Give an entity in use the mana component from kind mana: full,
 *        with the kind's maximum MP (1 or more).
 * @param k In common memory (a copy on the stack): the call maps page 110.
 */
void mana_add(nge_entity_id_t id, const mana_kind_t *k) NGE_BANKED;

/**
 * @brief Spend amount MP of an entity with mana, which has them (the
 *        caller checks MANA_HAS(): asserted; without asserts it stops at
 *        0). Posts EV_MANA_SPENT (p1: amount) unless amount is 0.
 */
void mana_spend(nge_entity_id_t id, uint8_t amount) NGE_BANKED;

/**
 * @brief Restore MP to an entity with mana, up to mp_max: posts
 *        EV_MANA_RESTORED (p1: the MP restored) if any were.
 * @return The MP restored.
 */
uint8_t mana_restore(nge_entity_id_t id, uint8_t amount) NGE_BANKED;

#endif
