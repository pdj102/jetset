#ifndef GEN_KINDS_H
#define GEN_KINDS_H

/**
 * @file gen_kinds.h
 * @brief The entities' kinds: an index into each class's table
 *        (gen_creature_pdata.c, gen_item_pdata.c).
 *
 * The tables use these as designated initialisers ([CR_RAT] = {...}), so
 * the order here is the only order. A future editing tool can generate this
 * file with the tables.
 */

/* Creatures (CLASS_CREATURE) */
#define CR_PLAYER  0
#define CR_RAT     1
#define CR_GOBLIN  2
#define CR_COUNT   3

/* Items (CLASS_ITEM) */
#define IT_POTION    0   /* healing */
#define IT_SWORD     1
#define IT_RING      2   /* of health */
#define IT_ANTIDOTE  3
#define IT_FOOD      4   /* a dead rat: poisoned */
#define IT_ELIXIR    5   /* of vitality */
#define IT_STRENGTH  6   /* potion of strength */
#define IT_QUEST     7   /* the quest item: one a game, made by game/actors.c, never at random */
#define IT_COUNT     8

/* Features (CLASS_FEATURE) */
#define FT_DOOR         0
#define FT_STAIRS_UP    1
#define FT_STAIRS_DOWN  2
#define FT_COUNT        3

/* Item subclasses, in each item's kind record */
#define ITEM_POTION     0
#define ITEM_WEAPON     1    /* melee */
#define ITEM_RING       2
#define ITEM_FOOD       3
#define ITEM_QUEST      4    /* the quest item (docs/winning.md) */
#define ITEM_RANGED     5    /* a launcher */
#define ITEM_AMMO       6    /* what a launcher shoots, or what is thrown */
#define ITEM_ARMOUR     7    /* shields too */
#define ITEM_DRINK      8
#define ITEM_PARCHMENT  9
#define ITEM_WAND       10   /* staves too */
#define ITEM_AMULET     11
#define ITEM_LIGHT      12
#define ITEM_KEY        13
#define ITEM_TOOL       14
#define ITEM_TREASURE   15
#define ITEM_COUNT      16

/* Item flags, in each item's kind record: stored now, their rules later (docs/items.md R25) */
#define ITEMF_TWO_HANDS  0x01   /* fills SLOT_SHIELD too */
#define ITEMF_CURSED     0x02   /* can't be taken off */

/* An item kind's group: what ammo works with what launcher (GROUP_*) */
#define GROUP_NONE    0
#define GROUP_ARROW   1
#define GROUP_BOLT    2
#define GROUP_STONE   3
#define GROUP_THROWN  4   /* needs no launcher */
#define GROUP_COUNT   5

/* ...or which locks a key opens (KEY_*); a lock pick's group is its bonus */
#define KEY_IRON    1
#define KEY_BRASS   2
#define KEY_SILVER  3
#define KEY_GOLD    4
#define KEY_ANY     5
#define KEY_COUNT   6

#define KIND_INVALID 0xFF /* Invalid kind */

#endif
