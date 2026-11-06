#ifndef EFFECTS_INTERNAL_H
#define EFFECTS_INTERNAL_H

/*
 * Shared by the effects module's files; not for use outside it (except the
 * tests):
 * - effects_bank.c (page 120): applying, firing, curing and ending
 *   effects, the ops, the saving throws, the texts;
 * - effects_pdata.c (page 121): the definitions (effects_defs.h).
 */

#include "game/effects/effects_defs.h"

/* A saving throw: 1d20 + target's modifier in ability arg (0 without stats) >= dc */
uint8_t effects_save(nge_entity_id_t target, uint8_t arg, uint8_t dc);

#endif
