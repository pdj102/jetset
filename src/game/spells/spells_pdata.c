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

/* docs/spells.md, section 4.12. Powers follow the items' */
const spell_def_t spell_defs[SPELL_COUNT] = {
    [SP_NONE]        = { 0 },
    [SP_MAGIC_BOLT]  = { "Magic Bolt",  1, SPELL_BOLT,   EF_BOLT_MAGIC, 3,  8, SPELL_AI_TARGET },
    [SP_CURE_WOUNDS] = { "Cure Wounds", 1, SPELL_SELF,   EF_HEALING,    4,  0, SPELL_AI_HURT },
    [SP_HOLD]        = { "Hold",        2, SPELL_DIRECT, EF_HOLD,       1,  6, SPELL_AI_TARGET },
    [SP_ENFEEBLE]    = { "Enfeeble",    2, SPELL_DIRECT, EF_WEAKNESS,   2,  6, SPELL_AI_TARGET },
    [SP_FIRE_BOLT]   = { "Fire Bolt",   3, SPELL_BOLT,   EF_BOLT_FIRE,  8,  8, SPELL_AI_TARGET },
    [SP_FROST_BOLT]  = { "Frost Bolt",  3, SPELL_BOLT,   EF_BOLT_COLD,  8,  8, SPELL_AI_TARGET },
    [SP_STONE_SKIN]  = { "Stoneskin",   4, SPELL_SELF,   EF_STONE_SKIN, 3,  0, SPELL_AI_NEVER },
    [SP_HASTE]       = { "Haste",       4, SPELL_SELF,   EF_HASTE,      3,  0, SPELL_AI_NEVER },
    [SP_POWER_BOLT]  = { "Power Bolt",  5, SPELL_BOLT,   EF_BOLT_MAGIC, 16, 8, SPELL_AI_TARGET },
    [SP_RENEWAL]     = { "Renewal",     5, SPELL_SELF,   EF_RENEW,      2,  0, SPELL_AI_HURT },
};

/* MP by spell level (docs/spells.md, 4.2): rising with every level above 0, an ability's */
const uint8_t spell_costs[SPELL_LEVEL_MAX + 1] = { 0, 1, 2, 4, 6, 9 };

/*
 * Spell slots by class, character level and spell level (docs/spells.md,
 * D27): the first slot of spell level L at character level 2L - 1 (R11),
 * never falling as the level rises (R35). The Fighter has none; the Dev
 * has the Mage's (Q4)
 */
#define MAGE_SLOTS                                                           \
    { { 2, 0, 0, 0, 0 }, { 3, 0, 0, 0, 0 }, { 3, 1, 0, 0, 0 }, { 3, 2, 0, 0, 0 }, \
      { 4, 2, 1, 0, 0 }, { 4, 2, 2, 0, 0 }, { 4, 3, 2, 1, 0 }, { 4, 3, 2, 2, 0 }, \
      { 5, 3, 3, 2, 1 }, { 5, 4, 3, 2, 2 } }

const uint8_t spell_slots_table[PC_CLASS_COUNT][PC_LEVEL_MAX][SPELL_LEVEL_MAX] = {
    [PC_CLASS_FIGHTER] = { { 0 } },
    [PC_CLASS_MAGE]    = MAGE_SLOTS,
    [PC_CLASS_DEV]     = MAGE_SLOTS,
};

/* Each book's spells (docs/spells.md, D28): basic levels 1 and 2, adept 3 and 4, master 5 (R43) */
const uint8_t spell_books[SPELL_BOOK_COUNT][SPELL_BOOK_MAX] = {
    [SPELL_BOOK_BASIC]  = { SP_MAGIC_BOLT, SP_CURE_WOUNDS, SP_HOLD, SP_ENFEEBLE },
    [SPELL_BOOK_ADEPT]  = { SP_FIRE_BOLT, SP_FROST_BOLT, SP_STONE_SKIN, SP_HASTE },
    [SPELL_BOOK_MASTER] = { SP_POWER_BOLT, SP_RENEWAL },
};

uint8_t spell_known[NGE_ENTITY_MAX][SPELL_SET_BYTES];
