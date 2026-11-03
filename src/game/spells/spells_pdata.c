/* nge-dataseg: PAGE_135 */
/*
 * The spells' page data, in page 135 (slot 7 while the spells' code in
 * page 134 runs): the definitions, constant data with their names, the
 * costs, and each caster's known spells (uninitialised: set by
 * spell_add()). See "Page data" in docs/memory_map.md.
 */
#ifdef __SDCC
#pragma constseg PAGE_135
#endif

#include "game/spells/spells_internal.h"

#include "game/effects/effects.h"

/* docs/spells.md, section 4.12: one spell for each character level. Powers follow the items' */
const spell_def_t spell_defs[SPELL_COUNT] = {
    [SP_NONE]        = { 0 },
    [SP_MAGIC_BOLT]  = { "Magic Bolt",  1, SPELL_BOLT,   EF_BOLT_MAGIC, 3,  8, 1,  SPELL_AI_TARGET },
    [SP_CURE_WOUNDS] = { "Cure Wounds", 1, SPELL_SELF,   EF_HEALING,    4,  0, 2,  SPELL_AI_HURT },
    [SP_HOLD]        = { "Hold",        2, SPELL_DIRECT, EF_HOLD,       1,  6, 3,  SPELL_AI_TARGET },
    [SP_ENFEEBLE]    = { "Enfeeble",    2, SPELL_DIRECT, EF_WEAKNESS,   2,  6, 4,  SPELL_AI_TARGET },
    [SP_FIRE_BOLT]   = { "Fire Bolt",   3, SPELL_BOLT,   EF_BOLT_FIRE,  8,  8, 5,  SPELL_AI_TARGET },
    [SP_FROST_BOLT]  = { "Frost Bolt",  3, SPELL_BOLT,   EF_BOLT_COLD,  8,  8, 6,  SPELL_AI_TARGET },
    [SP_STONE_SKIN]  = { "Stoneskin",   4, SPELL_SELF,   EF_STONE_SKIN, 3,  0, 7,  SPELL_AI_NEVER },
    [SP_HASTE]       = { "Haste",       4, SPELL_SELF,   EF_HASTE,      3,  0, 8,  SPELL_AI_NEVER },
    [SP_POWER_BOLT]  = { "Power Bolt",  5, SPELL_BOLT,   EF_BOLT_MAGIC, 16, 8, 9,  SPELL_AI_TARGET },
    [SP_RENEWAL]     = { "Renewal",     5, SPELL_SELF,   EF_RENEW,      2,  0, 10, SPELL_AI_HURT },
};

/* MP by spell level (docs/spells.md, 4.2): rising with every level above 0, an ability's */
const uint8_t spell_costs[SPELL_LEVEL_MAX + 1] = { 0, 1, 2, 4, 6, 9 };

uint8_t spell_known[NGE_ENTITY_MAX][SPELL_SET_BYTES];
