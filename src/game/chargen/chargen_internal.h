#ifndef CHARGEN_INTERNAL_H
#define CHARGEN_INTERNAL_H

/*
 * Shared by the chargen module's files only: the class table, in page 140
 * with the code that reads it (chargen_bank.c, chargen_screen_bank.c).
 */

#include <stdint.h>

#include "game/components/pc.h"

/* A kit item: its kind, and whether it starts equipped */
typedef struct chargen_kit {
    uint8_t kind;    /* IT_* */
    uint8_t equip;   /* 1: equipped (in the first free slot it fits), 0: carried */
} chargen_kit_t;

typedef struct chargen_class {
    const char *name;               /* "Fighter": CHARGEN_NAME_MAX characters at most */
    const char *default_name;       /* until the player types one: PC_NAME_MAX at most */
    uint8_t creature;               /* CR_*: abilities, HP, mana and spells */
    const chargen_kit_t *kit;
    uint8_t kit_count;
} chargen_class_t;

/* By PC_CLASS_*: page 140, so only this module's code reads it */
extern const chargen_class_t chargen_classes[PC_CLASS_COUNT];

#endif
