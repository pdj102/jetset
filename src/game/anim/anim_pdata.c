/* nge-dataseg: PAGE_127 */
/*
 * The anim module's page data, in page 127 (slot 7 while its code in page
 * 126 runs): the creature kinds' sequences, a copy of their kind records'
 * (docs/animations.md, D8). Uninitialised: anim_init() sets it. See "Page
 * data" in docs/memory_map.md.
 */
#include "game/anim/anim_internal.h"

gen_seq_t anim_seqs[CR_COUNT][GEN_SEQ_COUNT];
