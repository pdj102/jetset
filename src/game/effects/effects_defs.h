#ifndef EFFECTS_DEFS_H
#define EFFECTS_DEFS_H

/*
 * The effect definitions: the shape of a row, its values, and the table,
 * in page 121 (effects_pdata.c). For the effects module and the attributes
 * module (game/attrs), which maps page 121 to copy a kind's statuses and
 * modifiers; not for use elsewhere (except the tests).
 *
 * nge only sets up an active effect (ecs/systems/nge_effect.h): a row's
 * period and cycles are given to its timer when it starts or is refreshed.
 */

#include "game/attrs/attrs.h"
#include "game/effects/effects.h"

/** The page the definitions are in (effects_pdata.c's constseg). */
#define EFFECTS_PDATA_PAGE  121

/* Durations */
#define EFFECT_INSTANT    0   /* on_start runs once; nothing is left behind */
#define EFFECT_TIMED      1   /* an active effect for period x cycles ticks */
#define EFFECT_WORN       2   /* counts through an equipped item's worn hook; never applied */
#define EFFECT_DURATIONS  3

/* Definition flags */
#define EFFECT_STACK   0x01  /* timed, applied again: another instance; else the one there is refreshed */
#define EFFECT_INNATE  0x02  /* timed, for ever and no saving throw: what the creature is, so no cure ends it */

/* Modifiers per definition */
#define EFFECT_MODS  2

/* Ops */
#define OP_NONE        0
#define OP_CURE        1   /* end the target's effects granting statuses arg (effect_cure()) */
#define OP_RAISE_BASE  2   /* add amount (maybe negative) to attribute arg's base, for good */
#define OP_DAMAGE      3   /* amount damage of type arg (DMG_*), through health_damage() */
#define OP_HEAL        4   /* heal amount, through health_heal() */
#define OP_MANA        5   /* restore amount MP, through mana_restore(): nothing without mana */
#define OP_UNCURSE     10  /* lift the curse of everything the target has equipped (ITEM_UNCURSED) */
/* Pending: they do nothing until their mechanics are built (docs/effects.md, "Pending effects") */
#define OP_LIGHT       6
#define OP_IDENTIFY    7
#define OP_MAP         8
#define OP_TELEPORT    9
#define OP_ENCHANT     11
#define OP_SUMMON      12
#define OP_COUNT       13

typedef struct effect_op {
    uint8_t op;       /* OP_* */
    uint8_t arg;      /* a damage type, statuses or an attribute */
    int8_t  amount;   /* per power */
} effect_op_t;

typedef struct effect_mod {
    uint8_t attr;     /* ATTR_* */
    int8_t  amount;   /* per power; a mask attribute's bits, unscaled */
} effect_mod_t;

typedef struct effect_save {
    uint8_t arg;   /* the ability rolled (ABIL_*) */
    uint8_t dc;    /* what the roll must reach; 0 for no saving throw */
} effect_save_t;

typedef struct effect_def {
    const char  *name;         /* timed: shown while it lasts (the PC screen), 14 characters at most; in page 121 */
    const char  *start_text;   /* in page 121, or NULL */
    const char  *end_text;
    uint8_t      duration;     /* EFFECT_INSTANT, EFFECT_TIMED or EFFECT_WORN */
    uint8_t      flags;        /* EFFECT_STACK, EFFECT_INNATE */
    uint8_t      period;       /* timed: ticks between firings, 1-255 (the timer's period) */
    uint8_t      cycles;       /* timed: firings, 0 for ever */
    uint8_t      status;       /* statuses granted while it lasts (STATUS_*) */
    effect_mod_t mod[EFFECT_MODS];
    effect_op_t  on_start;     /* once, when applied */
    effect_op_t  on_tick;      /* timed: at each firing, a period after the last */
    effect_op_t  on_end;       /* timed: once, when it ends (expired, cured or saved) */
    effect_save_t save;        /* timed: a saving throw that ends it; dc 0 for none */
} effect_def_t;

extern const effect_def_t effect_defs[EF_COUNT];

#endif
