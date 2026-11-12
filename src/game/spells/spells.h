#ifndef SPELLS_H
#define SPELLS_H

/**
 * @file spells.h
 * @brief Spells: the table of spells, and the spells each caster knows.
 *        See docs/spells.md.
 *
 * A spell is a row of spell_defs[] (SP_*): its level, which sets its cost
 * (spell_cost()), its shape, which says who it reaches, and the one effect
 * it applies at its power (game/effects/effects.h). Adding a spell means
 * adding a row; only a new shape needs code. ACT_CAST casts one
 * (game/actions/act.h), for the player and monsters alike.
 *
 * Known spells: the component COMPONENT_SPELLS, a set of 1 bit a spell for
 * each caster, in the module's page data (page 135). A kind's record lists
 * the spells its creatures start with (spell_add()); spell_learn() is the
 * only way to add one later, and posts EV_SPELL_LEARNED.
 *
 * Slots and books (docs/spells.md section 10): the player has spell slots
 * at each spell level, by class and character level (spell_slots()); each
 * known spell fills one of its level. The player learns only from a spell
 * book, an item whose kind's data is a SPELL_BOOK_* list (spell_book()),
 * into a free slot (spell_free()): ACT_LEARN, which ends in spell_learn().
 * Monsters have no slots and know their kind's list.
 *
 * Banking: banked (NGE_BANKED, page 134, with the data in page 135): don't
 * call these while game code has its own page mapped in slot 6 or 7.
 * SPELLS_IS() is in common reach.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "game/components/components.h"

/* Spells: rows of spell_defs[]. SP_NONE (0) is an empty entry in a list */
#define SP_NONE         0
#define SP_MAGIC_BOLT   1    /**< Level 1 bolt: magic damage. */
#define SP_CURE_WOUNDS  2    /**< Level 1, self: heals. */
#define SP_HOLD         3    /**< Level 2, direct: rooted. */
#define SP_ENFEEBLE     4    /**< Level 2, direct: weaker. */
#define SP_FIRE_BOLT    5    /**< Level 3 bolt: fire damage. */
#define SP_FROST_BOLT   6    /**< Level 3 bolt: cold damage. */
#define SP_STONE_SKIN   7    /**< Level 4, self: AC up, slower. */
#define SP_HASTE        8    /**< Level 4, self: faster. */
#define SP_POWER_BOLT   9    /**< Level 5 bolt: more magic damage. */
#define SP_RENEWAL      10   /**< Level 5, self: heals over time. */
#define SP_BLINK        11   /**< Level 2, tile: the caster to a free tile in sight. */
#define SPELL_COUNT     12

/* Shapes: who a spell or an aimed item reaches (an item's: gen_item_shape()) */
#define SPELL_SELF    0   /**< The caster. */
#define SPELL_BOLT    1   /**< The first creature on the line to a tile (ACT_LINE_BOLT). */
#define SPELL_DIRECT  2   /**< The creature on the chosen tile, past any on the way (ACT_LINE_DIRECT). */
#define SPELL_TILE    3   /**< A free tile in range and line, never a creature: its effect on the caster, there (effect_apply_at()). */
#define SPELL_BALL    4   /**< Every creature within ACT_BALL_RADIUS of the chosen tile, the caster too (ACT_LINE_DIRECT). */
#define SPELL_BURST   5   /**< Every creature within ACT_BURST_RADIUS of the caster, not the caster. */
#define SPELL_SHAPE_COUNT  6

/* When the AI casts a spell */
#define SPELL_AI_NEVER   0   /**< Never: the AI can't use it well. */
#define SPELL_AI_HURT    1   /**< When hurt: on itself. */
#define SPELL_AI_TARGET  2   /**< At the creature it hunts. */
#define SPELL_AI_COUNT   3
#define SPELL_AI_ANY     0xFF   /**< For spell_next(): any of them. */

/* Spell levels: 0 is an ability, which costs nothing */
#define SPELL_LEVEL_MAX  5

/* Spell books (docs/spells.md 10.5.3): an ITEM_BOOK kind's data.book_spells, a row of spell_books[] */
#define SPELL_BOOK_BASIC   0   /**< Spells of levels 1 and 2. */
#define SPELL_BOOK_ADEPT   1   /**< Levels 3 and 4. */
#define SPELL_BOOK_MASTER  2   /**< Level 5. */
#define SPELL_BOOK_COUNT   3
#define SPELL_BOOK_MAX     8   /**< Spells in a book, at most. */

#define SPELL_NAME_MAX   14   /**< Characters in a name, at most. */
#define SPELL_SET_BYTES  4    /**< Bytes of a known set: 1 bit a spell. */

/* Every spell has a bit in a known set */
typedef char spells_count_check[(SPELL_COUNT <= SPELL_SET_BYTES * 8) ? 1 : -1];

/** A spell's definition: a row of spell_defs[]. */
typedef struct spell_def {
    const char *name;    /**< In page 135: copy it with spell_name(). */
    uint8_t     level;   /**< 0 to SPELL_LEVEL_MAX: sets the cost. */
    uint8_t     shape;   /**< SPELL_*. */
    uint8_t     ef;      /**< EF_*: what it does; never EF_NONE or worn. */
    int8_t      power;   /**< The effect's power: above 0. */
    uint8_t     range;   /**< A bolt's or direct spell's reach (nge_map_within()); 0 for SPELL_SELF. */
    uint8_t     ai;      /**< SPELL_AI_*. */
} spell_def_t;

/** Entity id has the spells component: it knows some spells (a caster). */
#define SPELLS_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_SPELLS)

/**
 * @brief Give an entity in use the spells component, knowing the spells
 *        of list (count entries, SP_NONE for none) and no others. Posts
 *        nothing: these are what it starts with.
 * @param list In common memory (a copy on the stack): the call maps page 134.
 */
void spell_add(nge_entity_id_t id, const uint8_t *list, uint8_t count) NGE_BANKED;

/** @brief 1 if id is a caster and knows spell sp. */
uint8_t spell_knows(nge_entity_id_t id, uint8_t sp) NGE_BANKED;

/**
 * @brief Teach caster id spell sp: posts EV_SPELL_LEARNED (p1: sp), unless
 *        it is known already, when nothing changes. The only way to add a
 *        spell, whatever the cause (docs/spells.md, R8).
 */
void spell_learn(nge_entity_id_t id, uint8_t sp) NGE_BANKED;

/**
 * @brief The spell slots id has at spell level level (1 to SPELL_LEVEL_MAX):
 *        the slot table's for the player's class and character level if id
 *        is the player with mana; 0 for anything else.
 */
uint8_t spell_slots(nge_entity_id_t id, uint8_t level) NGE_BANKED;

/**
 * @brief The free slots id has at spell level level: spell_slots() less the
 *        spells of that level id knows, 0 if none (never below 0).
 */
uint8_t spell_free(nge_entity_id_t id, uint8_t level) NGE_BANKED;

/**
 * @brief 1 if character level pc_level (PC_LEVEL_MIN to PC_LEVEL_MAX) gives
 *        class cls a slot, at any spell level, that pc_level - 1 didn't; 0
 *        for level 1. Reads only the table, so a message handler that runs
 *        after more levels is still right for this one (docs/spells.md, D26).
 */
uint8_t spell_slots_new(uint8_t cls, uint8_t pc_level) NGE_BANKED;

/**
 * @brief Copy book book's spells (SPELL_BOOK_*) into list, in order, and
 *        return how many there are.
 * @param list In common memory (the caller's stack), SPELL_BOOK_MAX bytes:
 *        entries past the count are SP_NONE.
 */
uint8_t spell_book(uint8_t book, uint8_t *list) NGE_BANKED;

/** @brief 1 if spell sp is in book book (SPELL_BOOK_*); 0 for SP_NONE. */
uint8_t spell_in_book(uint8_t book, uint8_t sp) NGE_BANKED;

/**
 * @brief The next spell after sp that caster id knows, wrapping round to
 *        sp itself, used by the AI as ai says (SPELL_AI_*, or SPELL_AI_ANY);
 *        SP_NONE if none. sp may be SP_NONE, to start at the first.
 */
uint8_t spell_next(nge_entity_id_t id, uint8_t sp, uint8_t ai) NGE_BANKED;

/** @brief Copy spell sp's row into *def (in common memory: the caller's stack). */
void spell_get(uint8_t sp, spell_def_t *def) NGE_BANKED;

/** @brief The MP a spell of level level costs. */
uint8_t spell_cost(uint8_t level) NGE_BANKED;

/** @brief Copy spell sp's name into buf, cut to size - 1 characters. */
void spell_name(uint8_t sp, char *buf, uint8_t size) NGE_BANKED;

#endif
