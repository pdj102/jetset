#ifndef EFFECTS_INTERNAL_H
#define EFFECTS_INTERNAL_H

/*
 * Shared by the effects module's files; not for use outside it (except the
 * tests):
 * - effects_bank.c (page 120): applying, firing, curing and ending
 *   effects, the ops, the saving throws, the texts;
 * - effects_pdata.c (page 121): the definitions (effects_defs.h), and the
 *   ops' tables.
 */

#include "game/effects/effects_defs.h"

/* The 8 tiles around one, in order round it: (dx, dy) pairs, for a summon (page 121) */
#define EFFECTS_AROUND  8
extern const int8_t effects_around[EFFECTS_AROUND][2];

/* A saving throw: 1d20 + target's modifier in ability arg (0 without stats) >= dc */
uint8_t effects_save(nge_entity_id_t target, uint8_t arg, uint8_t dc);

/*
 * For the PC tests: 1 skips the saving throw when an effect is applied
 * (docs/items_extensions.md R14), so a test of what an effect does needn't
 * roll for it to land. 0, uninitialised data, in the game
 */
extern uint8_t effects_apply_no_save;

#endif
