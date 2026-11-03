#ifndef SPELLS_INTERNAL_H
#define SPELLS_INTERNAL_H

/*
 * Shared by the spells module's files; not for use outside it:
 * - spells_bank.c (page 134): the functions;
 * - spells_pdata.c (page 135): the definitions, the costs and each
 *   caster's known spells.
 *
 * Page 135 is in slot 7 only while page 134's code runs: copy out of it
 * (spell_get(), spell_name()) before calling another page.
 */

#include "game/spells/spells.h"

/* Page 135 */
extern const spell_def_t spell_defs[SPELL_COUNT];
extern const uint8_t spell_costs[SPELL_LEVEL_MAX + 1];       /* MP by spell level */
extern uint8_t spell_known[NGE_ENTITY_MAX][SPELL_SET_BYTES];   /* 1 bit a spell: meaningful only with COMPONENT_SPELLS */

#endif
