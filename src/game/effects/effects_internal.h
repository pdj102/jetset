#ifndef EFFECTS_INTERNAL_H
#define EFFECTS_INTERNAL_H

/*
 * Shared by the effects module's files; not for use outside it (except the
 * tests):
 * - effects_bank.c (page 120): the functions;
 * - effects_pdata.c (page 121): the definitions and the active effects'
 *   kinds, in slot 7 only while the code in page 120 runs.
 */

#include "game/effects/effects.h"

/* Ops: what an effect does at its start, each period and at its end */
#define OP_NONE        0
#define OP_DAMAGE      1   /* amount damage of type arg (DMG_*), through health_damage() */
#define OP_HEAL        2   /* heal amount, through health_heal() */
#define OP_CURE        3   /* end the target's effects granting statuses arg (effect_cure()) */
#define OP_RAISE_BASE  4   /* add amount (maybe negative) to attribute arg's base, for good */
#define OP_COUNT       5

/* Definition flags */
#define EFFECT_TIMED  0x01   /* an active effect for period x cycles ticks; else instant (or worn) */
#define EFFECT_STACK  0x02   /* applied again: another instance; else the one there is refreshed */

/* Modifiers per definition */
#define EFFECT_MODS  2

typedef struct effect_op {
    uint8_t op;       /* OP_* */
    uint8_t arg;      /* a damage type, statuses or an attribute */
    int8_t  amount;
} effect_op_t;

/* A modifier: added to a number attribute, ORed (as a uint8_t) into a mask; 0 changes nothing */
typedef struct effect_mod {
    uint8_t attr;     /* ATTR_* */
    int8_t  amount;
} effect_mod_t;

typedef struct effect_def {
    const char  *start_text;   /* in page 121, or NULL */
    const char  *end_text;
    uint8_t      flags;        /* EFFECT_* */
    uint8_t      period;       /* timed: ticks between firings, 1-255 */
    uint8_t      cycles;       /* timed: firings, 0 for ever */
    uint8_t      status;       /* STATUS_* granted while it lasts */
    effect_mod_t mod[EFFECT_MODS];
    effect_op_t  start;
    effect_op_t  tick;
    effect_op_t  end;
} effect_def_t;

/* Page 121 */
extern const effect_def_t effect_defs[EF_COUNT];
extern uint8_t effect_kind[NGE_ENTITY_MAX];   /* EF_* of each active effect (COMPONENT_EFFECT) */

#endif
