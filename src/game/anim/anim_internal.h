#ifndef ANIM_INTERNAL_H
#define ANIM_INTERNAL_H

/*
 * Shared by the anim module's files; not for use outside it:
 * - anim_bank.c (page 126): starting the animations;
 * - anim_pdata.c (page 127): the creature kinds' sequences, mapped into
 *   slot 7 while its code runs.
 */

#include "game/anim/anim.h"
#include "game/gen/gen.h"

/* Each creature kind's sequences, by CR_* and GEN_SEQ_*, copied by anim_init() */
extern gen_seq_t anim_seqs[CR_COUNT][GEN_SEQ_COUNT];

#endif
