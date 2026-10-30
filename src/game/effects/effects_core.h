#ifndef EFFECTS_CORE_H
#define EFFECTS_CORE_H

/*
 * The effects module's internals: not for use outside it (except the
 * tests). The module is split in two, so its mechanism could become an
 * nge module once another game or system needs it:
 *
 * - the core (effects_core.c, page 120): the mechanism. Applying, ending
 *   and curing effects, active effects' lifecycle (an entity owned by its
 *   target, with a timer; refreshed or stacked), and recomputing derived
 *   values from the bases and the modifiers and statuses of each source.
 *   It knows nothing of the game's components: it may include only this
 *   file, nge headers and game/events.h (tests/Makefile checks it);
 * - the game's side (effects_game.c, page 120): the content. What the
 *   attributes mean and where their bases and values are stored
 *   (effects_game_attrs[]), the game's ops (OP_GAME and up,
 *   effects_game.h), the sources that aren't active effects (equipped
 *   items), which entities effects act on;
 * - effects_pdata.c (page 121): the definitions and the active effects'
 *   kinds and powers, in slot 7 only while the code in page 120 runs.
 *
 * The seam, below, is what the core calls on the game's side: plain
 * functions in the same page today, the callbacks of an nge API later.
 */

#include "game/components/attrs.h"
#include "game/effects/effects.h"

/* Durations */
#define EFFECT_INSTANT    0   /* on_start runs once; nothing is left behind */
#define EFFECT_TIMED      1   /* an active effect for period x cycles ticks */
#define EFFECT_WORN       2   /* counts through a source the game adds (an equipped item); never applied */
#define EFFECT_DURATIONS  3

/* Definition flags */
#define EFFECT_STACK  0x01   /* timed, applied again: another instance; else the one there is refreshed */

/* Modifiers per definition */
#define EFFECT_MODS  2

/* Ops the core does itself; the game's are numbered from OP_GAME (effects_game.h) */
#define OP_NONE        0
#define OP_CURE        1   /* end the target's effects granting statuses arg (effect_cure()) */
#define OP_RAISE_BASE  2   /* add amount (maybe negative) to attribute arg's base, for good */
#define OP_GAME        3   /* the first of the game's ops (effects_game_op()) */

typedef struct effect_op {
    uint8_t op;       /* OP_* */
    uint8_t arg;      /* a damage type, statuses or an attribute */
    int8_t  amount;   /* per power */
} effect_op_t;

/* A modifier: amount per power added to a number attribute, or ORed (as a uint8_t) into a mask; 0 changes nothing */
typedef struct effect_mod {
    uint8_t attr;     /* ATTR_* */
    int8_t  amount;
} effect_mod_t;

/* A saving throw: tried at each firing of a timed effect, after its tick op; passed ends the effect */
typedef struct effect_save {
    uint8_t arg;   /* what the game's side rolls with (an ability) */
    uint8_t dc;    /* what the roll must reach; 0 for no saving throw */
} effect_save_t;

typedef struct effect_def {
    const char  *name;         /* timed: shown while it lasts (the PC screen), 14 characters at most; in page 121 */
    const char  *start_text;   /* in page 121, or NULL */
    const char  *end_text;
    uint8_t      duration;     /* EFFECT_INSTANT, EFFECT_TIMED or EFFECT_WORN */
    uint8_t      flags;        /* EFFECT_STACK */
    uint8_t      period;       /* timed: ticks between firings, 1-255 */
    uint8_t      cycles;       /* timed: firings, 0 for ever */
    uint8_t      status;       /* statuses granted while it lasts: bits the game defines */
    effect_mod_t mod[EFFECT_MODS];
    effect_op_t  on_start;     /* once, when applied */
    effect_op_t  on_tick;      /* timed: at each firing, a period after the last */
    effect_op_t  on_end;       /* timed: once, when it ends (expired, cured or saved) */
    effect_save_t save;        /* timed: a saving throw that ends it; dc 0 for none */
} effect_def_t;

/* Derived values being worked out: numbers can pass 255 or go below 0 until the game clamps them */
typedef struct effects_derived {
    int16_t value[ATTR_COUNT];
    uint8_t status;
} effects_derived_t;

/* Page 121 */
extern const effect_def_t effect_defs[EF_COUNT];
extern uint8_t effect_kind[NGE_ENTITY_MAX];   /* EF_* of each active effect (COMPONENT_EFFECT) */
extern int8_t  effect_power[NGE_ENTITY_MAX];  /* its power: what started it may be gone */

/* ---- The core, for the game's side (page 120, plain calls) ---- */

/* Add effect ef's modifiers at power, and its statuses, to d (EF_NONE adds nothing) */
void effects_add(effects_derived_t *d, uint8_t ef, int8_t power);

/* ---- The seam: the game's side, called by the core (page 120, plain calls; data in page 121) ---- */

/* An attribute (ATTR_*): its base table, the component that has it, and how modifiers combine */
typedef struct effects_attr {
    uint8_t             *base;        /* the component's base table, [NGE_ENTITY_MAX] */
    nge_component_t      component;   /* an entity has the attribute only with it */
    uint8_t              is_mask;     /* modifiers ORed (1) or added (0) */
} effects_attr_t;

/* The attributes, by ATTR_* */
extern const effects_attr_t effects_game_attrs[ATTR_COUNT];

/* The timer kind of active effects: its handler calls effects_on_timer() */
extern const uint8_t effects_game_timer_kind;

/* id has attributes: effects_recompute() and effects_set_base() work on it */
uint8_t effects_game_has_attrs(nge_entity_id_t id);

/* Effects can act on id: it has attributes and isn't dead */
uint8_t effects_game_affectable(nge_entity_id_t id);

/* 1 if target passes saving throw arg (effect_save_t) against dc (1 or more) */
uint8_t effects_game_save(nge_entity_id_t target, uint8_t arg, uint8_t dc);

/* Do a game op (OP_GAME and up) to target, caused by source: its amount is already scaled by the power */
void effects_game_op(const effect_op_t *op, nge_entity_id_t target, nge_entity_id_t source);

/* Add the sources of modifiers that aren't active effects (worn items' effects) to d, with effects_add() */
void effects_game_sources(nge_entity_id_t id, effects_derived_t *d);

/*
 * Clamp d's values and store them as id's derived values (write 1), or
 * only compare (write 0). Returns 1 if they differ from what is stored
 */
uint8_t effects_game_store(nge_entity_id_t id, const effects_derived_t *d, uint8_t write);

#endif
