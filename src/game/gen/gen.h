#ifndef GEN_H
#define GEN_H

/**
 * @file gen.h
 * @brief Entity generators: create an entity of a given kind, with all
 *        its components, from its kind data.
 *
 * Each class of entity (creatures, items, features) has a table of
 * kind records: constant data, one per kind (gen_kinds.h), defined as
 * data in the class's page data file (gen_<class>_pdata.c). A generator
 * creates an entity, gives it its owner and its kind (game/components/
 * kind.h), adds every component the kind needs from the record (its
 * values the same for every entity of the kind), and returns it:
 *
 *     id = gen_creature(CR_RAT, LEVEL_AREA);
 *     if (id != NGE_ENTITY_INVALID) {
 *         nge_location_set(id, x, y);      // placing it is the caller's job
 *     }
 *
 * The owner is required: an area, an entity (a chest, the player's pack) or
 * NGE_OWNER_WORLD. A generator never sets a location. The _random versions
 * choose a kind whose band of depths holds the depth, weighted by its rarity.
 *
 * Kind data that never changes and isn't needed every turn stays in the
 * table: read it through functions like gen_name(), which copies into the
 * caller's buffer (on the stack, in common memory).
 *
 * Banking: each class's generator is banked with its table (creatures: code
 * page 106, table page 107; items: 108/109; features: 122/123). Don't call them while game
 * code has its own page mapped in slot 6 or 7. gen_name() is in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "ecs/components/nge_block.h"
#include "ecs/nge_owner.h"
#include "game/components/attack.h"
#include "game/gen/gen_kinds.h"

/**
 * @brief Create a creature of kind (CR_*), owned by owner: sprite, block,
 *        stats, health, mana if its kind has some, its natural attacks
 *        (melee, ranged), timer (the player's or a monster's turn) and, if
 *        it can carry things, inventory; then its kit, made in its inventory
 *        and equipped (act_equip()), and its innate effect, if any
 *        (its record's innate hook). The player is persistent: at 0 hp it isn't
 *        destroyed (game over instead); and a new character, with the PC
 *        component (game/components/pc.h). Every other creature has the AI
 *        component, with its kind's brain and start state (game/ai/ai.h),
 *        and the AI's timer.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free.
 */
nge_entity_id_t gen_creature(uint8_t kind, nge_owner_t owner) NGE_BANKED;

/**
 * @brief Create a random creature that appears at depth, weighted by rarity.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free or no kind appears there.
 */
nge_entity_id_t gen_creature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED;

/*
 * A creature kind's families and traits, in its record's flags byte
 * (docs/creature_extensions.md M4.3, D8): read with gen_creature_flags()
 */
#define CREATURE_UNDEAD    0x02   /**< Undead: what a summon of the dead makes. */
#define CREATURE_BEAST     0x04   /**< An animal. */
#define CREATURE_GIANT     0x08   /**< A giant. */
#define CREATURE_DRAGON    0x10   /**< A dragon. */
#define CREATURE_MINDLESS  0x20   /**< No mind: sleep, confusion and fear don't take (effect_apply()). */

/**
 * @brief Create a random creature with every flag in flags (CREATURE_*),
 *        of a kind that first appears at depth or above, and knows no spell
 *        (so a summoner's summons don't summon), weighted by rarity
 *        (docs/creature_extensions.md M2.1: summon undead).
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free or no kind fits.
 */
nge_entity_id_t gen_creature_random_flags(uint8_t depth, nge_owner_t owner, uint8_t flags) NGE_BANKED;

/** @brief A creature kind's (CR_*) flags (CREATURE_*). Banked with the creature table (page 106). */
uint8_t gen_creature_flags(uint8_t kind) NGE_BANKED;

/**
 * @brief Create an item of kind (IT_*), owned by owner: its sprite, the
 *        item component (not equipped, with its kind's count) and, if its
 *        kind has an attack, a melee attack, or a ranged one at range 2 or more.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free.
 */
nge_entity_id_t gen_item(uint8_t kind, nge_owner_t owner) NGE_BANKED;

/** @brief Create a random item that appears at depth, weighted by rarity (see gen_creature_random()). */
nge_entity_id_t gen_item_random(uint8_t depth, nge_owner_t owner) NGE_BANKED;

/**
 * @brief Create a feature of kind (FT_*), owned by owner (an area): the
 *        feature component in its kind's starting state. Not placed: the
 *        caller places it with feature_place() (game/components/feature.h),
 *        which draws it.
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free.
 */
nge_entity_id_t gen_feature(uint8_t kind, nge_owner_t owner) NGE_BANKED;

/**
 * @brief Create a random feature that appears at depth, weighted by rarity
 *        (see gen_creature_random()): not placed, and a container's contents
 *        not made (the caller does both).
 * @return The entity, or NGE_ENTITY_INVALID if no entity is free or no kind appears there.
 */
nge_entity_id_t gen_feature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED;

/** @brief A feature kind's (FT_*) DC, to find it when hidden and to disarm it. Banked with the feature table (page 122). */
uint8_t gen_feature_dc(uint8_t kind) NGE_BANKED;

/** @brief A container kind's (FT_*) most random items: 0 if it isn't one. Banked with the feature table (page 122). */
uint8_t gen_feature_holds(uint8_t kind) NGE_BANKED;

/** @brief A feature kind's (FT_*) verb (FVERB_*, game/components/feature.h). Banked with the feature table (page 122). */
uint8_t gen_feature_verb(uint8_t kind) NGE_BANKED;

/** A feature's look: a map tile and its metatile's palette bank (TILE_BANK(), data/tile_data.h). */
typedef struct gen_feature_look {
    uint8_t tile_id;
    uint8_t bank;
} gen_feature_look_t;

/** What a feature kind can do and how it looks: the part of its kind record the feature component uses. */
typedef struct gen_feature_info {
    gen_feature_look_t look[2];   /**< Closed ([0]) and open ([1]). */
    uint8_t            can;       /**< FCAN_* (game/components/feature.h). */
    nge_block_t        block_shut;   /**< What it blocks while closed. */
} gen_feature_info_t;

/**
 * @brief Copy a feature kind's (FT_*) info into *info (in common memory:
 *        on the caller's stack). Banked with the feature table (page 122).
 */
void gen_feature_info(uint8_t kind, gen_feature_info_t *info) NGE_BANKED;

/**
 * @brief The equipment slots an item kind (IT_*) can go in: a mask of
 *        SLOT_BIT() (game/components/item.h), 0 if it can't be equipped.
 *        Banked with the item table (page 108).
 */
uint16_t gen_item_fits(uint8_t kind) NGE_BANKED;

/**
 * A sequence of sprite patterns that an action plays once over the rest
 * animation (nge_render_play()): count 0 for none.
 */
typedef struct gen_seq {
    uint8_t first;   /**< First pattern. */
    uint8_t count;   /**< Patterns; 0: none. */
    uint8_t rate;    /**< Frames per pattern. */
} gen_seq_t;

/* The actions a creature kind has a sequence for: the index into its sequences */
#define GEN_SEQ_MOVE    0   /**< Moving a tile, with the slide. */
#define GEN_SEQ_ATTACK  1   /**< Attacking in melee, with the lunge. */
#define GEN_SEQ_COUNT   2

/**
 * @brief Copy a creature kind's (CR_*) sequences, by GEN_SEQ_*, into
 *        seq[GEN_SEQ_COUNT] (in common memory: on the caller's stack).
 *        Banked with the creature table (page 106).
 */
void gen_creature_seqs(uint8_t kind, gen_seq_t *seq) NGE_BANKED;

/**
 * @brief The XP the player gains for killing a creature of kind (CR_*),
 *        classic B/X values by hit dice (game/components/pc.h). Banked
 *        with the creature table (page 106).
 */
uint16_t gen_creature_xp(uint8_t kind) NGE_BANKED;

/*
 * When an item's one hook fires: the effect (EF_*, game/effects/effects.h)
 * and power its kind record names are tagged with one. Creatures and
 * features name each hook beside its part instead (docs/creature_extensions.md D3)
 */
#define GEN_HOOK_USE     0   /**< Applied to the user when used up (act_use()). */
#define GEN_HOOK_HIT     1   /**< Weapons and ammo: applied to what a hit leaves alive (act_strike()). */
#define GEN_HOOK_WORN    2   /**< Its modifiers and statuses count while equipped. */
#define GEN_HOOK_AIM     3   /**< Sent at a chosen target (act_aim(): wands, staves, parchments). */
#define GEN_HOOK_COUNT   4

/* A hook's packed answer (gen_hit_of() and the accessors): the effect in the low byte, its power in the high byte */
#define GEN_HOOK_PACK(ef, power)  ((uint16_t)(((uint16_t)(uint8_t)(power) << 8) | (uint8_t)(ef)))
#define GEN_HOOK_EF(h)            ((uint8_t)(h))
#define GEN_HOOK_POWER(h)         ((int8_t)(uint8_t)((h) >> 8))

/**
 * @brief The effect and power of a hit by an entity's attack of use
 *        (ATTACK_*), packed: read them with GEN_HOOK_EF() and
 *        GEN_HOOK_POWER(). A creature's hook for that attack (its melee_hit
 *        for ATTACK_MELEE), an item's GEN_HOOK_HIT hook whatever the use (a
 *        weapon, an arrow), a trap's hit. EF_NONE (0) if it names none, or
 *        the entity has no kind. In common memory: it calls its class's
 *        banked accessor (pages 106, 108, 122). The one reader of hooks that
 *        crosses classes, as what hits can be any of them (R17).
 */
uint16_t gen_hit_of(nge_entity_id_t id, uint8_t use);

/**
 * @brief An item kind's (IT_*) effect and power for hook (GEN_HOOK_*),
 *        packed as gen_hit_of()'s: EF_NONE if its one hook fires on
 *        another. Banked with the item table (page 108).
 */
uint16_t gen_item_hook(uint8_t kind, uint8_t hook) NGE_BANKED;

/**
 * @brief A feature kind's (FT_*) effect and power when operated, packed as
 *        gen_hit_of()'s: EF_NONE for none. Banked with the feature table (page 122).
 */
uint16_t gen_feature_use(uint8_t kind) NGE_BANKED;

/** @brief An item kind's (IT_*) subclass (ITEM_POTION...). Banked with the item table (page 108). */
uint8_t gen_item_subclass(uint8_t kind) NGE_BANKED;

/*
 * The readers of an item kind's subclass-dependent data (docs/items.md 4.7),
 * banked with the item table (page 108). Each checks the subclass.
 */

/** @brief A launcher's or ammo's GROUP_*: they must share one. GROUP_NONE for another subclass. */
uint8_t gen_item_ammo_group(uint8_t kind) NGE_BANKED;

/** @brief The KEY_* of the locks a key opens (KEY_ANY: all). KEY_NONE for another subclass. */
uint8_t gen_item_key_lock_group(uint8_t kind) NGE_BANKED;

/** @brief A lock pick's (ITEM_TOOL) bonus to its roll. 0 for another subclass. */
uint8_t gen_item_pick_bonus(uint8_t kind) NGE_BANKED;

/** @brief A book's SPELL_BOOK_*: its list of spells. Asserts the kind is a book (0 is a list). */
uint8_t gen_item_book_spells(uint8_t kind) NGE_BANKED;

/** @brief Armour's AC (docs/items.md 4.11). 0 for another subclass. */
uint8_t gen_item_armour_ac(uint8_t kind) NGE_BANKED;

/** @brief An item kind's ITEMF_* flags. Banked with the item table (page 108). */
uint8_t gen_item_flags(uint8_t kind) NGE_BANKED;

/**
 * @brief An aimed item kind's shape (SPELL_*), from its ITEMF_SHAPE field:
 *        SPELL_BOLT if the field is empty, as for a kind with no aim hook.
 *        Asserts an invalid value. Banked with the item table (page 108).
 */
uint8_t gen_item_shape(uint8_t kind) NGE_BANKED;

/**
 * @brief Copy an item kind's attack part into *attack (in common memory: on
 *        the caller's stack): dice 0 for none, as ammo that only adds its
 *        bonuses has. Banked with the item table (page 108).
 */
void gen_item_attack(uint8_t kind, attack_kind_t *attack) NGE_BANKED;

/**
 * @brief Copy the name of an entity's kind into buf: at most size-1
 *        characters and a NUL ("something" for an entity with no kind).
 * @param size 1 or more.
 */
void gen_name(nge_entity_id_t id, char *buf, uint8_t size);

#endif
