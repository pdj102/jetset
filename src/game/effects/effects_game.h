#ifndef EFFECTS_GAME_H
#define EFFECTS_GAME_H

/*
 * The game's side of the effects module (effects_core.h): its ops. Not
 * for use outside the module (except the tests).
 */

#include "game/effects/effects_core.h"

/* The game's ops, after the core's */
#define OP_DAMAGE  (OP_GAME + 0)   /* amount damage of type arg (DMG_*), through health_damage() */
#define OP_HEAL    (OP_GAME + 1)   /* heal amount, through health_heal() */
#define OP_MANA    (OP_GAME + 2)   /* restore amount MP, through mana_restore(): nothing without mana */
/* Pending: they do nothing until their mechanics are built (docs/items.md D17) */
#define OP_LIGHT     (OP_GAME + 3)
#define OP_IDENTIFY  (OP_GAME + 4)
#define OP_MAP       (OP_GAME + 5)
#define OP_TELEPORT  (OP_GAME + 6)
#define OP_UNCURSE   (OP_GAME + 7)
#define OP_ENCHANT   (OP_GAME + 8)
#define OP_SUMMON    (OP_GAME + 9)
#define OP_COUNT     (OP_GAME + 10)

#endif
