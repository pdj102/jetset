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
#define IT_COUNT     7

/* Features (CLASS_FEATURE) */
#define FT_DOOR         0
#define FT_STAIRS_UP    1
#define FT_STAIRS_DOWN  2
#define FT_COUNT        3

/* Item subclasses, in each item's kind record */
#define ITEM_POTION  0
#define ITEM_WEAPON  1
#define ITEM_RING    2
#define ITEM_FOOD    3

#define KIND_INVALID 0xFF /* Invalid kind */

#endif
