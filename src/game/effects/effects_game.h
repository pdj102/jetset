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
#define OP_COUNT   (OP_GAME + 2)

#endif
