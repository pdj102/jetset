#ifndef GEN_KINDS_H
#define GEN_KINDS_H

/**
 * @file gen_kinds.h
 * @brief The base entities' kinds: an index into each class's table
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
#define IT_POTION  0
#define IT_SWORD   1
#define IT_COUNT   2

/* Item subclasses, in each item's base record */
#define ITEM_POTION  0
#define ITEM_WEAPON  1

#endif
